# Embedded Scheduler

 The C language cooperative scheduler module provides an easy to use mechanism for scheduling tasks to be executed in the future without the complexity or overhead of a multi-tasking operating system.  Once scheduled, a task's handler is executed from the main context after its interval timer expires.  Tasks can be configured as one-shot or repeating. 

## Major Features

The scheduler offers the following features:

* The core scheduler module consumes ~1,000 bytes of ROM making it well suited for embedded platforms.
* All memory is statically allocated providing a fixed compile time memory footprint.
* Each unbuffered scheduled task only requires 20 bytes of RAM on a typical 32-bit processor.
* The scheduler is simple to learn and easy to use, configuring and starting a new task only requires a few lines of code.  
* The scheduler encourages the practice of writing lightweight interrupt handlers which can improve system responsiveness and stability.  ISR work can easily but moved into the main context with minimal overhead.
* The scheduler was architected with an eye towards efficiency and power reduction.  It offers flexible support for platform specific sleep, timer and power reductions mechanisms through optional platform specific function calls.

## Comparison to Multitasking OS
                                                            
The cooperative scheduler does not provide all of the same features which a multitasking OS typically does, the major differences include:

* Expired tasks are are executed in the order in which they are added to the scheduler's que, no task prioritization functionality is provided.  
* A task's handler executes until completion, once the task expires, in a cooperative manner.  A task handler can only be suspended by a interrupt or exceptation events and not by another schduler tasks.
* It's typical to have several milliseconds of jitter in task execution intervals for a system with multiple active tasks queued at the same time.  This jitter is nearly always acceptable for UI tasks such as blinking an LED, debouncing a switch or timing the length of music note.
* Tasks which require finer grain control or more deterministic behavior can be implemented using a separate hardware timer.  For example, an application might implement a real-time motion control loop with a 100 Hz hardware timer and perform UI tasks with the scheduler. 
                                                        
Most typical embedded systems only require hard real time performance for  small subsett of their tasks and often don't require hard real performance at all.  These systems can live within the schedulers constraints saving the overhead and complexity required by a typical RTOS.  

## Cooperative Scheduler Use Cases

The scheduler can be used in most simple to meduim complexity embedded systems but really shines for appllications which have some of the folllowing characteritics.

* Power reduction is a high priority.
* The selected platform has the hardware required to to stop program execution and sleep in a low power state.
* The platform is anticipated to be sleeping the majority of the time. 
* RAM and ROM resources are limited.


## Initializing & Starting the Scheduler

The scheduler must be initialized and started from the main context.  The `sched_start()` function repeatably executes expired tasks sleeping when no tasks are active.  The function does not return until the the scheduler is stopped.  Many implementations will never need to stop the scheduler unless they need reset the processor to enter the bootloader for a firmware upgrade or similar task.

```c
int main() {
  // Initialize the Scheduler.
  sched_init();
  
  // Start the Scheduler.
  sched_start();
}
```

## Task Types

The scheduler supports both buffered and unbuffered task types. Buffered tasks have their own internal data memory for storing user data.  Data is added to buffered task by copying it the task's internal data buffer.

Unbuffered tasks don't have an internal data buffer.  Data is added to an unbuffered task by reference,  only a pointer to the user data and the data size are stored in the task.  This means that user data must still be available when the task handler is called at a later point in time.    

| Task Type             | Unbuffered              | Buffered |
|  :----                | :----                   |  :----    |
| Internal Buffer Size  | 0 Bytes                 | 1 to 255 Bytes      |
| Data Storage          | Stored by Reference     | Stored by Copy       |
| Data Lifetime         | The user data must still be valid at later handler call.  | Since the user data is copied to the internal task buffer, the data only needs to be valid when it is added to the task. |
| Task Definition      | SCHED_TASK_DEF()        | SCHED_TASK_DEF_BUFF() or allocated from a Task Pool|


## Task Handler

Each task must have a task handler function defined.  The hander function is be used to perform the task's work and is called from the main context once the task's interval has expired.  The handler must follow the `sched_handler_t` function prototype. 

```c
/*
 * Custom data struture for passing data received from a UART 
 * interrupt service routine to the main context for handling. 
 */
typedef struct {
  uint8_t data[32];       // Data received
  uint8_t len;            // Length of the data
  bool pending            // Is there more data coming?
} uart_data_t;

// Custom task handler function.
static void uart_data_handler(sched_task_t *p_task, void *p_data, uint8_t data_size) {

    // Check if the data size matches the custom structure size.
    assert(sizeof(uart_data_t) == data_size);

    // Cast the task data pointer to the custom stucture.
    uart_data_t * p_uart_data = (uart_data_t *) p_data;

    // Perform the task's work.
    // p_uart_data->pending ...
}
```

A reference to the task itself is supplied to the handler so that the task can updated inside the handler function if needed.  For example, a repeating task might be stopped after a certain condition is met.   

### Defining & Configuring an Unbuffered Task

A an unbuffered scheduler task should be defined with the `SCHED_TASK_DEF()` macro. 

```c
// Unbuffered Task Definition
SCHED_TASK_DEF(my_task);
```
Each task needs to be configured with the `sched_task_config()` function before starting it with `sched_task_start()`. 

```c
// Configure the task to repeat every 100 mS.
sched_task_config(&my_task, my_task_handler, NULL, 100, true);

// Add the task to the schedulers task que.
sched_task_start(&my_task);
```

## Defining & Configuring a Buffered Task

TODO

## Task Pools

TODO


## Porting to a New Platform
 
Several [example projects](./examples/README.md) are provided in the  `/examples/` folder for various hardware platforms.  If your processor doesn't match one of the example projects, you will need to implement the mandatory functions defined in [sched_port.h](./src/scheduler/sched_port.h).  See the [Platform Port](./docs/port.md) document for more detailed information on adding support for a new platform.