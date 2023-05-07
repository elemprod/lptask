#  Task State & Access Control


## Overview

Each task tracks its current state with a state variable.  The state tracking is primarily used to control access to the task's configuration and data.  This mechanism protects a task from being modified in a way which could corrupt it. 

## Task States

Each task can be in one of the following states at any given time:

* SCHED_TASK_UNINIT: The task has not be initialized yet.  The state is primarily used to track if the task has been added to the scheduler's task que.  Once the configuration function has been called, a task does not return to this state until the scheduler is stopped.

* SCHED_TASK_STOPPED: The task has been added to the task que but the task is not currently active.  The task moves to the active state once the start function is called.

* SCHED_TASK_ACTIVE: The task has been started.  The task's handler function will be called once its timer interval expires but the handler is not currently executing.

* SCHED_TASK_EXECUTING: The task timer expired and its handler function is currently executing.  

* SCHED_TASK_STOPPING: The task's handler is currently executing and the task will be stopped once the handler returns.  A non-repeating task will be placed into this state during handler execution.  A task will also enter this state if the `sched_task_stop()` function is called and it can not stop immediately.

## Interrupts

A non-preemptive cooperative task scheduler significantly reduces the challenge of implementing task access control versus a more complex preemptive multitasking system.  Only one task handler will ever be active at any given time. Once started, each task handler runs until completion and the task handler's execution can only be suspended due to an interrupt or other exception event.  The flow charts below shows the scheduler's program flow for interrupt events during the different scheduler states.

### Interrupt from Sleep
<img src="./img/sleep_int.svg" align="left" hspace="15" vspace="15" alt="Sleep Interrupt">

The diagram shows the processor being woken from sleep by an interrupt event.  Once the ISR associated with the interrupt completes, the scheduler checks the cached next-expiring task for expiration.  After handling any expired tasks, the scheduler puts the processor back to sleep.
<br clear="left"/>

### Interrupt Inside a Task Handler
<img src="./img/handler_int.svg" align="left" hspace="15" vspace="15" alt="Handler Interrupt">

The diagram shows an interrupt or exception event happening during task handler execution.  The ISR can not modify the data for a task which is currently executing its scheduler without risk of corrupting the handler's operation. 

<br clear="left"/>

### Nested Interrupts
<img src="./img/handler_int_nested.svg" align="left" hspace="15" vspace="15" alt="Handler Interrupt Nested">

The flow chart shows a second interrupt event happening while the processor is already executing an ISR as a result of previous interrupt. This can only happen on a platform which supports nested preemptive interrupts which many modern processors do.  A nested interrupt has the same access restrictions as a single interrupt.
<br clear="left"/>

Note that an interrupt is just one of the several types of exceptions which a particular platform may support.   A variety of different hardware events including memory access, timer expiration, reset and hardware interrupts can generate exceptions.  Each exception type typically has its own handler which is triggered after the exception event happens.  In the case of interrupts, the exception handler is known as an ISR (Interrupt Service Routine).  The flow charts above only show interrupt events impeding the task handler flow but any exception types can do the same.    Interrupts are shown here because ISR's are generally user level code and exception handlers are generally system level code. Scheduler tasks would typically only be accessed inside an ISR and not in a system level exception handler. 

## Access Control by State

Access to a task is limited by the task's current state as summarized in the table below.  Although only one task's handler ever runs at any given time, an interrupt can both suspend a task handler's execution and also wake the processor from sleep.  The access protection mechanism prevents the task from being modified in a way which might corrupt the task or its data.   For example, setting an active task's data pointer to NULL inside of an ISR could lead to unpredictable task handler operation.

| Task State           |Task Config | Task Start | Task Stop | Task Interval Update | Task Data Update |
| :----                |   :----:   |   :----:   |  :----:   |     :----:           |    :----:        |
| SCHED_TASK_UNINIT    | &#9726;    |            |           |                      |                  | 
| SCHED_TASK_STOPPED   | &#9726;    | &#9726;    | &#9726;   | &#9726;              | &#9726;          | 
| SCHED_TASK_ACTIVE    |            | &#9726;    | &#9726;   | &#9726;              |                  | 
| SCHED_TASK_EXECUTING |            | &#9726;    | &#9726;   | &#9726;              |                  | 
| SCHED_TASK_STOPPING  |            | &#9726;    | &#9726;   | &#9726;              |                  | 


## Access Control Rationale

The following list describes reasoning for the access control restrictions for each state.

* SCHED_TASK_UNINIT:
    * A task must be initialized with `sched_task_config()` function prior to use. 
    * Any other function calls on uninitialized tasks will fail.

* SCHED_TASK_STOPPED:  
    * No access protection is required while the task is stopped.
    * This is the only state during which the task data can  be updated.  This protects against task data from modification from within an ISR while the task's handler is executing.

* SCHED_TASK_ACTIVE:  
    * Although it would technically be possible to update a task configuration with the `sched_task_config()` function in the SCHED_TASK_ACTIVE state, it was decided to explicitly disallow it.  Doing so simplifies the configuration rule to a task must be stopped or uninitialized before its configuration can be modified.
    * Calls to the interval set function, on a currently active task, stop the task prior to updating it.    
    * Starting a task that's already in the SCHED_TASK_ACTIVE state updates the task's start time but have no other effect since the task is already active.
    
* SCHED_TASK_EXECUTING & SCHED_TASK_STOPPING:  
    * The executing and stopping states require the most access control restrictions.  
    * The task's scheduler function has been called but has not returned yet in both of these states.  Changing the task data could conflict with the work being performed by the handler function.
    * Although it may be somewhat unintuitive at first consideration, a task's interval can safely be updated in both of these states.   A task is checked for expiration just prior to entering the SCHED_TASK_EXECUTING.  The task interval is not accessed by the scheduler while in the SCHED_TASK_EXECUTING or SCHED_TASK_STOPPING states and can therefore be safely be updated.
    * A call to the stop function on a currently executing task does not happen immediately.  The task's handler function must return before the task can stop.  The stop request is recorded by the task moving the into the SCHED_TASK_STOPPING state. The task will then move to the SCHED_TASK_STOPPED state once the handler returns.
    * A non-repeating task will be placed into the SCHED_TASK_STOPPING state prior to calling the task handler function to indicate that it will be stopped once the handler function returns.  This gives the user the opportunity to restart the task inside of its handler if desired.


