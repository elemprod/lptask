#  Task State & Access Control


## Overview

Each task includes an internal variable to track the task's current state.  The state tracking is primarily used to control access to the task's configuration and data.  This mechanism is required to prevent the task from being modified in a way which could corrupt its operation.  The state is variable is declared as volatile since it can be updated from an ISR context.

## Task States

A task can be in one of the following states at any given time:

* TASK_STATE_UNINIT: The task has not be initialized yet.  The state is primarily used to track if the task has been added to the Scheduler's task que.  Once the configuration function has been called, a task does not return to this state unless the scheduler is stopped.

* TASK_STATE_STOPPED: The task has been added to the task que but the tasks is not currently active.  The task moves to the active state once the start function is called.

* TASK_STATE_ACTIVE: The task has been started.  The task's handler function will be called once it's timer interval expires but the handler is not currently executing.

* TASK_STATE_EXECUTING: The task timer has expired and it's handler function is currently executing.  

* TASK_STATE_STOPPING: The task's handler is currently executing and the task will be stopped once the handler returns.  A non-repeating task will be placed into this state while its handler is executing.  A repeating task will enter this state if the stop function is called during its handler execution.

## Interrupts

A non-preemptive cooperative task scheduler significantly reduces the challenge of implementing access control versus a more complex preemptive multitasking system.  Only a one task handler can ever be running at any given time. Each task's handler runs until completion once started and the handler execution can only be suspended due to an interrupt or other exception event.  The flow charts below show the scheduler's program flow for interrupt events during different scheduler states.


### Interrupt from Sleep
<img src="./img/sleep_int.svg" align="left" hspace="15" vspace="15" alt="Sleep Interrupt">

The diagram shows the processor being woken from sleep by an interrupt event.  Once the ISR completes, the scheduler operation checks for expired tasks.  If there are no expired tasks, the handler puts the processor back to sleep
<br clear="left"/>

### Interrupt Inside a Task Handler
<img src="./img/handler_int.svg" align="left" hspace="15" vspace="15" alt="Handler Interrupt">

The diagram shows an interrupt / exception event happening during task handler execution.  The ISR can not modify the task data without risk of corrupting with the handler's operation. 
<br clear="left"/>

### Nested Interrupts
<img src="./img/handler_int_nested.svg" align="left" hspace="15" vspace="15" alt="Handler Interrupt Nested">

The diagram shows an interrupt event happening while the processor is executing an ISR as a result of a previous interrupt. This can only happen on a platform which supports nested interrupts which most modern processors do.  A nested interrupt has the same access restrictions are single interrupt.
<br clear="left"/>

Note that an interrupt is just one of the several types of exceptions which a particular platform may support.   A variety of different events including memory access, timer expiration, reset and hardware interrupts can be generate exceptions.  Each exception type typically has it's own handler which is triggered once the exception event happens.  In the case of interrupts, the exception handler is known as an ISR (Interrupt Service Routine).  The flow charts above only shows interrupt events impeding the task handler flow but a other exception types can also do the same.    ISR's are generally user level code and the other exception handler are generally system level code. Interrupts are used in the diagrams above because a scheduler task would typically only be accessed inside an ISR and not in the system level exception handlers. 

## Access Control by State

Access to a task is limited by the task's current state as summarized in the table below.  Although only one task's handler ever runs at any given time, an interrupt can both suspend a task handler's execution and also wake the processor from sleep.  The access protection mechanism prevents the task from being modified in way which might corrupt the task or its data.   

| Task State           |Task Config | Task Start | Task Stop | Task Interval Update | Task Data Update |
| :----                |   :----:   |   :----:   |  :----:   |     :----:           |    :----:        |
| TASK_STATE_UNINIT    | &#x26AB;   |            |           |                      |                  | 
| TASK_STATE_STOPPED   | &#x26AB;   | &#x26AB;   | &#x26AB;  | &#x26AB;             | &#x26AB;         | 
| TASK_STATE_ACTIVE    | &#x26AB;   | &#x26AB;   | &#x26AB;  | &#x26AB;             |                  | 
| TASK_STATE_EXECUTING |            | &#x26AB;   | &#x26AB;  | &#x26AB;             |                  | 
| TASK_STATE_STOPPING  |            | &#x26AB;   | &#x26AB;  | &#x26AB;             |                  | 


## State Access Notes

* TASK_STATE_UNINIT:
    * A task must be initialized prior to use so only the config function is available in the uninitialized state.
    * All other function calls on uninitialized tasks are ignored.

* TASK_STATE_STOPPED:  
    * No access protection is required if the task is stopped.
    * This is the only state during which the task data can be updated.

* TASK_STATE_ACTIVE:  

    * Calls to the task config and interval set functions on a currently active task stop the task.    
    * Starting a task that's already in the TASK_STATE_ACTIVE state updates the task's start time but has no other effect since the task is already active.
    
* TASK_STATE_EXECUTING & TASK_STATE_STOPPING:  
    * The executing and stopping states require the most access control restrictions.  
    * The task's scheduler function has been called but has not returned yet in these states.  Any changes to the tasks data inside an ISR could conflict with the work being performed by the handler function.
    * Although it may be somewhat unintuitive on first consideration, the task's interval can safely be updated in either of these states.   A task's interval is checked for expiration just prior to entering the TASK_STATE_EXECUTING state and the subsequent task handler function call so there is no potential for a task interval access conflicts.
    * A call to the stop function on a currently executing task does not happen immediately.  The task's handler function must return before the task can stop.  The stop request is recorded by moving from the TASK_STATE_EXECUTING to the TASK_STATE_STOPPING state.  The task will move to the TASK_STATE_STOPPED state once the handler returns.
    * A non-repeating task will be placed into the TASK_STATE_STOPPING state prior to calling the task handler function to record that they will be stopped once the handler function returns which gives the user the opportunity to restart the task inside the handler.

## User Prospective

From the users prospective, task access can be summarized by 3 basic rules:

1. A task must be defined and configured before it can be used.
2. A task's configuration can only be modified if the task is stopped or while task's handler is not executing.
3. A task's data can only be modified while it is stopped.  

## Task Data Update

While calling the the sched_task_data() function is limited to TASK_STATE_STOPPED state, it is worth noting that the data stored by the task could potentially be updated in any state except for TASK_STATE_UNINIT.  The task data pointer has not been assigned in the TASK_STATE_UNINIT state.

A repeating task might utilize a data structure to track it's current state.  For example, a task which produces a series of LED patterns might store the current LED pattern index in the task buffer.  The handler could cycle through LED pattern indexes at each call and output the current pattern.  In ths case, the value of the index stored in the buffer can be referenced from the task data buffer pointer supplied to the handler and updated at each handler called.  The values stored in the data buffer can be accessed without the sched_task_data() function.

In practice, the data buffer values should only be modified by the task handler for buffered tasks. Modifying the data buffer in other context's could lead to access conflicts and should be done with care.


