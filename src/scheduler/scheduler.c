#include "scheduler.h"
#include "scheduler_port.h"
#include <assert.h>


typedef enum {
  SCHED_STATE_STOPPED  = 0,   // The module is stopped.
  SCHED_STATE_WAIT,           // The module is waiting for the next task to expire.
  SCHED_STATE_ACTIVE,         // The scheduler is handling any tasks with expired timers.
  SCHED_STATE_STOPPING,       // The scheduler is in the process of stopping. 
} sched_state_t;


//TODO it would be relatively easy to add support for pausing the scheduler which could be useeful\

/** 
 * The scheduler's interal data structure.
 *
 * The pointer to the head task gives the scheduler a starting point when
 * traversing the task linked list.
 * The tail task pointer provides the scheduler with a reference to the last 
 * task in the linked list.  New tasks are always added to the end of the list.
 */
typedef struct
{
  sched_task_t            * p_head; // Pointer to the head task in the linked list
  sched_task_t            * p_tail; // Pointer to the tail task in the linked list
  volatile sched_state_t    state;  // The module's current state.
} scheduler_t;

// The module's internal data.
static scheduler_t scheduler = {.p_head = NULL, 
                                .p_tail = NULL, 
                                .state = SCHED_STATE_STOPPED};

bool sched_init(void) {
  
  if(scheduler.state == SCHED_STATE_STOPPING) {
    // The scheduler can not be started if it is currently stopping.
    return false;
  }

  // Start the scheduler if currently stopped
  if(scheduler.state == SCHED_STATE_STOPPED) {
    // Perform any platform specific initialization first.
    scheduler_port_init();

    // Clear the head and tail task references.
    scheduler.p_head = NULL;
    scheduler.p_tail = NULL;

    scheduler.state = SCHED_STATE_WAIT;
  }

  return true;
}

/**
 * Function for removing all tasks from the scheduler's que.
 */
static void sched_clear_que() {

  // Get exclusive que access.
  scheduler_port_que_lock();
  
  // Walk through the task list starting at the head.
  sched_task_t * p_current_task = scheduler.p_head;

  while (p_current_task != NULL) {

    // Mark each task as inactive and not added to the que.
    p_current_task->active = false;
    p_current_task->added = false;

    // Move to the next task in the linked list
    p_current_task = p_current_task->p_next;
  }

  // Clear the head and taill references.
  scheduler.p_head = NULL;
  scheduler.p_tail = NULL;

  // Release the que lock.
  scheduler_port_que_free();
}

/**
 * Function for completing a scheduler stop.
 */
static void sched_stop_finalize(void) {
  // Can't stop if the scheduler is currently executing a task handler.
  assert(scheduler.state != SCHED_STATE_ACTIVE);

  // Clear the que.
  sched_clear_que();

  // Perform any platform specific deinitialization last.
  scheduler_port_deinit();

  scheduler.state = SCHED_STATE_STOPPED;
}

bool sched_stop(void) {

  switch (scheduler.state) {

    case SCHED_STATE_STOPPED:
      // Already stopped, nothing to do.
      return true;

    case SCHED_STATE_WAIT:
      // The scheduler isn't executing a task, it can stop immediately.
      scheduler.state = SCHED_STATE_STOPPING;
      sched_stop_finalize();
      return true;
     
    case SCHED_STATE_ACTIVE:
      // The scheduler is currently executing a task
      // Move to the stopping state so the scheduler will be stopped 
      // once the current task completes.
      scheduler.state = SCHED_STATE_STOPPING;
      return false;

    case SCHED_STATE_STOPPING:
      // Already stopping, nothing to do.
      return false;
  }
}


void sched_task_config( sched_task_t * p_task,
                        sched_handler_t handler,
                        void * p_context,
                        uint32_t interval_ms,
                        bool repeat) {

  // A pointer to the task to be added the schedueler que must be supplied.
  assert(p_task != NULL);

  // The task's hander must be supplied.
  assert(handler != NULL);

  // Mark the task as inactive.
  p_task->active = false;
  // Set the task handler.
  p_task->handler = (sched_handler_t)handler;
  // Save the user context.
  p_task->p_context = (void *)p_context;
  // Store the period limiting it to the max interval.
  p_task->interval_ms = (interval_ms & SCHEDULER_MS_MAX);
  // Store the repeating status.
  p_task->repeat = (bool)repeat;

  // Add the task to the scheduler's que if it hasn't been previously added.
  if (!p_task->added) {

    // The new task will be the last one in the list so it's next task will always be NULL.
    p_task->p_next = NULL;

    // Take exclusive write access of scheduler's task que.
    scheduler_port_que_lock();

    if (scheduler.p_head == NULL) {
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

    // Release task que exclusive access.
    scheduler_port_que_free();
  }
}

void sched_task_start(sched_task_t * p_task) {

  // A pointer to the task to start must be supplied.
  assert(p_task != NULL);

  // The task must have been previously added to the que.
  assert(p_task->added);

  // Store the start time.
  p_task->start_ms = scheduler_port_ms();

  // Mark the task as active
  p_task->active = true;
}

void sched_task_update(sched_task_t * p_task, uint32_t interval_ms) {

  // A pointer to the task to update must be supplied.
  assert(p_task != NULL);

  // Store the new period limiting it to the max interval.
  p_task->interval_ms = (interval_ms & SCHEDULER_MS_MAX);

  // start the task
  sched_task_start(p_task);
}

void sched_task_stop(sched_task_t * p_task) {

  // A pointer to the task to start must be supplied.
  assert(p_task != NULL);

  // Set the task as inactive but don't remove it from the que.
  p_task->active = false;
}

bool sched_task_expired(sched_task_t * p_task) {
  assert(p_task != NULL);

  if (p_task->active) {
    return (scheduler_port_ms() - p_task->start_ms) >= p_task->interval_ms;
  } else {
    return false;
  }
}

uint32_t sched_task_remaining_ms(sched_task_t * p_task) {
  assert(p_task != NULL && p_task->active);

  uint32_t elapsed_ms = scheduler_port_ms() - p_task->start_ms;
  if (elapsed_ms < p_task->interval_ms) {
    return p_task->interval_ms - elapsed_ms;
  } else {
    // Expired
    return 0;
  }
}

/*
 * Function for calculating the time since a task's timer was started
 * or restarted in the case of a repeating timer.
 *
 * Note that the task must be active to calculate the elapsed time.
 *
 * @return    The time in mS since the task was started.
 */
uint32_t sched_task_elapsed_ms(sched_task_t * p_task) {
  assert(p_task != NULL && p_task->active);

  return scheduler_port_ms() - p_task->start_ms;
}

/*
 * Function for comparing the time until expiration of two tasks and returning
 * the one which expires sooner.
 *
 * Note that the tasks must be be active for the comparison to be valid.
 *
 * @return    Returns a pointer to the task which expires the first or
 *            NULL if both tasks are inactive.
 */
sched_task_t * sched_task_compare(sched_task_t * p_task_a, sched_task_t * p_task_b) {

  if (p_task_a == NULL || !p_task_a->active) {
    if (p_task_b == NULL || !p_task_b->active) {
      // Both Tasks are Inactive
      return NULL;
    } else {
      // Only Task B is active
      return p_task_b;
    }
  } else if (p_task_b == NULL || !p_task_b->active) {
    // Only Task A is active
    return p_task_a;
  } else {
    // Both tasks are active, compare the remaining time.
    if (sched_task_remaining_ms(p_task_a) <= sched_task_remaining_ms(p_task_b)) {
      return p_task_a;
    } else {
      return p_task_b;
    }
  }
}

/*
 * Function for finding the next expiring scheduler task.
 *
 * @return Pointer to the next expiring task or NULL if there are no active tasks.
 */
static sched_task_t * sched_next_task(void) {

  // Pointer to the next expiring task.
  sched_task_t * p_expiring_task = NULL;

  /* Walk through each task in the linked list starting at the head task
   * and ending at the tail task.  Task access is read only so it's not
   * necessary to aquire exclusive accesss.
   */
  sched_task_t * p_current_task = scheduler.p_head;

  // TODO we could improve this code by caching the time_remaining for the current expiring task.
  // It's currently recalculated every time through the loop.
  while (p_current_task != NULL) {
    if (p_current_task->active == true) {
      if (p_expiring_task == NULL) {
        // No expiring task was previously set, the current task is the current next epiring task.
        p_expiring_task = p_current_task;
      } else if (sched_task_remaining_ms(p_expiring_task) > sched_task_remaining_ms(p_current_task)) {
        p_expiring_task = p_current_task;
      }
    }

    // Move to the next task in the list
    p_current_task = p_current_task->p_next;
  }

  return p_expiring_task;
}

sched_task_t * sched_execute(void) {

  if(scheduler.state == SCHED_STATE_WAIT) {
    // Move to the active state
    scheduler.state = SCHED_STATE_ACTIVE;
  }

  // Loop through each task in the linked list starting with the head task
  sched_task_t * p_current_task = scheduler.p_head;

  // Check the scheduler state each time through the task loop since it may have 
  // changed to stopping if the stop function was called.
  while (p_current_task != NULL && (scheduler.state == SCHED_STATE_ACTIVE)) {

    // Store the pointer to next task since the current task could be modified
    // inside the task's handler.
    sched_task_t * p_next_task = p_current_task->p_next;

    // Filter on active tasks with expired timers.
    if (p_current_task->active && sched_task_expired(p_current_task)) {

      // Update the start time before calling the handler so the
      // handler's execution time doesn't introducing error into the
      // start time calculation.
      p_current_task->start_ms = scheduler_port_ms();

      // Update the active flag before calling the handler to avoid overwriting
      // any active flag changess made inside of the handler.  A task will only
      // be active at this point if its a repeating task.
      p_current_task->active = p_current_task->repeat;

      // Execute the task Handler with the user supplied context.
      p_current_task->handler(p_current_task->p_context);
    }

    // Move to the next task in the list
    p_current_task = p_next_task;
  }
 
   if(scheduler.state == SCHED_STATE_STOPPED) {
    return NULL;
   } else if(scheduler.state == SCHED_STATE_STOPPING) {
    // Finishing stopping the scheduler.
    sched_stop_finalize();
    return NULL;
   } else {
     // Move back to the wait state.
    scheduler.state = SCHED_STATE_WAIT;

    // The next expiring task can only be calculated once the scheduler has fully executed 
    // all of the task handlers in the que since a task's active status and/or its interval
    // may be changed by the event handler.
    return sched_next_task();
  }
}

/**
 * Weak port implementation function definition to make it optional.
 */
__attribute__((weak)) void scheduler_port_init(void) {
  // Empty
};

/**
 * Weak port implementation function definition to make it optional.
 */
__attribute__((weak)) void scheduler_port_deinit(void) {
  // Empty
};
