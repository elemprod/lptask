# Low Power Task Scheduler

 LPTASK is a cooperative / non-preemptive task scheduler library which provides an easy-to-use mechanism for scheduling tasks to be executed in the future without the complexity or overhead of an operating system.  Once scheduled, a task's handler is executed from the main context once its interval timer expires.  

## Major Features

The scheduler offers the following features:

* The scheduler's design has been optimized for low power embedded applications.  Every design decision was made with the goal of performing the required work  and putting the the processors back to sleep as efficiently as possible.
* The platform-specific port function make is easy for developers to take advantage of any sleep, timer and other power reduction mechanisms provided by a particular processor.
* The core scheduler only module consumes ~1,000 bytes of ROM making it well suited for embedded platforms.
* All scheduler and task memory is statically allocated, providing a fixed compile time memory footprint. 
* Each unbuffered scheduler task only requires 20 bytes of RAM on a typical 32-bit processor.
* The scheduler is simple to learn and easy to use.  Configuring and starting a new task only requires a few lines of code.  
* The scheduler encourages the practice of writing lightweight interrupt handlers which can improve the system responsiveness and stability of an embedded application.  Work from interrupt handlers can easily be moved into the main context with minimal overhead.

## Comparison to Preemptive OS
                                                            
The cooperative scheduler does not provide all of the same features which a preemptive OS typically does.  The major differences include:

* Expired tasks are executed in the order in which they are added to the scheduler's que.  No task prioritization functionality is provided.  
* A task's handler executes until completion, once the task expires, in a cooperative manner.  A task handler can only be suspended by a interrupt or exception events and not by another scheduler task.
* It's typical to have several milliseconds of jitter in task execution intervals for a system with multiple active tasks queued at the same time.  This jitter is nearly always acceptable for UI tasks, such as blinking an LED, debouncing a switch or timing the length of music note.
* Tasks which require finer grain control or more deterministic behavior should be implemented with a dedicated  hardware timer.  For example, an application might implement a real-time motion control loop with a 50 Hz hardware timer and perform UI tasks with the scheduler. 
                                                        
Most embedded systems only require hard real time performance for a small subset of their tasks and often don't require hard real time performance at all.  These systems can live within a cooperative scheduler's constraints, saving the overhead and complexity required by a typical RTOS and ultimately reducing the overall system power consumption. 

## Use Cases

The LPTASK scheduler library can be used in almost any simple to medium complexity embedded systems but it really shines in applications which have some of the following characteristics:

* Power reduction is a high priority which is often the case for battery powered devices.
* The application is low low-duty cycle, the processor is anticipated to be sleeping the majority of the time. 
* The selected platform has the hardware required to pause program execution and sleep in a low power state during inactivity.
* RAM and ROM resources are limited.

<img src="./docs/img/scheduler_app.svg" align="center" hspace="15" vspace="15" alt="Typical Scheduler Application">

## Initializing & Starting the Scheduler

The scheduler must be initialized and started from the main context.  The `sched_start()` function repeatably executes expired tasks, sleeping when no tasks are active.  The function does not return until the the scheduler is stopped.  Many implementations will never need to stop the scheduler unless they need reset the processor to enter the boot loader for a firmware upgrade or similar task.

```c
int main() {
  // Initialize the Scheduler.
  sched_init();
  
  // Start the Scheduler.
  sched_start();
}
```

## Unbuffered vs. Buffered Tasks

The scheduler supports both buffered and unbuffered task types. Buffered tasks have their own internal data memory for storing user data.  Data is added to a buffered task by copying it to the task's internal data buffer by calling `sched_task_data()`.

Unbuffered tasks don't have an internal data buffer.  Data is added to the task by reference.  Only a pointer to the externally stored user data and the data size are stored in the task.  Any user data added to the task with `sched_task_data()` must still be valid when the task handler is called at a later point in time.    

| Task Type             | Unbuffered              | Buffered |
|  :----                | :----                   |  :----    |
| Internal Buffer Size  | 0 Bytes                 | 1 to 255 Bytes      |
| Data Storage          | Stored by Reference     | Stored by Copy       |
| Data Lifetime         | The externally stored data must still be valid when the task's handler is called.  | Since the data is copied to the internal buffer, the data only needs to be valid at the time that it is added to the task. |
| Task Definition      | `SCHED_TASK_DEF()`        | `SCHED_TASK_BUFF_DEF()` or allocated from a task pool.|

An unbuffered scheduler task is defined with the `SCHED_TASK_DEF()` macro. 

```c
// Unbuffered Task Definition
SCHED_TASK_DEF(my_task);
```

A buffered task is defined with the `SCHED_TASK_BUFF_DEF()` macro.  The size of the task's internal buffer is supplied as a parameter to the macro.

```c
// Buffered Task Definition
SCHED_TASK_BUFF_DEF(my_buff_task, sizeof(task_data_t));
```
 
## Buffered Task Pools

Task pool's are an alternative method of defining tasks.  A task pool is a collection of reusable buffered tasks which can be dynamically allocated at runtime.  Once a task is removed from the task pool at allocation, it can be can accessed in the same way that a regular buffered task would be.  The task remains allocated until the task is stopped at which point it returns to the pool and can be allocated again for other purposes.

A task pool is defined with the `SCHED_TASK_POOL_DEF()` macro.   The macro includes parameters which define the internal buffer size to reserve for each task and the number of tasks in the pool. 

```c
 // Custom data structure for data received from a UART.
typedef struct {
  uint8_t data[32];       // Data received
  uint8_t len;            // Length of the data
} uart_data_t;

// Define a buffered task pool for the UART data with 4 tasks.
SCHED_TASK_POOL_DEF(uart_pool, sizeof(uart_data_t), 4);
```

A task is allocated from the pool with the `sched_task_alloc()` function which returns a pointer to the allocated task or `NULL` if no unallocated tasks are available.

```c
// Get a new task from the task pool.
sched_task_t * p_task = sched_task_alloc(&uart_pool);

// The pool's task count may need to be increased if sched_task_alloc() returns NULL.
assert(p_task != NULL);

```

## Task Handler

Each task must have a handler function assigned with the `sched_task_config()` function.  The handler function is used to perform the task's work and is called from the main context once the task's interval has expired.  The handler must follow the `sched_handler_t` function prototype. 

A reference to the task itself is supplied to the handler so that the task can be updated inside the handler function if needed.  For example, a repeating task might be stopped after a certain condition is met.  

```c
// Task Handle Function
static void my_task_handler(sched_task_t *p_task, void *p_data, uint8_t data_size) {
  // Perform the task work.
}
```

## Configuring & Starting Tasks

Once defined or allocated in the case of a task pool, buffered and unbuffered tasks are accessed in the same way.  

Each task needs to be configured with the `sched_task_config()` function before starting it with `sched_task_start()` function. 

Tasks can optionally have data added to them with the `sched_task_data()` function before being started.

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

1. A task must be configured with the `sched_task_config()` function before it can be accessed with any of the other task functions.
2. Once configured and started, a task can only be reconfigured with the `sched_task_config()` once it has stopped.  
3. The task data can only be set with the `sched_task_data()` function if the task is stopped.  

Note that the value of the data stored inside a buffered task can be updated inside of its task handler using the data pointer supplied to the handler.  For example, a repeating task might utilize a data structure to track its current state.  A task which produces a series of LED patterns might store the current LED pattern index in its task buffer.  The handler could cycle through the LED patterns at each call. The task data values should typically only be modified inside the task handler since modifying a data value in a different context might lead to access conflicts.

See the [Task State](./docs/task_state.md) documentation for more details on the access control mechanism.

## Porting to a New Platform
 
Several [example projects](./examples/README.md) are provided in the  `/examples/` folder for various hardware platforms.  If your processor doesn't match one of the examples, you will need to implement the mandatory functions defined in [sched_port.h](./src/scheduler/sched_port.h).  See the [Platform Port](./docs/port.md) document for more detailed information on adding support for a new platform.

## Additional Information

[Test Projects](./test/POSIX/README.md)

[Examples](./examples/README.md)

[Task State & Access Control](./docs/task_state.md)

[Project Style Guide](./docs/style_guide.md)

### License

This project is licensed under the terms of the [MIT license](./LICENSE.md).
