# Embedded Scheduler

 The C language cooperative scheduler module provides an easy to use mechanism for scheduling tasks to be executed in the future without the complexity or overhead of a multi-tasking operating system.  Once scheduled, a task's handler is executed from the main context after its interval timer expires.  Tasks can be configured as one-shot or repeating. 

## Major Features

The scheduler offers the following features:

* The core scheduler module consumes ~700 bytes of ROM making it well suited for embedded platforms.
* Each scheduled task requires 20 bytes of RAM on a typical 32-bit processor.
* Static memory allocation provides a fixed compile time memory footprint.
* The scheduler is simple to learn and easy to use, configuring a new task only requires a few lines of code.  
* The scheduler encourages the practice of writing lightweight interrupt handlers which can improve system responsiveness and stability.  ISR work can easily but moved into the main context with minimal overhead.
* The scheduler was architected with an eye towards efficiency and power reduction.  It offers flexible support for platform specific sleep, timer and power reductions mechanisms through optional platform specific function calls.

## Comparison to Multitasking OS
                                                            
The cooperative scheduler does not provide all of the same features which a multitasking OS typically does, the major differences include:

* Expired tasks are are executed in the order in which they are added to the scheduler's que, no task prioritization functionality is provided.  
* A task's handler executes until completion, once started, in a cooperative manner.  Task handlers can only be interrupted by a interrupt events and not by other other tasks..
* It's typical to have several milliseconds of jitter in task execution intervals for a system with multiple active tasks queued at the same time.  This jitter is nearly always acceptable for UI tasks such as blinking an LED, debouncing a switch or timing the length of music note.
* Repetitive tasks, which require finer grain control or more deterministic behavior, can be implemented using a separate hardware timer.  For example, an application could implement a real-time motion control loop with a 100 Hz hardware timer and perform UI tasks with the scheduler. 
                                                        
Many embedded systems don't require hard real time performance and can live within the schedulers constraints saving the overhead and complexity required by full RTOS.    

## Cooperative Scheduler Use Cases

A cooperative scheduler can be used in most simple to meduim complexity embedded systems but really shines fo appllications which have some of the folllowing characteritics.

* Power reduction is a high priority.
* The selected platform is capable of stopping program execution and sleeping in a low power state.
* The processor's is anticipated to be sleeping in the majority of the time. 
* RAM and ROM resources are limited.



## Initializing & Starting the Scheduler

The scheduler must be initialized and started from the main context.  The `sched_start()` function repeatably executes expired tasks sleeping when no tasks are active.  The function does not return until the the scheduler is stopped.  Many implementations will never stop the scheduler unless they need reboot the processor to perform a firmware upgrade or similar task.

```c
int main() {
  // Initialize the Scheduler.
  sched_init();
  
  // Start the Scheduler.
  sched_start();
}
```

## Defining & Configuring a Task

A scheduler task should be defined with the `SCHED_TASK_DEF()` macro.  Each task must also have a task handler defined which follows the `sched_handler_t` function prototype.  The handler is called called once the task's interval has expired and is used to perform the task's work.  The context pointer passed to the handler provides a mechanism for supplying data to the handler function.

```c
// Task Definition
SCHED_TASK_DEF(my_task);

// Custom task handler function .
static void my_task_handler(void *p_context) {

    // Code to perform the task's work on task timer expiration.

}
```
Each task needs to be configured with the `sched_task_config()` function before starting it with `sched_task_start()`. 

```c
// Configure the task to repeat every 100 mS.
sched_task_config(&my_task, my_task_handler, NULL, 100, true);

// Add the task to the schedulers task que.
sched_task_start(&my_task);
```

## Porting to a New Platform
 
Several [example projects](./examples/README.md) are provided in the  `/examples/` folder for various hardware platforms.  If your processor doesn't match one of the example projects, you will need to implement the mandatory functions defined in [scheduler_port.h](./src/scheduler/scheduler_port.h).  See the [Platform Port](./docs/port.md) page for more detailed information on adding support for a new processor.