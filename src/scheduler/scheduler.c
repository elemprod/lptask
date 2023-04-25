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
 * @brief Internal function for setting a task's interval.
 * 
 * The tasks repeating status should be set prior to making the function call.
 * 
 * @note: The task pointer is not NULL checked.
 * 
 * @param[in] p_task    Pointer to the task.
 * @param[in] interval  The new interval to store.
 * @return void
 */
static inline void task_interval_set(sched_task_t *p_task, uint32_t interval_ms) {

 if(p_task->repeat && (interval_ms == 0)) { 
    // Repeating tasks must have an interval > 0.   
    p_task->interval_ms = 1;
  } else {
#if (SCHED_MS_MAX < UINT32_MAX)
    // Limit the interval to the max interval.
    if(interval_ms > SCHED_MS_MAX) {
      p_task->interval_ms = SCHED_MS_MAX;
    } else {
      p_task->interval_ms = interval_ms;
    }
#else
    p_task->interval_ms = interval_ms;
#endif
  }
}

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
 * @note: The task is assumed to be active and the task pointer is not NULL 
 * checked.
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
 * @brief Internal function for removing all tasks from the scheduler's que.
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
 * @brief Internal function for completing a scheduler stop.
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

    // Perform any platform-specific deinitialization last.
    sched_port_deinit();

    scheduler.state = SCHED_STATE_STOPPED;
  }
}
/**
 * @brief Internal function for executing an expired task's handler function.
 * 
 * Note that the task is not checked to be active or for expiration.
 *
 * @return None
 */ 
static void task_execute_handler(sched_task_t *p_task) {

  assert(p_task != NULL);
  if(p_task->repeat) {
    /* A repeating task will be in the executing state while inside of
      * its handler. 
      */ 
    p_task->state = SCHED_TASK_EXECUTING;

    /* Update the start time before calling the handler so the handler's 
    * execution time doesn't introduce error.  The start time only needs 
    * to be updated for repeating tasks.
    */
    p_task->start_ms = sched_port_ms();

  } else {
    /* A non-repeating task will be in the stopping state while executing 
      * its handler.
      */
    p_task->state = SCHED_TASK_STOPPING;
  }

  // Call the task's handler function.
  sched_handler_t handler = (sched_handler_t) p_task->p_handler;
  assert(handler != NULL);
  handler(p_task, p_task->p_data, p_task->data_size);

  // Update the task state after the handler finishes.
  if(p_task->state == SCHED_TASK_EXECUTING) {
    // Executing tasks move back to the active state.
    p_task->state = SCHED_TASK_ACTIVE;
  } else {
    assert(p_task->state == SCHED_TASK_STOPPING);
    // Stopping tasks move to the stopped state.
    p_task->state = SCHED_TASK_STOPPED;
    // A task is no longer allocated once its stopped.
    p_task->allocated = false;
  }  
}

/**
 * @brief Internal function for executing tasks with expired intervals.
 *
 * The function first checks the cached next task for expiration.  If it is
 * unexpired, the function immediately returns which avoids having to search
 * the entire list for each call.
 * 
 * If no cached next task has been set, each task in the list is checked for
 * expiration and the next expiring task is stored for future use.
 * 
 * @note The function should only be called from the main context  when no
 * task handlers are active.
 *
 * @return The time until expiration of the next expiring task in mS or 
 *         SCHED_MS_MAX if no active tasks were found.
 */
static uint32_t sched_execute_que(void) {

  // Get the current time.
  uint32_t now_time_ms = sched_port_ms();

  // The next task's time until expiration.
  uint32_t next_task_ms = SCHED_MS_MAX;

  /* Make a copy of the next expiring task's pointer to guard against it 
   * being externally modified during the expiration check.
   */
  sched_task_t * p_next_task = scheduler.p_next;
  
  // Check if the next task has been set.
  if(p_next_task != NULL) {

    // The next task should be active here.
    assert(p_next_task->state == SCHED_TASK_ACTIVE);

    // Calculate the task's time until expiration.
    next_task_ms = task_time_remaining_ms(p_next_task, now_time_ms);  

    if(next_task_ms == 0) {
      // The cached next task has expired, execute the task handler.
      task_execute_handler(p_next_task);
    } else if(p_next_task == scheduler.p_next) {
      /* The next task has not expired yet and the schedulers cached next task 
       * has not been modified from a different context so return the time
       * remaining until expiration.
       */
      return next_task_ms;
    }
    // The next task was either serviced or is no longer valid so clear it.
    p_next_task = NULL;
    next_task_ms = SCHED_MS_MAX;
  } 

  // Start searching for the next expiring task at the start of the linked list.
  sched_task_t *p_search_task = scheduler.p_head;

  while (p_search_task != NULL) {

    // Filter on active tasks.
    if (p_search_task->state == SCHED_TASK_ACTIVE) {

      // Calculate the search task's remaining time.
      uint32_t search_task_ms = task_time_remaining_ms(p_search_task, now_time_ms);
    
      if (search_task_ms == 0) {        
        /* Execute the search task's handler if the task is expired. 
         * Note that we don't move to the next task in the list for case so 
         * that the search task is retested for expiration after its handler 
         * returns.  This has the risk that an an always expiring task 
         * could potentially starve the other tasks of CPU time.
        */  
        task_execute_handler(p_search_task);
      } else {

        /* If the search task expires before the previously found
         * expiring task, it becomes the new next task.
         */  
        if(search_task_ms < next_task_ms) {
          p_next_task = p_search_task;
          next_task_ms = search_task_ms;
        }

        // Move to the next task in the list
        p_search_task = p_search_task->p_next;
      }

    } else {
      // Move to the next task in the list
      p_search_task = p_search_task->p_next;
    }

  }
  
  // The next task time should always be > 0.
  assert(next_task_ms > 0);

  // Update the cached expiring task for future use.
  scheduler.p_next = p_next_task;
  return next_task_ms;
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
     * The new task will be the last one in the list so its next task will 
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

  // Store the repeating status.
  p_task->repeat = repeat;

  // Store the task interval.
  task_interval_set(p_task, interval_ms);

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
    /* Set the task to executing if it is currently stopping.
     * This can happen if the task is started inside an
     * ISR while executing its handler or if the task is
     * restarted inside its handler.
     */
    p_task->state = SCHED_TASK_EXECUTING;
  }

  // Store the start time as now.
  p_task->start_ms = sched_port_ms();
  
  /* Update the cached next task to the newly started task if it expires 
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
  // Store the new interval.
  task_interval_set(p_task, interval_ms);

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

  // Clear the cached next task if it is the stopped task.
  if(scheduler.p_next == p_task) {
    scheduler.p_next = NULL;
  }

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

    // Perform any platform-specific initialization first.
    sched_port_init();

    // Clear the task references.
    scheduler.p_head = NULL;
    scheduler.p_tail = NULL;
    scheduler.p_next = NULL;

    scheduler.state = SCHED_STATE_ACTIVE;
  }
}

void sched_start(void) {

  /* Repeatably execute any expired tasks in the scheduler's task list 
   * until the scheduler is stopped.
   */
  while (scheduler.state == SCHED_STATE_ACTIVE) {

    // Execute any tasks in the que with expired timers.
    uint32_t next_task_ms = sched_execute_que();
    assert(next_task_ms > 0);
    // Sleep until the next task expires using the platform-specific sleep method.
    sched_port_sleep(next_task_ms);
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