#include <assert.h>
#include "scheduler.h"
#include "scheduler_config.h"

/* The scheduler's interal data structure.
*/
typedef struct
{            
  sched_task_t   * p_head;   // Pointer to the head task in the linked list
  sched_task_t   * p_tail;   // Pointer to the tail task in the linked list
} scheduler_t;

// The scheduler module's internal data.
static scheduler_t scheduler;           

void sched_init(void) {
  // The only required initialization is to set the head and tail tasks to NULL.
  scheduler.p_head    = NULL;
  scheduler.p_tail    = NULL;
}

void sched_task_config(sched_task_t    * p_task, 
                       sched_handler_t   handler, 
                       void            * p_context,
                       uint32_t          period_ms,
                       bool              repeat) {

  // A pointer to the task to be added the schedueler que must be supplied.
  assert(p_task != NULL);

  // The task's hander must be supplied.
  assert(handler != NULL);

  // Mark the task as inactive.
  p_task->active = false;      
  // Set the task handler.          
  p_task->handler = (sched_handler_t) handler;   
  // Save the user context.                               
  p_task->p_context = (void *) p_context;      
  // Limit the period.                                  
  p_task->period_ms = period_ms & SCHEDULER_MS_MAX;  
  // Store the repeating status.
  p_task->repeat = (bool) repeat;  

  // Add the task to the scheduler's que if not previously added
  if(!p_task->added) {

    // The new task will be the last one in the list so it's next task will always be NULL.
    p_task->p_next = NULL;

    // Take exclusive write access to scheduler's task linked list.
    SCHEDULER_CRITICAL_REGION_ENTER();

    if(scheduler.p_head == NULL) {
      // No other tasks exists in the list so the new task will be both the head & tail task.
      scheduler.p_head = p_task;
    } else {
      // If other tasks exist, this task will be the next task for the current tail task.
      assert(scheduler.p_tail != NULL);
      scheduler.p_tail->p_next = p_task;
    }
    // Set the new task to the tail task to add it to the end of the list.
    scheduler.p_tail = p_task;

    p_task->added = true;
    p_task->pending = false;

    // Release task que exclusive access.
    SCHEDULER_CRITICAL_REGION_EXIT();
  }
}

void sched_task_start(sched_task_t * p_task) {

  // A pointer to the task to start must be supplied.
  assert(p_task != NULL);

  // The task must have been previously added to the que.
  assert(p_task->added);

  if(scheduler_tick_active()) {
    // Update the task's expire tick if scheduler's timer is running
    p_task->expire_tick = p_task->period_ms + scheduler_get_tick();
    p_task->pending = false;
  } else {
    // Mark the task as pending so the expire_tick is calculated once the 
    // scheduler's timer is enabled.  This could happen if the task is added 
    // from an Wake Up interrupt routine while the timer is Stopped.
    p_task->pending = true;
  }
  
  // Mark the task as active
  p_task->active = true;

}

void sched_task_update(sched_task_t * p_task, uint32_t period_ms) {

  // A pointer to the task to update must be supplied.
  assert(p_task != NULL);

  // Store the new period.
  p_task->period_ms = period_ms;
  
  // start the task
  sched_task_start(p_task);   
}

void sched_task_stop(sched_task_t * p_task) {

  // A pointer to the task to start must be supplied.
  assert(p_task != NULL);

  // Set the task as inactive but don't remove it from the que.
  p_task->active = false;

}

// Function for checking if a task's timer has expired
static bool sched_task_expired(sched_task_t * p_task) {
  
  // TODO
  return (scheduler_get_tick() - p_task->expire_tick) < 0x80000000;
}


// Function for finding the next expiring scheduler task
static sched_task_t * sched_next_evt(void) {

  // Pointer to the next expiring task.
  sched_task_t * p_next_expiring_evt = NULL;  

  /* Loop through each task in the linked list starting at the head task
   * and ending at the tail task.  task access is read only so it's not 
   * necessary to aquire exclusive accesss here.
   */
  sched_task_t * p_current_evt  = scheduler.p_head;         
  
  while(p_current_evt != NULL) {
    if(p_current_evt->active == true) {
      if(p_next_expiring_evt == NULL) {
        // No expiring task was previously stored so this task is the next expiring task
        p_next_expiring_evt = p_current_evt;
      } else if(p_current_evt->expire_tick < p_next_expiring_evt->expire_tick) {
        //TODO does this correctly handle the SysTick roll over or do we need to compare each time subtracked from SysTick???
        // The current task expires sooner than the previous task so set it as the next expiring task
        p_next_expiring_evt = p_current_evt;
      } 
    }

    // Move to the next task in the list
    p_current_evt = p_current_evt->p_next;
  }

  return p_next_expiring_evt;
}

sched_task_t * sched_execute(void)
{
  // The scheduler's timer must be active in order to correctly calculate the 
  // task expiration times.
  assert(scheduler_tick_active());

  // Loop through each task in the linked list starting with the head task
  sched_task_t * p_current_evt  = scheduler.p_head;         

  while(p_current_evt != NULL) {

    // Store pointer to next task since the current task may be removed
    // if non-repeating and expired or removed in the task handler
    sched_task_t * p_next_evt  = p_current_evt->p_next;
    
    // tasks should have alredy been marked as added to the list
    assert(p_current_evt->added);   

    // Filter on Active tasks
    if(p_current_evt->active) {

      // Update the expire tick if it is marked as pending
      if(p_current_evt->pending) {
        p_current_evt->expire_tick = p_current_evt->period_ms + scheduler_get_tick();
        p_current_evt->pending = false;
      }

      // Handle tasks with expired timers   
      if(sched_task_expired(p_current_evt)) {
        if(p_current_evt->repeat) {
          // Calculate the next expiration ticks to reschedule the task before calling the
          // handler so the handler execution time does not delay the expiration tick calculation.  
          p_current_evt->expire_tick = p_current_evt->period_ms + scheduler_get_tick();

        } else {
          // Mark the non-repeating task as inactive before calling the handler since it may be restarted in the handler.
          p_current_evt->active = false;
        }
        // Execute the task Handler with the user supplied context pointer.
        p_current_evt->handler(p_current_evt->p_context);
      }
    }

    // Move to the next task in the list
    p_current_evt = p_next_evt;
  }

  // The next expiring task needs to be determined after the scheduler has fully executed since an task's
  // active status may be changed after its handler has executed.  An task's active status may have been modified 
  // by another task's handler later in the que.
  return sched_next_evt();    
}


