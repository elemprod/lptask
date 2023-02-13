# Embedded Scheduler

The C language scheduler module provides a simple method for scheduling tasks to be executed in the future without the complexity or overhead of a full RTOS.  Once scheduled, a task's handler is called from the main context once its interval timer has expired.  Tasks can be configured as repeating, in which case they will be executed periodically until stopped, or as one-shot tasks.  

## Major Features

The scheduler module has minimal RAM and ROM requirements making it well suited for typical embedded platforms and offers the following features:

* The core scheduler module is small and lightweight requiring ~650 bytes of ROM.
* Each scheduled task typically requires 20 bytes of RAM.
* Static memory allocation provides a fixed compile time memory footprint.
* The scheduler is simple to learn, configuring a new task only requires a few lines of code.  
* The scheduler encourages the practice of writing lightweight interrupt handlers which improves system responsiveness and stability.  ISR related work can easily but moved into the main context with minimal overhead.
* The scheduler was architected with an eye towards efficiency and power reduction.  It offers flexible support for platform specific sleep, timer and power reductions techniques.

## Comparison to RTOS
                                                            
The scheduler does not provide all of the same features that Real Time Operating System does, the major differences include:

* Scheduled tasks are are executed in the order in which they are added to the scheduler's que, no task prioritization functionality is provided.  
* A task handler executes until completion, once started, in a cooperative multitasking manner and are only interrupted by a hardware IRQ event.
* It's typical to have several milliseconds of jitter in task execution times for a system with several active tasks queued at the same time.  This jitter is nearly always acceptable for UI tasks such as blinking an LED, debouncing a switch or timing the length of music note.
* Repetitive tasks, which require finer grain control or more deterministic behavior, can be implemented using a separate hardware timer.  For example, an application could implement a real-time motion control loop with a 100 Hz hardware timer and perform UI tasks with the scheduler.  The performance of such a system will often exceed that of an full RTOS implementation due to the minimal scheduler overhead. 
                                                        
Many embedded systems don't require hard real time performance and can live within the schedulers constraints saving the overhead and complexity required by full RTOS.    

## Initializing & Starting the Scheduler

TODO

## Defining & Configuring a Task

A Scheduler Task must be defined with the `SCHED_TASK_DEF()` macro.  Each task must also have a task handler function defined follows `sched_handler_t` function prototype.  The handler is called called once a tasks interval has expired and can used to perform the task's work.  A context pointer is passed to the handler providing a mechanism for providing data to the handler function.

```c
// Task Definition
SCHED_TASK_DEF(my_task);

// The custom handler function for the task.
static void my_task_handler(void *p_context) {

    // Code to execute on task timer expiration.

}
```
Each task needs to be configured with the `sched_task_config()` function before starting it with the `sched_task_start()` function. 

```c
// Configure the task to repeat every 100 mS.
sched_task_config(&my_task, my_task_handler, NULL, 100, true);

// Add the task to the schedulers task que.
sched_task_start(&my_task);
```

## Porting to a New Platform
 
Several [example projects](./examples/README.md) are provided in the  `/examples/` folder for various hardware platforms.  If your processor doesn't match one of the example projects, you will need to implement the functions defined in [scheduler_port.h](./src/scheduler/scheduler_port.h) which are marked as mandatory.  See the [Platform Port](./docs/port.md) documentation for more detailed information.