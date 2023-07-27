# Low Power Task Scheduler

 The LPTASK cooperative task scheduler library provides an easy-to-use 
 mechanism for scheduling tasks to be executed in the future without the 
 complexity or overhead of an operating system.  Once scheduled, a task's 
 handler is executed from the main context once its programmed interval 
 expires.  

## Major Features

The scheduler offers the following features:

* The scheduler's design has been optimized for low-power embedded 
applications.  Every architecture decision was made with the goal of performing 
the required work and putting the the processors back to sleep as efficiently as 
possible.
* The platform-specific port function make is easy for developers to take 
advantage of the sleep, timer and other power-reduction mechanisms provided 
by a particular processor.
* The core scheduler module only requires ~1100 bytes of ROM making it 
well-suited for embedded platforms.
* Tasks are statically allocated, providing a fixed compile time memory 
footprint. 
* Each scheduler task only requires 20 bytes of RAM on a typical 32-bit 
processor.
* The scheduler is simple to learn and easy to use.  Configuring and starting a 
new task only requires a few lines of code.  
* The scheduler encourages the practice of writing lightweight interrupt 
handlers which typically improves the system responsiveness and the stability of 
an embedded application.  Work from interrupt handlers can easily be moved into 
the main context with minimal overhead.

## Comparison to a Preemptive OS / RTOS
                                                            
The cooperative scheduler does not support concurrency like a preemptive OS 
does.  A cooperative task's handler function executes until it returns, once 
the task interval expires, in a non-preemptive manner.  The task handler's 
execution can only be suspended by a interrupt or exception event and not by 
another scheduler task.

A preemptive OS, in comparison, executes multiple threads in parallel.  A 
thread can be suspended and resumed at a later time.   Thread concurrency is 
very beneficial for managing multiple long running tasks in complex 
implementations but it requires some additional OS overhead. Shared resources 
must also be atomically accessed to avoid conflicts between the threads. A 
cooperative scheduler can avoid the resource sharing issues since only one task 
is ever be active at a given time.

Since a cooperative task's handler can not be suspended by another task,  
task handlers should ideally have a relatively short execution time.  A task 
handler with a long execution time might delay other tasks introducing 
interval error.  In practice, the problem rarely appears in a low-power 
embedded systems which typically do not perform computationally intensive 
operations.  If it does occur, longer running task can either be broken up into 
shorter tasks or be configured as repeating tasks which perform their work 
incrementally in smaller chunks.  

## Use Cases

The LPTASK scheduler can be used in almost any simple to medium complexity 
single processor embedded system but it really shines in applications 
which have some of the following characteristics:

* Power reduction is a high priority which is almost always the case for 
battery powered devices.
* The application has low duty cycle, the processor is anticipated to 
be sleeping the majority of the time. 
* The platform supports sleeping in a low-power state during periods 
of inactivity.
* RAM and ROM resources are limited.

Low-power embedded systems typically only require hard real time performance 
for a small subset of their tasks and often don't require hard real-time 
performance at all.  These systems can live within a cooperative scheduler's 
constraints, saving the overhead and complexity required by a typical 
preemptive OS and potentially reducing the system's power consumption. 

<p align="center">
  <img src="./docs/img/scheduler_app.svg" vspace="10" 
alt="Typical Scheduler Application">
</p>

## Initializing & Starting the Scheduler

The scheduler must be initialized with the `sched_init()` function and then 
started with `sched_start()` from the main context.  The start function 
repeatably services any expired tasks and sleeps when no tasks are active.  The 
function does not return, once started, until the the scheduler has been 
stopped.  
```c
int main() {
  // Initialize the Scheduler.
  sched_init();
  
  // Start the Scheduler.
  sched_start();
}
```

## Unbuffered vs. Buffered Tasks

The scheduler supports both unbuffered and buffered task types. Unbuffered tasks 
don't have an internal data buffer. Data is added to the unbuffered tasks by 
reference.  Only a pointer to the user-supplied data and the data length are 
stored inside the task structure when the `sched_task_data()` function is 
called.  The externally stored data must still be valid when the task handler 
is called at a later point in time.    

Buffered tasks have dedicated internal memory for storing the user's task 
data.  Data is added to a buffered task by copying it to the task's internal 
data buffer when the `sched_task_data()` function is called. 

| Task Type             | Unbuffered              | Buffered |
|  :----                | :----                   |  :----    |
| Internal Buffer Size  | 0 Bytes                 | 1 to 255 Bytes      |
| Data Storage          | Stored by Reference     | Stored by Copy       |
| Data Lifetime         | The data must still be valid when the task's handler is called.  | The data only needs to be valid at the time that it is added to the task. |
| Task Definition      | `SCHED_TASK_DEF()`        | `SCHED_TASK_BUFF_DEF()` or allocated from a task pool.|

An unbuffered scheduler task is defined with the `SCHED_TASK_DEF()` macro. 

```c
// Unbuffered Task Definition
SCHED_TASK_DEF(my_task);
```
A buffered task is defined with the `SCHED_TASK_BUFF_DEF()` macro.  The size of 
the task's internal buffer is supplied as a parameter to the macro.  The size
parameter represents maximum data size that can be stored in the task buffer.

```c
// Buffered Task Definition
SCHED_TASK_BUFF_DEF(my_buff_task, sizeof(task_data_t));
```

## Buffered Task Pools

Task pool's are an alternative method of defining tasks.  A task pool is a 
collection of reusable buffered tasks which can be dynamically allocated at 
runtime.  Once a task is removed from the task pool at allocation, it is 
accessed in the same way that a regular buffered task would be.  The task 
remains allocated until the task is stopped, at which point it returns to the 
pool and can be allocated again for other purposes.

A task pool is defined with the `SCHED_TASK_POOL_DEF()` macro.   The macro 
includes parameters for setting the internal data buffer size to reserve for 
each task and the number of tasks in the pool. 

```c
 // Data structure for storing data received from a UART.
typedef struct {
  uint8_t data[64];       // Data storage.
  uint8_t len;            // Length of the data (bytes).
} uart_data_t;

// Define a buffered task pool for handling UART data from the main context.
SCHED_TASK_POOL_DEF(uart_pool, sizeof(uart_data_t), 2);
```

A task can be allocated from the pool with the `sched_task_alloc()` function.  
The function returns a pointer to the allocated task or `NULL` if no 
tasks could be allocated.

```c
static void uart_isr() {

  // Attempt to allocate a new task from the task pool.
  sched_task_t * p_task = sched_task_alloc(&uart_pool);

  // The pool's task count may need to be increased if NULL was returned.
  assert(p_task != NULL);

  ...
}
```

## Task Handler

Each task has a handler function assigned with the `sched_task_config()` 
function.  The handler function is used to perform the task's work and is 
called from the main context once the task's interval has expired.  The handler 
function must follow the `sched_handler_t` prototype. 

A reference to the task is supplied to the handler function when it is called 
so that the task can be updated inside the handler function.  For example, a 
repeating task could be stopped inside its handler once a condition has been 
met.

```c
// Task Handler Function
static void my_task_handler(sched_task_t *p_task, void *p_data, uint8_t data_size) {
  // Perform the task work.
}
```

## Configuring & Starting Tasks

Once defined or allocated in the case of a task pool, buffered and unbuffered 
tasks are accessed in the same way.  

Each task must to be configured with the `sched_task_config()` function before 
it can be started with `sched_task_start()` function. 

Tasks can optionally have data added to them with the `sched_task_data()` 
function before being started.

```c
// Configure the task to repeat every 100 mS.
sched_task_config(&my_task, my_task_handler, 100, true);

// Add data to the task.
uint8_t add_len = sched_task_data(&my_task, &my_task_data, sizeof(my_task_data));

// Check if the data was successfully added to the task.
assert(add_len == sizeof(my_task_data));

// Start the task.
sched_task_start(&my_task);
```

## Task Access

Access to the task functions are restricted by the following rules:

1. A task must be configured with the `sched_task_config()` function before 
it can be accessed with any of the other task functions.
2. Once configured and started, a task can only be reconfigured with the 
`sched_task_config()` after it has stopped.  
3. The task data can only be set with the `sched_task_data()` function when 
the task is stopped.  

Note that the values of the data stored inside a buffered task can be updated 
inside of its task handler using the data pointer supplied to the handler.  For 
example, a repeating task might utilize the buffer memory to track its current 
state.  A task might generate a series of LED patterns for example.  The handler 
could increment an index value to cycle through the LED patterns at each call. 
The task data should typically only be modified inside the task handler since 
modifying the data from an interrupt or exception context might lead to a race 
condition unless an atomic data type were utilized.

The [Task State](./docs/task_state.md) documentation offers a more detailed 
explanation of the task access control mechanism.

## Porting to a New Platform
 
An [example project](./examples/README.md) is available in the `/examples/` 
folder and more are projects are planned.  If your processor doesn't match the 
example, you will need to implement the mandatory functions defined in the
[sched_port.h](./src/scheduler/sched_port.h) header.  See the 
[Platform Port](./docs/port.md) document for more detailed information on 
adding support for a new platform.

## Additional Documentation

Several [Build Configuration](./docs/build_config.md) options are provided to 
customize the scheduler for particular project. A series of 
[Test Projects](./test/POSIX/README.md) have been created to automate 
verification of the scheduler's operation under a POSIX environment.  A 
[Project Style Guide](./docs/style_guide.md) and a brief 
[Project History](./docs/about.md) are also available.

### License

This project is licensed under the terms of the [MIT license](./LICENSE.md).
