#include <assert.h>
#include <string.h>
#include "scheduler.h"
#include "scheduler_int.h"
#include "scheduler_port.h"

/**
 * Scheduler State Definitions
 */
typedef enum {
  SCHED_STATE_STOPPED = 0, // The scheduler is stopped.
  SCHED_STATE_ACTIVE,      // The scheduler is running.
  SCHED_STATE_STOPPING,    // The scheduler is in the process of stopping.
} sched_state_t;

/**
 * The scheduler's internal data structure.
 *
 * The pointer to the head task gives the scheduler a starting point when
 * traversing the task linked list.
 *
 * The pointer to the tail task provides the scheduler with a reference to
 * the last  task in the linked list.  New tasks are always added to the end
 * of the list.
 *
 * The next expiring task is cached which avoids having to recalculate it
 * during each sched_execute() call which improves efficiency.
 *
 * The que must be locked prior to modifying any of these pointers.
 */
typedef struct
{
  sched_task_t *p_head;         // Pointer to the head task in the task que.
  sched_task_t *p_tail;         // Pointer to the tail task in the task que.
  sched_task_t *p_next;         // Pointer to the next expiring task if known.
  volatile sched_state_t state; // The module's current state.
} scheduler_t;

// The module's internal data.
static scheduler_t scheduler = {
    .p_head = NULL,
    .p_tail = NULL,
    .p_next = NULL,
    .state = SCHED_STATE_STOPPED};

/**
 * Function for removing all tasks from the scheduler's que.
 */
static void sched_clear_que() {

  // Get exclusive que access.
  scheduler_port_que_lock();

  // Walk through the task list starting at the head.
  sched_task_t *p_current_task = scheduler.p_head;

  while (p_current_task != NULL) {
    // Set each task as uninitialized.
    p_current_task->state = TASK_STATE_UNINIT;

    // Move to the next task in the linked list
    p_current_task = p_current_task->p_next;
  }

  // Clear the task references.
  scheduler.p_head = NULL;
  scheduler.p_tail = NULL;
  scheduler.p_next = NULL;

  // Release the que lock.
  scheduler_port_que_free();
}

/**
 * Function for completing a scheduler stop.
 */
static void sched_stop_finalize(void) {
  if (scheduler.state == SCHED_STATE_STOPPING) {

    // Clear the que.
    sched_clear_que();

    // Perform any platform specific deinitialization last.
    scheduler_port_deinit();

    scheduler.state = SCHED_STATE_STOPPED;
  }
}

void sched_stop(void) {
  // Move to the stopping state if not already stopped.
  if (scheduler.state != SCHED_STATE_STOPPED) {
    scheduler.state = SCHED_STATE_STOPPING;
  }
}

bool sched_task_config(sched_task_t *p_task, sched_handler_t handler,
    uint32_t interval_ms, bool repeat) {

  // A pointer to the task must be supplied.
  if(p_task == NULL) {
    return false;
  }

  // A task handler must be supplied.
  if(handler == NULL) {
    return false;
  }

  if ((p_task->state == TASK_STATE_EXECUTING) || (p_task->state == TASK_STATE_STOPPING)) {
    // A task can not be configured while the handler is currently executing.
    return false;
  } else if (p_task->state == TASK_STATE_UNINIT) {
    // Add the task to the scheduler's que if it hasn't been previously added.

    // The new task will be the last one in the list so it's next task will always be NULL.
    p_task->p_next = NULL;

    // Take exclusive write access of scheduler's task que.
    scheduler_port_que_lock();

    if (scheduler.p_head == NULL) {
      // No other tasks exists in the list so the new task will be both the head & tail tasks.
      scheduler.p_head = p_task;
    } else {
      // If other tasks exist, this task will be the next task for the current tail task.
      assert(scheduler.p_tail != NULL);
      scheduler.p_tail->p_next = p_task;
    }
    // Set the new task to the tail task.  This adds it to the end of the list.
    scheduler.p_tail = p_task;

    // Release the task que exclusive access.
    scheduler_port_que_free();
  }

  // Store the task handler.
  p_task->p_handler = handler;

  // Store the task interval limiting it to the max interval.
  p_task->interval_ms = (interval_ms & SCHED_MS_MAX);

  // Store the repeating status.
  p_task->repeat = repeat;

  // Tasks are always in the stopped state after configuration. 
  p_task->state = TASK_STATE_STOPPED;

  return false;
}

bool sched_task_start(sched_task_t *p_task) {

  // A pointer to the task must be supplied.
  if(p_task == NULL) {
    return false;
  }

  if(p_task->state == TASK_STATE_UNINIT) {
    // A task must be configured before it can be started.
    return false;
  } else if(p_task->state == TASK_STATE_STOPPED) {
    // Set the task to active if it is currently stopped.
    p_task->state = TASK_STATE_ACTIVE;
  } else if(p_task->state == TASK_STATE_STOPPING) {
    /* Set the task to executing if is currently stopping.
     * This could happen if the task is started inside an
     * ISR while executing it's handler or if the task is
     * restarted in the handler.
     */
    p_task->state = TASK_STATE_EXECUTING;
  }

  // Store the start time as now.
  p_task->start_ms = scheduler_port_ms();
  
  /* Set the cached next task to the newly started task if it expires 
   * sooner than the currently cached next task.
   */
  scheduler.p_next =  sched_task_compare(scheduler.p_next, p_task);

  return true;
}

bool sched_task_update(sched_task_t *p_task, uint32_t interval_ms) {

  // A pointer to the task must be supplied.
  if(p_task == NULL) {
    return false;
  }
  // Store the new interval limiting it to the max interval.
  p_task->interval_ms = (interval_ms & SCHED_MS_MAX);

  // Start the task. 
  return sched_task_start(p_task);
}


uint8_t sched_task_data(sched_task_t * p_task, void * p_data, uint8_t data_size) {

   // Data can only be set for stopped tasks.
  if((p_task == NULL) || (p_task->state != TASK_STATE_STOPPED)) {
     return 0; 
  }

  // Store the data size.
  p_task->data_size = data_size;

  if(TASK_BUFFERED(p_task)) {

    // Limit the data size to the task buffer size.
    if(p_task->data_size > p_task->buff_size) {
      p_task->data_size = p_task->buff_size;
    }

    if(p_data == NULL) {
      p_task->data_size = 0;
    } else {
      // Copy the data into the task data buffer.
      memcpy(p_task->p_data, p_data, data_size);
    }
  } else {
    // Just set the data pointer for an unbuffered task.
    p_task->p_data = p_data;
  }

  // Return the data size.
  return p_task->data_size;
}

bool sched_task_stop(sched_task_t *p_task) {

  // A pointer to the task must be supplied.
  if(p_task == NULL) {
    return false;
  }

  if(p_task->state == TASK_STATE_UNINIT) {

    // A task must have been previously initialized.
    return false;

  } else if(p_task->state == TASK_STATE_ACTIVE) {

    // Active task can move to the stopped state immediately.
    p_task->state = TASK_STATE_STOPPED;

    // A task is no longer allocated once stopped.
    p_task->allocated = false;

  } else if(p_task->state == TASK_STATE_EXECUTING) {
    /* Executing tasks move to the stopping state since their 
     * handlers are currently executing. The stop will complete
     * after the handler returns.
     */
    p_task->state = TASK_STATE_STOPPING;
  }

  /* It's not necessary to clear the cached next task here
   * since the cached next task's state is checked to be active
   * before it is executed.  
   */
  return true;
}

bool sched_task_expired(sched_task_t *p_task) {

  if(TASK_ACTIVE_SAFE(p_task)) {
    return (scheduler_port_ms() - p_task->start_ms) >= p_task->interval_ms;
  } else {
    return false;
  }
}

uint32_t sched_task_remaining_ms(sched_task_t *p_task) {

  if(TASK_ACTIVE_SAFE(p_task)) {
    uint32_t elapsed_ms = scheduler_port_ms() - p_task->start_ms;

    if (elapsed_ms < p_task->interval_ms) {
      return p_task->interval_ms - elapsed_ms;
    } else {
      // Expired
      return 0;
    }
  } else {
    return SCHED_MS_MAX;
  }
}

uint32_t sched_task_elapsed_ms(sched_task_t *p_task) {

  if(TASK_ACTIVE_SAFE(p_task)) {
    return scheduler_port_ms() - p_task->start_ms;
  } else {
    return 0;
  }
}

sched_task_t *sched_task_compare(sched_task_t *p_task_a, sched_task_t *p_task_b) {

  if (TASK_ACTIVE_SAFE(p_task_a)) {
    if(TASK_ACTIVE_SAFE(p_task_b)) {
      // Both tasks are active, compare the remaining time.
      if (sched_task_remaining_ms(p_task_a) <= sched_task_remaining_ms(p_task_b)) {
        return p_task_a;
      } else {
        return p_task_b;
      }
    } else {
      // Only Task A is active.
      return p_task_a;
    }
  } else if(TASK_ACTIVE_SAFE(p_task_b)) {
      // Only Task B is active
      return p_task_b;
  } else {
    // Neither task is active.
    return NULL;
  }
}

/*
 * Function for updating the cached next task.
 *
 * @return none
 */
static void sched_next_task(void) {

  // Pointer to the next expiring task.
  sched_task_t *p_expiring_task = NULL;

  /* Walk through each task in the linked list starting at the 
  * head task and ending at the tail task.
   */
  sched_task_t *p_current_task = scheduler.p_head;

  while (p_current_task != NULL) {
    /* Only need to test for active state and not the executing state here since no tasks will 
     * be executing when the function is called.
     */
    if (p_current_task->state == TASK_STATE_ACTIVE) {
      if (p_expiring_task == NULL) {
        // No expiring task was previously set so the current task is the next expiring task.
        p_expiring_task = p_current_task;
      } else if (sched_task_remaining_ms(p_expiring_task) > sched_task_remaining_ms(p_current_task)) {
        p_expiring_task = p_current_task;
        // TODO this could be done more effiecently.  We could save the previous expiring time so it doesn't have to be recalculated
        // during each iteration.
        // The function null and active checks which are also repetive
      }
    }

    // Move to the next task in the list
    p_current_task = p_current_task->p_next;
  }

  // Update the cached expiring task.
  scheduler.p_next = p_expiring_task;
}

/**
 * Function for executing scheduled tasks in the que which have expired timers.
 *
 * This function must be called from within the main loop. It will execute
 * all scheduled tasks having expired timers before returning.
 *
 * @return  none
 */
void sched_execute_que(void) {

  /* Check if the cached next task is both valid and unexpired
   * before executing the task que. This check enables the 
   * scheduler to avoid testing every task in the que 
   * for for expiration if the next expiring task is already 
   * known.
   *
   * The pointer ot the next task has to be accessed carefully.
   * An ISR could be called during the task expiration
   * check. The ISR might invalidate the cached next task 
   * which could lead to unpredictable results.  The scheduler 
   * protects against this possibility by using the following 
   * steps:
   * 
   *  1. The next task pointer is copied.
   *  2. The active and expiration check is performed on the copy.  
   *  3. The next task pointer is compared to local copy that was 
   *  made to test if it was modified during the expiration check.
   * 
   * This technique protects against a possible changes to the 
   * cached next task without having to use the scheduler lock.
   */
   sched_task_t * p_next_copy = scheduler.p_next;
   if ((p_next_copy != NULL) &&
      (p_next_copy->state == TASK_STATE_ACTIVE) &&
      !TASK_EXPIRED(p_next_copy) &&
      (p_next_copy == scheduler.p_next)) {
    //  Return if the cached next task is valid but not expired yet.      
    return;
  }
  
  // Loop through each task in the linked list starting with the head task
  sched_task_t *p_current_task = scheduler.p_head;

  while (p_current_task != NULL) {

    // Filter on active tasks with expired timers.
    if ((p_current_task->state == TASK_STATE_ACTIVE) && TASK_EXPIRED(p_current_task)) {

      if(p_current_task->repeat) {
        // A repeating task will be in the executing state while inside of its handler.
        p_current_task->state = TASK_STATE_EXECUTING;

       /* Update the start time before calling the handler so the handler's execution 
        * time doesn't introduce error.  The start time only needs to be updated for
        * repeating tasks.
        */
        p_current_task->start_ms = scheduler_port_ms();

      } else {
        // A non-repeating task will be in the stopping state while executing its handler.
        p_current_task->state = TASK_STATE_STOPPING;
      }

      // Call the Task Handler Function.
      sched_handler_t handler = (sched_handler_t) p_current_task->p_handler;
      handler(p_current_task, p_current_task->p_data, p_current_task->data_size);

      // Update the state after the handler returns.
      if(p_current_task->state == TASK_STATE_EXECUTING) {
        // Executing tasks move back to the active state.
        p_current_task->state = TASK_STATE_ACTIVE;
      } else {
        assert(p_current_task->state == TASK_STATE_STOPPING);
        // Stopping tasks move to the stopped state.
        p_current_task->state = TASK_STATE_STOPPED;
        // The task is no longer allocated once stopped.
        p_current_task->allocated = false;
      }
    }

    // Move to the next task in the list
    p_current_task = p_current_task->p_next;
  }

  /* The cached next expiring task always needs to be refreshed here since the 
   * previous expiring task has been serviced.
   */
  sched_next_task();
}

void sched_init(void) {

  // Start the scheduler if not currently running.
  if (scheduler.state == SCHED_STATE_STOPPED) {

    // Perform any platform specific initialization first.
    scheduler_port_init();

    // Clear the task references.
    scheduler.p_head = NULL;
    scheduler.p_tail = NULL;
    scheduler.p_next = NULL;

    scheduler.state = SCHED_STATE_ACTIVE;
  }
}

void sched_start(void) {

  /* Repeatably execute any expired tasks in the schedulers que 
   * until the scheduler is stopped.
   */
  while (scheduler.state == SCHED_STATE_ACTIVE) {

    // Execute any tasks in the que with expired timers.
    sched_execute_que();

    // Calculate the time interval until the next expiring task.
    uint32_t sleep_interval = sched_task_remaining_ms(scheduler.p_next);

    // Sleep until the next task expires using the platform specific sleep method.
    if (sleep_interval > 0) {
      scheduler_port_sleep(sleep_interval);
    }
  }

  // Finish stopping the scheduler before returning.
  sched_stop_finalize();
}

// Internal function for initializing a scheduler task pool.
static void sched_task_pool_init(sched_task_pool_t * const p_pool) {

  assert(p_pool != NULL);
  assert(p_pool->p_data != NULL);
  assert(p_pool->p_tasks != NULL);

  // Clear the array of tasks.
  memset(p_pool->p_tasks, 0x00, p_pool->task_cnt * sizeof(sched_task_t));

  // Pointer to the current task and last task location.
  sched_task_t * p_task_cur = p_pool->p_tasks;
  sched_task_t * p_task_last = p_pool->p_tasks + p_pool->task_cnt;
  
  // Pointer to the current task's data.
  uint8_t * p_data_cur = p_pool->p_data;

  // Configure each task.
  do {
    // Set the task's data pointer to the buffer location.
    p_task_cur->p_data = p_data_cur;

    // Set the task buffer size.
    p_task_cur->data_size = p_pool->buff_size;

    // Increment the task pointer
    p_task_cur ++;

    // Increment the data buffer pointer
    p_data_cur += p_pool->buff_size;

  } while(p_task_cur <= p_task_last);

  p_pool->initialized = true;
  
}


sched_task_t * sched_task_alloc(sched_task_pool_t * const p_pool) {

  if(p_pool == NULL) {
    return NULL;
  }

  // Initialize the pool of buffered tasks if needed.
  if(p_pool->initialized == false) {
    sched_task_pool_init(p_pool);
  }

  // Pointer's to the current and last tasks in the pool.
  sched_task_t * p_task_cur = p_pool->p_tasks;
  sched_task_t * p_task_last = p_pool->p_tasks + p_pool->task_cnt;

  // Search for the first unallocated task.
  do {
    if(!p_task_cur->allocated) {

      // Acquire the scheduler lock so the task can be allocated
      // without interruption.
      scheduler_port_que_lock();

      // Check that the task is still unallocated
      if(p_task_cur->allocated) {

        /* The task allocation status must have been changed 
         * from a different context before the task could be 
         * allocated. Release the lock and continue the search.
         */
        scheduler_port_que_free();

      } else {
      
        // Mark the task as allocated.
        p_task_cur->allocated = true;

        // Release the lock
        scheduler_port_que_free();
      
        // Clear the task's data buffer
  #if SCHED_TASK_BUFF_CLEAR != 0
        memset(p_task_cur->p_data, 0x00, p_task_cur->buff_size);
  #endif
        // Reset the task data size.
        p_task_cur->data_size = 0;
        
        // Return the allocated task.
        return p_task_cur;
      } 
    }
    // Increment the task pointer
    p_task_cur ++;

  } while(p_task_cur <= p_task_last);

  // No unallocated tasks were found.
  return NULL;
}

/**
 * Weak & empty implementations of the optional port functions.
 */
__attribute__((weak)) void scheduler_port_sleep(uint32_t interval_ms) {
  // Empty
}

__attribute__((weak)) void scheduler_port_init(void){
    // Empty
};

__attribute__((weak)) void scheduler_port_deinit(void){
    // Empty
};