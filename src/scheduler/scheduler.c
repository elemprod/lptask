/**
 * @file scheduler.c
 * @author Ben Wirz
 */ 

#include <assert.h>
#include <string.h>
#include "scheduler.h"

/***** Scheduler Module Internal Data *****/

/**
 * @brief Scheduler State Type
 */
typedef enum {
  /// @brief The scheduler is stopped.
  SCHED_STATE_STOPPED = 0,
  /// @brief The scheduler is running.  
  SCHED_STATE_ACTIVE,
  /// @brief The scheduler is in the process of stopping.  
  SCHED_STATE_STOPPING
} sched_state_t;

/**
 * @brief The scheduler's internal data structure.
 *
 * @note The scheduler must be locked prior to modifying any of the task
 * pointers.
 */
typedef struct {
  /** 
   * @brief Pointer to the head task in the task que.
   * 
   * The pointer to the head task gives the scheduler a starting point
   * when traversing the task que.
   */   
  sched_task_t *p_head; 
  /**   
   * @brief Pointer to the tail task in the task que.
   * 
   * The pointer to the tail task provides the scheduler with a 
   * reference to the last  task in the task que.  New tasks are always 
   * added to the end of the que.
   */  
  sched_task_t *p_tail;
  /**   
   * @brief Pointer to the next expiring task if known else NULL.
   * 
   * The next expiring task is cached when possible which avoids having 
   * to recalculate it during each sched_execute() call.
   */  
  sched_task_t *p_next;
  /**   
   * @brief The module's current state.
   * 
   * The state variable must be volatile since the scheduler could be stopped 
   * from an interrupt context.
   */ 
  volatile sched_state_t state;
} scheduler_t;

// The scheduler module's internal data.
static scheduler_t scheduler = {
    .p_head = NULL,
    .p_tail = NULL,
    .p_next = NULL,
    .state = SCHED_STATE_STOPPED};

/***** Scheduler Configuration Defines *****/

/**
 * @brief Define to enable clearning the task data buffer.
 * 
 * If SCHED_TASK_BUFF_CLEAR is defined to be != 0, the task data buffer will 
 * be cleared each time the task is allocated.  The default implementation 
 * is to not clear the buffer but the end user can override this by defining
 * SCHED_TASK_BUFF_CLEAR to be 1 if desired.  Clearing a large task data 
 * buffer can be costly and is unnecessary for most applications since the
 * buffer is overwritten when data is added.  Clearing the buffer can be 
 * useful though for certain debugging purposes.
 */
#ifndef SCHED_TASK_BUFF_CLEAR
#define SCHED_TASK_BUFF_CLEAR (0)
#endif

/**
 * @brief Define for the maximum task interval time in mS. 
 * 
 * The define sets the maximum task interval time in mS.  The default value 
 * of UINT32_MAX will be suitable for most applications but the end user 
 * can define a lower value should they desire to limit the maximum interval.
 */ 
#ifndef SCHED_MS_MAX
#define SCHED_MS_MAX (UINT32_MAX)
#endif

/***** Scheduler Task Macro's (Internal Use) *****/    

/**
 * @brief Macro for checking if a task is buffered.
 *
 * @note The task pointer is not NULL checked
 *
 * @param[in] p_task   Pointer to the task.
 * @return             True if the task is buffered.
 *                      False if the task is unbuffered.
 */
#define TASK_BUFFERED(p_task) ((p_task)->buff_size > 0)

/**
 * @brief Macro for safely checking if a task is buffered.
 *
 * @param[in] p_task   Pointer to the task.
 * @return             True if the task is buffered.
 *                      False if the task is unbuffered
 *                      or the task pointer is NULL>
 */
#define TASK_BUFFERED_SAFE(p_task) (((p_task) != NULL) && TASK_BUFFERED(p_task))

/**
 * @brief Macro for checking if a task is active.
 *
 * @note The task pointer is not NULL checked
 *
 * @param[in] p_task   Pointer to the task.
 * @return             True if the task is active else False.
 */
#define TASK_ACTIVE(p_task) (((p_task)->state == SCHED_TASK_ACTIVE) || ((p_task)->state == SCHED_TASK_EXECUTING))

/**
 * @brief Macro for safely checking if a task is active.
 *
 *  @param[in] p_task   Pointer to the task.
 *  @return             True if the task is active.
 *                      False if the task pointer is NULL 
 *                      or the task is inactive.
 */
#define TASK_ACTIVE_SAFE(p_task) (((p_task) != NULL) && TASK_ACTIVE(p_task))

/**
 * @brief Macro for checking if a task has expired.
 *
 * @note: The task pointer is not NULL checked and the task's Active status 
 * is also not checked.
 *
 * @param[in] p_task   Pointer to the task.
 * @return             True if the task is expired else False.
 */
#define TASK_EXPIRED(p_task) ((sched_port_ms() - (p_task)->start_ms) >= (p_task)->interval_ms)

/**
 * @brief Macro for safely checking if a task has expired.
 *
 * @param[in] p_task   Pointer to the task.
 * @return             True if the task is active and expired.
 *                     False if the task is expired, inactive or NULL.
 */
#define TASK_EXPIRED_SAFE(p_task) (TASK_ACTIVE_SAFE(p_task) && TASK_EXPIRED(p_task))

/**
 * @brief Internal function for checking the time since a task was started.
 * 
 * @note: The task pointer is not NULL checked and the task is assumed to be
 * active.
 * 
 * @param[in] p_task  Pointer to the task.
 * @param[in] now_ms  The current time in mS.
 * @return The time since the task was started.
 */
static inline uint32_t task_time_elapsed_ms(const sched_task_t *p_task, uint32_t now_time_ms) {
  return now_time_ms - p_task->start_ms;
}

/**
 * @brief Internal function for checking if a task has expired.
 * 
 * @note: The task pointer is not NULL checked and the task is assumed to be
 * active.
 * 
 * @param[in] p_task        Pointer to the task.
 * @param[in] now_time_ms   The current time in mS.
 * @return  The time until the task expires or 0 if it is already expired.
 */
static inline bool task_time_expired(const sched_task_t *p_task, uint32_t now_time_ms) {
  return (now_time_ms - p_task->start_ms) >= p_task->interval_ms;
}

/**
 * @brief Internal function for calculating the time until a task expires.
 * 
 * @note: The task is assumed to be active.
 * @note: The task pointer is not NULL checked.
 * 
 * @param[in] p_task        Pointer to the task.
 * @param[in] now_time_ms   The current time in mS.
 * @return  The time until the task expires or 0 if it is already expired.
 */
static inline uint32_t task_time_remaining_ms(const sched_task_t *p_task, uint32_t now_time_ms) {
  uint32_t elapsed_ms = now_time_ms - p_task->start_ms;
  if (p_task->interval_ms > elapsed_ms) {
    return p_task->interval_ms - elapsed_ms;
  } else {
    return 0;
  }
}

/***** External Task Helper Functions *****/

bool sched_task_expired(const sched_task_t *p_task) {
  
  if(TASK_ACTIVE_SAFE(p_task)) {
    return (sched_port_ms() - p_task->start_ms) >= p_task->interval_ms;
  } else {
    return false;
  }
}

uint32_t sched_task_remaining_ms(const sched_task_t *p_task) {

  if(TASK_ACTIVE_SAFE(p_task)) {
    uint32_t elapsed_ms = sched_port_ms() - p_task->start_ms;

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

uint32_t sched_task_elapsed_ms(const sched_task_t *p_task) {

  if(TASK_ACTIVE_SAFE(p_task)) {
    return sched_port_ms() - p_task->start_ms;
  } else {
    return 0;
  }
}

sched_task_t *sched_task_compare(const sched_task_t *p_task_a, const sched_task_t *p_task_b) {

  if (TASK_ACTIVE_SAFE(p_task_a)) {
    if(TASK_ACTIVE_SAFE(p_task_b)) {
      // Both tasks are active, compare the remaining time.
      if (sched_task_remaining_ms(p_task_a) <= sched_task_remaining_ms(p_task_b)) {
        return (sched_task_t *) p_task_a;
      } else {
        return (sched_task_t *) p_task_b;
      }
    } else {
      // Only Task A is active.
      return (sched_task_t *) p_task_a;
    }
  } else if(TASK_ACTIVE_SAFE(p_task_b)) {
      // Only Task B is active
      return (sched_task_t *) p_task_b;
  } else {
    // Neither task is active.
    return NULL;
  }
}

/***** Internal Scheduler Functions *****/

/**
 * @brief Function for removing all tasks from the scheduler's que.
 * 
 * @return void
 */
static void sched_clear_que() {

  // Get exclusive que access.
  sched_port_lock();

  // Walk through the task list starting at the head.
  sched_task_t *p_current_task = scheduler.p_head;

  while (p_current_task != NULL) {
    // Set each task as uninitialized.
    p_current_task->state = SCHED_TASK_UNINIT;

    // Move to the next task in the linked list
    p_current_task = p_current_task->p_next;
  }

  // Clear the task references.
  scheduler.p_head = NULL;
  scheduler.p_tail = NULL;
  scheduler.p_next = NULL;

  // Release the que lock.
  sched_port_free();
}

/**
 * @brief Function for completing a scheduler stop.
 * 
 * Called once the scheduler finishes executing the expired tasks handlers
 * to complete a scheduler stop.
 * 
 * @return void
 */
static void sched_stop_finalize(void) {
  if (scheduler.state == SCHED_STATE_STOPPING) {

    // Clear the que.
    sched_clear_que();

    // Perform any platform specific deinitialization last.
    sched_port_deinit();

    scheduler.state = SCHED_STATE_STOPPED;
  }
}




#if 0

/**
 * @brief Internal function for finding the first active task in the task list.
 *
 * @return void
 */
static void sched_next_task(void) {

  // Pointer to the next expiring task.
  sched_task_t *p_expiring_task = NULL;

  /* Walk through each task in the linked list starting at the 
   * head task and ending at the tail task.
   */
  sched_task_t *p_current_task = scheduler.p_head;

  while (p_current_task != NULL) {
    /* Only need to test for active state and not the executing state here 
     * since no tasks will be executing when the function is called.
     */
    if (p_current_task->state == SCHED_TASK_ACTIVE) {
      if (p_expiring_task == NULL) {
        // No expiring task was previously set so the current task is the next expiring task.
        p_expiring_task = p_current_task;
      } else if (sched_task_remaining_ms(p_expiring_task) > sched_task_remaining_ms(p_current_task)) {
        p_expiring_task = p_current_task;
      }
    }

    // Move to the next task in the list
    p_current_task = p_current_task->p_next;
  }

  // Update the cached expiring task.
  scheduler.p_next = p_expiring_task;
}

#else

/**
 * @brief Internal function for finding the first active task in the task list.
 * 
 * @retval The first active task that was found.
 * @retval NULL if no active tasks were found.
 */
static sched_task_t * sched_active_task(void) {

  /* Start searching for first active task at the head task and end at the 
   * tail task if no active tasks are found.
   */
  sched_task_t *p_search_task = scheduler.p_head;

  while (p_search_task != NULL) {
    if(p_search_task->state == SCHED_TASK_ACTIVE) {
      return p_search_task;
    }
    // Move to the next task in the list.
    p_search_task = p_search_task->p_next;
  }
  // No active tasks were found.
  return NULL;
}

/**
 * @brief Internal function for updating the cached next expiring task.
 *
 * The function checks each task in the task for the next expiring task.
 * If an an expired task is found, the expired task is considred to be next 
 * expiring task and the function returns immediately.
 * 
 * The current time is locally cached so the the sched_port_ms() function 
 * only has to be called once.  
 * 
 * @note The function should only be called from the main context while 
 * outside of a task handler since it assumes that no tasks are in the 
 * executing state.
 *
 * @return void
 */
static void sched_next_task(void)
{
  /* Search for the first active task in the task list and set it to be 
   * the next expiring task.
   */
  sched_task_t *p_next_task = sched_active_task();

  // Store the current time.
  uint32_t now_time_ms = sched_port_ms();

  /* The time remaining for the next expiring task is stored so that it
   * does not have to be recalculated at each task comparision.
  */
  uint32_t next_task_ms = task_time_remaining_ms(p_next_task, now_time_ms);  

  /* If no active tasks were found or the first active task is expired, 
   * stop the search.
   */
  if ((p_next_task != NULL) && (next_task_ms > 0)) {

    /* Start searching for the next expiring task at the next task after 
     * first active task which was previously found.  End the search at the 
     * last task in the linked list.
     * 
     * The search task is the the task from que which is currently being 
     * compared to the previously found expiring task.
    */
    sched_task_t *p_search_task = p_next_task->p_next;

    while (p_search_task != NULL) {
      // Filter on active tasks.
      if (p_search_task->state == SCHED_TASK_ACTIVE) {
        // Calculate the search task's remaining time.
        uint32_t search_task_ms = task_time_remaining_ms(p_search_task, now_time_ms);

        /* If the current task expires before the previously found next
         * expiring task, it becomes the new next task.
        */
        if (search_task_ms < next_task_ms) {
          if (search_task_ms == 0) {
            // The search task has already expired, it must be the next task.
            scheduler.p_next = p_search_task;
            return;
          }
          p_next_task = p_search_task;
          next_task_ms = search_task_ms;
        }
      }
      // Move to the next task in the list
      p_search_task = p_search_task->p_next;
    }
  }

  // Update the cached expiring task.
  scheduler.p_next = p_next_task;
}
#endif


/**
 * @brief Function for executing tasks in the que with expired intervals.
 *
 * This function must be called from within the main loop. It executes
 * all scheduled tasks having expired timers before returning.
 *
 * @return void
 */
static void sched_execute_que(void) {

  // Store the current time to reduce calls to the port time funciton.
  uint32_t now_time_ms = sched_port_ms();

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
   * This technique protects against the cached next task being modified
   * from a different context without having to utilize the scheduler lock.
   */

  // TODO move this check into a static inline function.

   sched_task_t * p_next_copy = scheduler.p_next;
   if ((p_next_copy != NULL) &&
      (p_next_copy->state == SCHED_TASK_ACTIVE) &&
      !task_time_expired(p_next_copy, now_time_ms) &&
      (p_next_copy == scheduler.p_next)) 
  {
    //  Return if a cached next task exists but has not expired yet.      
    return;
  }
  
/* TODO we wind up looping through the task que twice with this implementation.   
 * Its may be more efficient to immediately execute the next expiring task if
 * one has been previously stored.  
 * 
 * Once executed, we can perform a new search for the next expiring task.  If
 * the next task hasn't expired yet, we can sleep otherwise we can execute its 
 * handler and then repeat the search until we find a task that isn't expired. 
 * This is likely  better for an application with tasks that don't frequently 
 * expire at the same time.  It may not be better for a system with a lot of 
 * task.
 * 
 * We may want to make the search method configurable.  
 */ 
  // Loop through each task in the linked list starting with the head task
  sched_task_t *p_current_task = scheduler.p_head;

  while (p_current_task != NULL) {

    // Filter on active tasks with expired timers.
    if ((p_current_task->state == SCHED_TASK_ACTIVE) && 
        task_time_expired(p_current_task, now_time_ms)) {

      if(p_current_task->repeat) {
        /* A repeating task will be in the executing state while inside of
         * its handler. 
         */ 
        p_current_task->state = SCHED_TASK_EXECUTING;

       /* Update the start time before calling the handler so the handler's 
        * execution time doesn't introduce error.  The start time only needs 
        * to be updated for repeating tasks.
        */
        p_current_task->start_ms = sched_port_ms();

      } else {
        /* A non-repeating task will be in the stopping state while executing 
         * its handler.
         */
        p_current_task->state = SCHED_TASK_STOPPING;
      }

      // Call the task's handler function.
      sched_handler_t handler = (sched_handler_t) p_current_task->p_handler;
      assert(handler != NULL);
      handler(p_current_task, p_current_task->p_data, p_current_task->data_size);

      // Update the state once the handler returns.
      if(p_current_task->state == SCHED_TASK_EXECUTING) {
        // Executing tasks move back to the active state.
        p_current_task->state = SCHED_TASK_ACTIVE;
      } else {
        assert(p_current_task->state == SCHED_TASK_STOPPING);
        // Stopping tasks move to the stopped state.
        p_current_task->state = SCHED_TASK_STOPPED;
        // A task is no longer allocated once stopped.
        p_current_task->allocated = false;
      }
    }

    // Move to the next task in the list
    p_current_task = p_current_task->p_next;
  }

  /* The cached next expiring task should always refreshed here since the 
   * previous expiring task was serviced.
   */
  sched_next_task();

}


/***** External Scheduler Task Functions *****/

bool sched_task_config(sched_task_t *p_task, sched_handler_t handler,
    uint32_t interval_ms, bool repeat) {

  // A pointer to the task and its handler must be supplied.
  if((p_task == NULL) || (handler == NULL)) {
    return false;
  }

  if ((p_task->state == SCHED_TASK_EXECUTING) || (p_task->state == SCHED_TASK_STOPPING)) {
    // A task can't be configured while its handler is currently executing.
    return false;
  } else if (p_task->state == SCHED_TASK_UNINIT) {
    /* Add the task to the scheduler's que if it hasn't been previously added.
     * The new task will be the last one in the list so it's next task will 
     * always be NULL.
     */ 
    p_task->p_next = NULL;

    // Take exclusive write access of scheduler's task que.
    sched_port_lock();

    if (scheduler.p_head == NULL) {
      /* No other tasks exists in the list so the new task will be both the 
       * head & tail task.
       */
      scheduler.p_head = p_task;
    } else {
      /* If other tasks exist, this task will be the next task for the current 
       * tail task.
       */
      assert(scheduler.p_tail != NULL);
      scheduler.p_tail->p_next = p_task;
    }
    // Set the new task to the tail task to add it to the end of the list.
    scheduler.p_tail = p_task;

    // Release the task que exclusive access.
    sched_port_free();
  }

  // Store the task handler.
  p_task->p_handler = handler;

  // Store the task interval limiting it to the max interval.
  p_task->interval_ms = (interval_ms % SCHED_MS_MAX);

  // Store the repeating status.
  p_task->repeat = repeat;

  // Tasks will always be in the stopped state after configuration. 
  p_task->state = SCHED_TASK_STOPPED;

  return true;
}

bool sched_task_start(sched_task_t *p_task) {

  // A pointer to the task must be supplied.
  if(p_task == NULL) {
    return false;
  }

  if(p_task->state == SCHED_TASK_UNINIT) {
    // A task must be configured before it can be started.
    return false;
  } else if(p_task->state == SCHED_TASK_STOPPED) {
    // Set the task to active if it is currently stopped.
    p_task->state = SCHED_TASK_ACTIVE;
  } else if(p_task->state == SCHED_TASK_STOPPING) {
    /* Set the task to executing if is currently stopping.
     * This could happen if the task is started inside an
     * ISR while executing it's handler or if the task is
     * restarted inside its handler.
     */
    p_task->state = SCHED_TASK_EXECUTING;
  }

  // Store the start time as now.
  p_task->start_ms = sched_port_ms();
  
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
  p_task->interval_ms = (interval_ms % SCHED_MS_MAX);

  // Start the task. 
  return sched_task_start(p_task);
}


uint8_t sched_task_data(sched_task_t * p_task, const void * p_data, uint8_t data_size) {

   // Data can only be set for stopped tasks.
  if((p_task == NULL) || (p_task->state != SCHED_TASK_STOPPED)) {
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
      memcpy(p_task->p_data, (uint8_t *) p_data, p_task->data_size);
    }
  } else {
    // Just set the data pointer for an unbuffered task.
    p_task->p_data = (uint8_t *) p_data;
  }

  // Return the data size.
  return p_task->data_size;
}

bool sched_task_stop(sched_task_t *p_task) {

  // A pointer to the task must be supplied.
  if(p_task == NULL) {
    return false;
  }

  if(p_task->state == SCHED_TASK_UNINIT) {

    // A task must have been previously initialized.
    return false;

  } else if(p_task->state == SCHED_TASK_ACTIVE) {

    // Active task can move to the stopped state immediately.
    p_task->state = SCHED_TASK_STOPPED;

    // A task is no longer allocated once stopped.
    p_task->allocated = false;

  } else if(p_task->state == SCHED_TASK_EXECUTING) {
    /* Executing tasks move to the stopping state since their 
     * handlers are currently executing. The stop will complete
     * after the handler returns.
     */
    p_task->state = SCHED_TASK_STOPPING;
  }

  /* It's not necessary to clear the cached next task here
   * since the cached next task's state is checked to be active
   * before it is executed.  
   */
  return true;
}

/***** External Scheduler Task Pool Functions *****/

// Internal function for initializing a scheduler task pool.
static void sched_task_pool_init(sched_task_pool_t * p_pool) {

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

    // Store the task's buffer size.
    p_task_cur->buff_size = p_pool->buff_size;

    // Increment the task pointer
    p_task_cur ++;

    // Increment the data buffer pointer
    p_data_cur += p_pool->buff_size;

  } while(p_task_cur <= p_task_last);

  p_pool->initialized = true;
  
}

sched_task_t * sched_task_alloc(sched_task_pool_t * p_pool) {

  if(p_pool == NULL) {
    return NULL;
  }

  // Initialize the pool of buffered tasks if not previously initialized.
  if(p_pool->initialized == false) {
    sched_task_pool_init(p_pool);
  }

  // Pointer to the current and last tasks in the pool.
  sched_task_t * p_task_cur = p_pool->p_tasks;
  sched_task_t * p_task_last = p_pool->p_tasks + p_pool->task_cnt;

  // Search for the first unallocated task.
  do {
    if(!p_task_cur->allocated) {

      /* Acquire the scheduler lock so the task can be allocated
       * without interruption.
       */
      sched_port_lock();

      // Check that the task is still unallocated
      if(p_task_cur->allocated) {

        /* The task allocation status must have been changed 
         * from a different context before the task could be 
         * allocated. Release the lock and continue the search.
         */
        sched_port_free();

      } else {
      
        // Mark the task as allocated.
        p_task_cur->allocated = true;

        // Release the lock
        sched_port_free();
      
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

uint8_t sched_pool_allocated(const sched_task_pool_t * p_pool) {

  if( (p_pool == NULL) || (p_pool->initialized == false) ) {
    return 0;
  }

  // Count of the allocated pool tasks.
  uint8_t alloc_count = 0;

  // Pointer to the current and last tasks in the pool.
  sched_task_t * p_task_cur = p_pool->p_tasks;
  sched_task_t * p_task_last = p_pool->p_tasks + p_pool->task_cnt;

  do {
    if(!p_task_cur->allocated) {
      alloc_count ++;
    }
    // Increment the task pointer
    p_task_cur ++;

  } while(p_task_cur <= p_task_last);

  return alloc_count;
}

uint8_t sched_pool_free(const sched_task_pool_t * p_pool) {
  if(p_pool == NULL) {
    return 0;
  }
  return p_pool->task_cnt - sched_pool_allocated(p_pool);
}

/***** External Scheduler Functions *****/

void sched_init(void) {

  // Start the scheduler if not currently running.
  if (scheduler.state == SCHED_STATE_STOPPED) {

    // Perform any platform specific initialization first.
    sched_port_init();

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
      sched_port_sleep(sleep_interval);
    }
  }

  // Finish stopping the scheduler before returning.
  sched_stop_finalize();
}

void sched_stop(void) {
  // Move to the stopping state if not already stopped.
  if (scheduler.state != SCHED_STATE_STOPPED) {
    scheduler.state = SCHED_STATE_STOPPING;
  }
}

/***** Weak implementations of the optional port functions. *****/

__attribute__((weak)) void sched_port_sleep(uint32_t interval_ms) {
  // Empty
}

__attribute__((weak)) void sched_port_init(void) {
  // Empty
};

__attribute__((weak)) void sched_port_deinit(void) {
  // Empty
};