# emsched (Embedded Scheduler)

# What is `emsched`?

Emsched is a lightweight embedded C language scheduler module. The module provides a simple method for scheduling tasks to be executed in the future without the complexity or overhead of a full RTOS.  A task's  handler is called from the main context once its timer has expired.  Tasks can be configured as repeating, in which case they will be executed periodically until stopped, or as one-shot tasks.  

# Features

The scheduler module has minimal RAM and ROM requirements making it well suited for typical embedded platforms.

* The core scheduler module requires ~500 bytes of ROM.
* Each scheduler task requires 20 bytes of RAM on platforms which utilize 32-bit pointers.

The scheduler's task que is implemented as a single linked list.  Each task contains a reference to the next task in the que.  Tasks are statically allocated utilizing a convenience macro.  This results in the scheduler having a fixed compile time memory footprint avoiding the overhead and risk associated with dynamic memory allocation.  

In addition to scheduling tasks to be executed in the future, the scheduler provides an easy to use mechanism for moving work out of hardware triggered ISR's and into the main context.  This encourages the good practice of writing lightweight interrupt handlers which can improve system responsiveness and stability.

The scheduler is simple to learn and easy to use  Configuring a new task only requires a few lines of code.  Porting to new platform only requires implement 3 platform specific functions.

The scheduler was architected with an eye towards power reduction.  

# Comparison to RTOS
                                                            
The scheduler does not provide all of the same features that Real Time Operating System does, the major differences include:

* Scheduled tasks are are executed in the order in which they are added to the scheduler's que, no task prioritization functionality is provided.  

* A task handler executes until completion, once started, in a cooperative multitasking manner and are only interrupted by a hardware IRQ event.
                                                                            
* It's typical to have several milliseconds of jitter in task execution times for a system with several active tasks queued at the same time.  This jitter is often acceptable for UI tasks such as blinking an LED, debouncing a switch or timing the length of music note.  
    
* Repetitive tasks, which require finer grain control or more deterministic behavior, can be implemented using a separate hardware timer.  For example, an application could implement a real-time motion control loop with a 100 Hz hardware timer and perform UI tasks with the scheduler.  The performance of such a system will often exceed that of an full RTOS implementation due to the minimal scheduler overhead. 
                                                        
Many embedded systems don't require hard real time performance and can live within the schedulers constraints saving the overhead and complexity required by full RTOS.    

