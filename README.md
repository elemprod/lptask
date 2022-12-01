# emsched

EMSched is a light weight embedded C language scheduler module. The module provides a simple method for scheduling tasks to be executed at later point in time without the complexity or overhead of an RTOS.  The schedule event handlers are called from the main context.  Events may be configured as repeating in which case they will be executed periodically until stopped or as one-shot events.  

The scheduler module has minimal RAM and ROM requirements make it well suited for typical embedded processors.

    - The scheduler module requires 8 bytes of RAM and aproximately 400 bytes of ROM.
    - Each scheduler event requires 20 bytes of RAM on platforms which utilize 32-bit pointers.

The event que is implemented as a single linked list.  Each event contains a reference to the next event in the que.  Each event is statically declared utilizing a convience macro inside of the containing module.  This results in the scheduler having a fixed memory footprint.  

In addition to scheduling events to be executed in the future, the scheduler provides a simple mechanism for moving work out of ISR's and into the main context.  This encourages the coding practice of writing light weight interrupt handlers.
                                                            
The scheduler does not provide all of the sames features as Real Time Operating typical does.
    
    - Scheduled events are are executed in the order they are added to the scheduler's que, no event prioritization functionality is provided.  

    - Events executes until completion once started in a cooperative multitasking manner.  Event execution can only be pausesd by a hardware IRQ event or by the event handler returning.
    
    - Many embedded systems outside of motion and process control don't require hard real time operation and therefore can live within these constraints saving overhead a full RTOS.                             
                                                                                
    - It's not unusual to have several of milisconds of jitter in task execution for a system with multiple active repeating tasks qued at the same time.  This jitter is typically acceptable for most UI tasks suck as blinking an LED, debouncing a switch or timing the length of music note.  
    
    - Repeativive tasks which require finer grain control can be implemented using a seperate hardware timer in addition to the scheduler.  For example, an application could peform a real-time motion control loop from a 50 Hz hardware timer and perform UI tasks with the scheduler.  The performance of such a system would typically exceed that of an RTOS implementation due to the decreased scheduler overhead. 
                                                        

