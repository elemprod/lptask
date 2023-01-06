# emsched (Embedded Scheduler)

EMSched is a lightweight embedded C language scheduler module. The module provides a simple method for scheduling tasks to be executed at later point in time without the complexity or overhead of a full RTOS.  A task's event handler is called from the main context once its timer has expired.  Tasks may be configured as repeating in which case they will be executed periodically until stopped or as one-shot tasks.  

The scheduler module has minimal RAM and ROM requirements make it well suited for typical embedded processors.

* The scheduler module requires 8 bytes of RAM and ~400 bytes of ROM.
* Each scheduler task requires 20 bytes of RAM on platforms which utilize 32-bit pointers.

The task que is implemented as a single linked list.  Each task contains a reference to the next task in the que.  Tasks are statically declared utilizing a convience macro inside of the containing module.  This results in the scheduler having a fixed compile time memory footprint.  

In addition to scheduling tasks to be executed in the future, the scheduler provides a simple mechanism for moving work out of ISR's and into the main context.  The scheduler encourages the development practice of writing lightweight interrupt handlers.
                                                            
The scheduler does not provide all of the sames features that a Real Time Operating System typically does, the major differences include:

* Scheduled tasks are are executed in the order in wich they are added to the scheduler's que, no task prioritization functionality is provided.  

* Tasks executes until completion once started in a cooperative multitasking manner.  Taks execution can only be pausesd by a hardware IRQ event or by the task's event handler returning.
                                                                            
* It's not unusual to have several of milisconds of jitter in task execution for a system with multiple active repeating tasks qued at the same time.  This jitter is typically acceptable for most UI tasks such as blinking an LED, debouncing a switch or timing the length of music note.  
    
* Repeativive tasks which require finer grain control can be implemented using a seperate hardware timer.  For example, an application could peform a real-time motion control loop with a 100 Hz hardware timer and perform UI tasks with the scheduler.  The performance of such a system will often exceed that of an full RTOS implementation due to the minimal scheduler overhead. 
                                                        
Many embedded systems don't require hard real time operation and can live within these constraints saving the overhead and complexity required by full RTOS.    

## Get started

1. Add the repo. into your project.
2. Setup the project's build system to include the scheduler directory and complile scheduler.c by editing the project's configuration in your complier's GUI or updating the make file.
3. Customize the scheduler_config.h header file for the project's architecture.   The header provides the platform specific configuration.
4. 


