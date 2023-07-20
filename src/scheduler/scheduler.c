/**
 * @file scheduler.c
 * @author Ben Wirz
 */

#include <assert.h>
#include <string.h>
#include "scheduler.h"

/***** Scheduler Configuration Defines *****/

/**
 * @brief Definition to enable / disable clearing buffered task data.
 *
 * If SCHED_TASK_BUFF_CLEAR_EN is defined to be > 0, task data buffers
 * will be cleared when they are configured.  The default implementation
 * is to not clear the buffer but the end user can override this by defining
 * SCHED_TASK_BUFF_CLEAR_EN to be 1 if desired.  Clearing large task data
 * buffer can be expensive and is unnecessary for most applications since the
 * buffer is overwritten when data is added.  Clearing the buffer can be
 * useful for certain debugging purposes and it therefore optionally supported.
 */
#ifndef SCHED_TASK_BUFF_CLEAR_EN
#define SCHED_TASK_BUFF_CLEAR_EN (0)
#endif

/**
 * @brief Definition for the maximum task interval time in mS.
 *
 * The define sets the maximum task interval time in mS.  The default value
 * of UINT32_MAX will be suitable for most applications but the end user
 * can define a lower value should they need to limit task intervals.
 */
#ifndef SCHED_MS_MAX
#define SCHED_MS_MAX (UINT32_MAX)
#endif

/**
 * @brief Definition to enable or disable Scheduler Task Pool's.
 *
 * If SCHED_TASK_POOL_EN is defined to be != 0, scheduler task pool support
 * will be enabled.  Task pools are enabled by default but the end user can
 * disable them to reduce the scheduler's ROM footprint support for them is not
 * needed.
 */
#ifndef SCHED_TASK_POOL_EN
#define SCHED_TASK_POOL_EN (1)
#endif

/**
 * @brief Definition to enable or disable task caching.
 *
 * If SCHED_TASK_CACHE_EN is defined to be != 0, the scheduler will save the
 * next expiring task during each task service loop.  This will improve the
 * efficiency of the task search in many case by enabling the scheduler to
 * immediately check the next expiring task on wake up.  If the cached task is
 * unexpired, the scheduler can skip the task que search and immediately put
 * the processor back to sleep.  The caching optimization is enabled by default
 * but can be disabled by end user if desired.
 */
#ifndef SCHED_TASK_CACHE_EN
#define SCHED_TASK_CACHE_EN (1)
#endif

/***** Scheduler Module Internal Data *****/

/**
 * @brief Scheduler State Type
 */
typedef enum
{
  /// @brief The scheduler is stopped.
  SCHED_STATE_STOPPED = 0,
  /// @brief The scheduler is running.
  SCHED_STATE_ACTIVE,
  /// @brief The scheduler is in the process of stopping.
  SCHED_STATE_STOPPING,
} sched_state_t;

/**
 * @brief The scheduler's internal data structure.
 *
 * @note The scheduler should be locked prior to modifying any of the task
 * pointers.
 */
typedef struct
{
  /**
   * @brief Pointer to the head task in the task que.
   *
   * The pointer to the head task gives the scheduler a starting point for
   * traversing the task que.
   */
  sched_task_t *p_head;
  /**
   * @brief Pointer to the tail task in the task que.
   *
   * The pointer to the tail task provides the scheduler with a reference to
   * the last task in the task que.  New tasks are always added to the end of
   * the que.
   */
  volatile sched_task_t *p_tail;

#if (SCHED_TASK_CACHE_EN != 0)
  /**
   * @brief The updated flag tracks wether any active tasks have had their
   * intervals updated since the schedulers task que was last serviced.  A
   * task with an updated interval could invalidate the cached next task.  The
   * updated flag indicates that the cached next task should be ignored and
   * the next task search repeated by the scheduler during the task service
   * loop.
   */
  volatile bool updated;
#endif

  /**
   * @brief The module's current state.
   *
   * The state variable must be volatile since the scheduler can be stopped
   * from an interrupt context.
   */
  volatile sched_state_t state;
} scheduler_t;

// The scheduler module's internal data.
static scheduler_t scheduler = {
    .p_head = NULL,
    .p_tail = NULL,
#if (SCHED_TASK_CACHE_EN != 0)
    .updated = false,
#endif
    .state = SCHED_STATE_STOPPED};

/***** Internal Scheduler Task Helper Macro's. *****/

/**
 * @brief Macro for checking if a task is buffered.
 *
 * @note The task pointer is not NULL checked
 *
 * @param[in] p_task   Pointer to the task.
 * @return             True if the task is buffered.
 *                     False if the task is unbuffered.
 */
#define TASK_BUFFERED(p_task) ((p_task)->buff_size > 0)

/**
 * @brief Macro for safely checking if a task is buffered.
 *
 * @param[in] p_task   Pointer to the task.
 * @return             True if the task is buffered.
 *                     False if the task is unbuffered
 *                     or the task pointer is NULL>
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
 * @param[in] p_task   Pointer to the task.
 * @return             True if the task is active.
 *                     False if the task pointer is NULL
 *                     or the task is inactive.
 */
#define TASK_ACTIVE_SAFE(p_task) (((p_task) != NULL) && TASK_ACTIVE(p_task))

/**
 * @brief Macro for checking if a task has expired.
 *
 * @note: The task pointer is not NULL checked and the task's active status
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

/***** Internal Scheduler Task Helper Functions. *****/

/**
 * @brief Internal function for setting a task's interval.
 *
 * The tasks repeating status should be set prior to making the function call.
 *
 * @note: The task pointer is not NULL checked.
 *
 * @param[in] p_task    Pointer to the task.
 * @param[in] interval_ms  The new interval to store.
 */
static inline void task_interval_set(sched_task_t *p_task, uint32_t interval_ms)
{

  if (p_task->repeat && (interval_ms == 0))
  {
    /* Repeating tasks must have an interval > 0.  A repeating task with an
     * interval of 0 would repeatably be executed by the scheduler and starve
     * other tasks of processor time.
     */
    p_task->interval_ms = 1;
  }
  else
  {
#if (SCHED_MS_MAX < UINT32_MAX)
    // Limit the interval to the max interval if one is defined.
    if (interval_ms > SCHED_MS_MAX)
    {
      p_task->interval_ms = SCHED_MS_MAX;
    }
    else
    {
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
 * @param[in] now_time_ms  The current time in mS.
 * @return The time since the task was started.
 */
static inline uint32_t task_time_elapsed_ms(const sched_task_t *p_task, uint32_t now_time_ms)
{
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
static inline bool task_time_expired(const sched_task_t *p_task, uint32_t now_time_ms)
{
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
static inline uint32_t task_time_remaining_ms(const sched_task_t *p_task, uint32_t now_time_ms)
{
  uint32_t elapsed_ms = now_time_ms - p_task->start_ms;
  if (p_task->interval_ms > elapsed_ms)
  {
    return p_task->interval_ms - elapsed_ms;
  }
  else
  {
    return 0;
  }
}

/***** External Scheduler Task Helper Functions *****/

bool sched_task_expired(const sched_task_t *p_task)
{

  if (TASK_ACTIVE_SAFE(p_task))
  {
    return (sched_port_ms() - p_task->start_ms) >= p_task->interval_ms;
  }
  else
  {
    return false;
  }
}

uint32_t sched_task_remaining_ms(const sched_task_t *p_task)
{

  if (TASK_ACTIVE_SAFE(p_task))
  {
    uint32_t elapsed_ms = sched_port_ms() - p_task->start_ms;

    if (elapsed_ms < p_task->interval_ms)
    {
      return p_task->interval_ms - elapsed_ms;
    }
    else
    {
      // Expired
      return 0;
    }
  }
  else
  {
    return SCHED_MS_MAX;
  }
}

uint32_t sched_task_elapsed_ms(const sched_task_t *p_task)
{

  if (TASK_ACTIVE_SAFE(p_task))
  {
    return sched_port_ms() - p_task->start_ms;
  }
  else
  {
    return 0;
  }
}

sched_task_t *sched_task_compare(const sched_task_t *p_task_a, const sched_task_t *p_task_b)
{

  if (TASK_ACTIVE_SAFE(p_task_a))
  {
    if (TASK_ACTIVE_SAFE(p_task_b))
    {
      // Both tasks are active, compare the remaining time.
      if (sched_task_remaining_ms(p_task_a) <= sched_task_remaining_ms(p_task_b))
      {
        return (sched_task_t *)p_task_a;
      }
      else
      {
        return (sched_task_t *)p_task_b;
      }
    }
    else
    {
      // Only Task A is active.
      return (sched_task_t *)p_task_a;
    }
  }
  else if (TASK_ACTIVE_SAFE(p_task_b))
  {
    // Only Task B is active
    return (sched_task_t *)p_task_b;
  }
  else
  {
    // Neither task is active.
    return NULL;
  }
}

/***** Internal Scheduler Functions *****/

#if (SCHED_TASK_CACHE_EN != 0)

/**
 * @brief Function for atomically getting the updated flag and clearing it in
 * one call.
 *
 * The function takes exclusive access to the schedulers data structure,
 * makes a copy of updated flag, clears the flag, releases exclusive access and
 * then returns the original flag value.
 *
 * @return  The value of the updated flag before it was cleared.
 */
static inline bool sched_updated_get_clear()
{
  sched_port_lock();
  bool updated = scheduler.updated;
  scheduler.updated = false;
  sched_port_free();
  return updated;
}

/**
 * @brief Function for atomically setting the schedulers updated flag.
 */
static inline void sched_updated_set()
{
  sched_port_lock();
  scheduler.updated = true;
  sched_port_free();
}

#else
static inline void sched_updated_set()
{
  // Empty
}

#endif // (SCHED_TASK_CACHE_EN != 0)

/**
 * @brief Internal function for removing all tasks from the scheduler's que.
 */
static void sched_clear_que(void)
{

  // Get exclusive que access.
  sched_port_lock();

  // Walk through the task list starting at the head.
  sched_task_t *p_current_task = (sched_task_t *)scheduler.p_head;

  while (p_current_task != NULL)
  {
    // Set each task as uninitialized.
    p_current_task->state = SCHED_TASK_UNINIT;

    // Move to the next task in the linked list
    p_current_task = p_current_task->p_next;
  }

  // Clear the task references.
  scheduler.p_head = NULL;
  scheduler.p_tail = NULL;

  // Release the que lock.
  sched_port_free();
}

/**
 * @brief Internal function for completing a scheduler stop.
 *
 * Called once the scheduler finishes executing the expired tasks handlers
 * to complete a scheduler stop.
 */
static void sched_stop_finalize(void)
{
  if (scheduler.state == SCHED_STATE_STOPPING)
  {

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
 */
static void task_execute_handler(sched_task_t *p_task)
{

  assert(p_task != NULL);
  if (p_task->repeat)
  {
    /* A repeating task will be in the executing state while inside of
     * its handler.
     */
    p_task->state = SCHED_TASK_EXECUTING;

    /* Update the start time before calling the handler so the handler's
     * execution time doesn't introduce error.  The start time only needs
     * to be updated for repeating tasks.
     */
    p_task->start_ms = sched_port_ms();
  }
  else
  {
    /* A non-repeating task will be in the stopping state while executing
     * its handler.  It will be stopped once the handler returns.
     */
    p_task->state = SCHED_TASK_STOPPING;
  }

  // Call the task's handler function.
  sched_handler_t handler = (sched_handler_t)p_task->p_handler;
  assert(handler != NULL);
  handler(p_task, p_task->p_data, p_task->data_size);

  // Update the task state after the handler finishes.
  if (p_task->state == SCHED_TASK_EXECUTING)
  {
    // Executing tasks move back to the active state.
    p_task->state = SCHED_TASK_ACTIVE;
  }
  else
  {
    assert(p_task->state == SCHED_TASK_STOPPING);
    // Stopping tasks move to the stopped state.
    p_task->state = SCHED_TASK_STOPPED;
    // A task is no longer allocated once its stopped.
    p_task->allocated = false;
  }
}

/**
 * @brief Internal function for executing tasks in the scheduler's task que
 * which have expired intervals.
 *
 * The function first checks the cached next task for expiration.  If it is
 * valid and unexpired, the function returns immediately.
 *
 * If cached next task has expired or is invalid, the function services any
 * expired task in the que and stores next expiring task for future use.
 *
 * @return The time until expiration of the next expiring task in mS or
 *         SCHED_MS_MAX if no active tasks were found.
 */
static uint32_t sched_execute_que()
{

  // Get the current time.
  uint32_t now_time_ms = sched_port_ms();

#if (SCHED_TASK_CACHE_EN != 0)
  // Store next expiring task statically to persist it across calls.
  static sched_task_t *p_next_task = NULL;

  // Check the updated flag and clear it.
  if (!sched_updated_get_clear())
  {
    // No tasks have been been updated so attempt to service the cached next task.
    if ((p_next_task != NULL) && (p_next_task->state == SCHED_TASK_ACTIVE))
    {

      // Calculate the cached task's time until expiration.
      uint32_t cache_task_ms = task_time_remaining_ms(p_next_task, now_time_ms);

      if (cache_task_ms == 0)
      {
        // The cached next task has expired, execute its handler.
        task_execute_handler(p_next_task);
      }
      else
      {
        /* The next task has not expired yet so return the time remaining until
         * expiration so the processor can go back to sleep.
         */
        return cache_task_ms;
      }
    }
  }
  // Clear the cached next task since its either been serviced or is invalid.
  p_next_task = NULL;
#else
  // The next expiring task, doesn't need to be static if the cache is disabled.
  sched_task_t *p_next_task = NULL;
#endif

  /* The next expiring task's time until expiration.  The time until
   * expiration is stored in addition the task pointer.  This improves the
   * task loop efficiency since the  loop does not have to recalculate the
   * interval each time through the loop.
   */
  uint32_t next_task_ms = UINT32_MAX;

  // Start searching for the next expiring task at the start of the linked list.
  sched_task_t *p_search_task = (sched_task_t *)scheduler.p_head;

  while (p_search_task != NULL)
  {

    // Filter on active tasks.
    if (p_search_task->state == SCHED_TASK_ACTIVE)
    {

      // Calculate the search task's remaining time.
      uint32_t search_task_ms = task_time_remaining_ms(p_search_task, now_time_ms);

      if (search_task_ms == 0)
      {
        /* Execute the search task's handler if the task has expired.
         *
         * Note that the scheduler only moves to the next task in the list
         * once the task is unexpired.  The search task's expiration time is
         * recalculated each time its handler returns since the task interval
         * may have been modified inside the handler.  This carries the risk
         * that an an always expiring task could potentially starve the other
         * tasks of processor cycles if it were to repeatably restart itself
         * with an expired interval inside its own handler.
         */
        task_execute_handler(p_search_task);
      }
      else
      {
        /* If the search task expires before the previously found next expiring
         * task, it becomes the next expiring task.
         */
        if (search_task_ms < next_task_ms)
        {
          p_next_task = p_search_task;
          next_task_ms = search_task_ms;
        }
        // Move to the next task in the list
        p_search_task = p_search_task->p_next;
      }
    }
    else
    {
      // Move to the next task in the list if the search task is inactive.
      p_search_task = p_search_task->p_next;
    }
  }

  /* Recalculate the next tasks expiration time using the current mS timer
   * value to improve the accuracy of the sleep interval in cases were the task
   * execution time was significant.
   */
  return sched_task_remaining_ms(p_next_task);
}

/***** External Scheduler Task Functions *****/

bool sched_task_config(sched_task_t *p_task, sched_handler_t handler,
                       uint32_t interval_ms, bool repeat)
{

  // A pointer to the task and its handler must be supplied.
  if ((p_task == NULL) || (handler == NULL))
  {
    return false;
  }

  if (scheduler.state == SCHED_STATE_STOPPED)
  {
    // Task's can only be configured after the scheduler has been initialized.
    return false;
  }

  if (p_task->state == SCHED_TASK_UNINIT)
  {

    /* Add the task to the scheduler's que if it hasn't been previously added.
     * The new task will be the last one in the list so its next task will
     * always be NULL.
     */
    p_task->p_next = NULL;

    // Take exclusive write access of scheduler's task que.
    sched_port_lock();

    if (scheduler.p_head == NULL)
    {
      /* No other tasks exists in the list, the new task will be both the
       * head & tail task.
       */
      scheduler.p_head = p_task;
    }
    else
    {
      /* The head task has already been set so this task will be the next task
       * for the current tail task.
       */
      assert(scheduler.p_tail != NULL);
      scheduler.p_tail->p_next = p_task;
    }
    // Set the new task to the tail task so it is added to the end of the list.
    scheduler.p_tail = p_task;

    // Release the task que exclusive access.
    sched_port_free();
  }
  else if (p_task->state != SCHED_TASK_STOPPED)
  {
    // A task can only be configured in the uninitialized or stopped states.
    return false;
  }

  // Clear the task data buffer.
#if (SCHED_TASK_BUFF_CLEAR_EN != 0)
  if (TASK_BUFFERED(p_task))
  {
    memset(p_task->p_data, 0x00, p_task->buff_size);
  }
#endif

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

bool sched_task_start(sched_task_t *p_task)
{

  // A pointer to the task must be supplied.
  if (p_task == NULL)
  {
    return false;
  }

  if (p_task->state == SCHED_TASK_UNINIT)
  {
    // A task must be configured before it can be started.
    return false;
  }
  else if (p_task->state == SCHED_TASK_STOPPED)
  {
    // Set the task to active if it is currently stopped.
    p_task->state = SCHED_TASK_ACTIVE;

    /* Set the updated flag to indicate that the newly started task might
     * have invalidated the cached expiring task.
     */
    sched_updated_set();
  }
  else if (p_task->state == SCHED_TASK_STOPPING)
  {
    /* Set the task to executing if it is currently stopping. This could happen
     * if the task is started inside an ISR while executing its handler or
     * more commonly if a non-repeating task restarts itself inside its
     * own handler.  Don't set the updated flag in the case since the cached
     * next expiring task will be updated on handler return if needed.
     */
    p_task->state = SCHED_TASK_EXECUTING;
  }

  // Store the start time as now.
  p_task->start_ms = sched_port_ms();

  return true;
}

bool sched_task_update(sched_task_t *p_task, uint32_t interval_ms)
{

  // A pointer to the task must be supplied.
  if (p_task == NULL)
  {
    return false;
  }
  // Store the new interval.
  task_interval_set(p_task, interval_ms);

  // Start the task.
  return sched_task_start(p_task);
}

uint8_t sched_task_data(sched_task_t *p_task, const void *p_data, uint8_t data_size)
{

  // Data can only be set for stopped tasks.
  if ((p_task == NULL) || (p_task->state != SCHED_TASK_STOPPED))
  {
    return 0;
  }

  // Store the data size.
  p_task->data_size = data_size;

  if (TASK_BUFFERED(p_task))
  {
    // Limit the data size to the task buffer size.
    if (p_task->data_size > p_task->buff_size)
    {
      p_task->data_size = p_task->buff_size;
    }

    if (p_data == NULL)
    {
      p_task->data_size = 0;
    }
    else
    {
      // Copy the data into the task data buffer.
      memcpy(p_task->p_data, (uint8_t *)p_data, p_task->data_size);
    }
  }
  else
  {
    // Just set the data pointer for an unbuffered task.
    p_task->p_data = (uint8_t *)p_data;
  }

  // Return the data size.
  return p_task->data_size;
}

bool sched_task_stop(sched_task_t *p_task)
{

  // A pointer to the task must be supplied.
  if (p_task == NULL)
  {
    return false;
  }

  if (p_task->state == SCHED_TASK_UNINIT)
  {

    // A task must have been previously initialized.
    return false;
  }
  else if (p_task->state == SCHED_TASK_ACTIVE)
  {

    // Active task can move to the stopped state immediately.
    p_task->state = SCHED_TASK_STOPPED;

    // A task is no longer allocated once stopped.
    p_task->allocated = false;
  }
  else if (p_task->state == SCHED_TASK_EXECUTING)
  {
    /* Executing tasks move to the stopping state since their
     * handlers are currently executing. The stop will complete
     * after the handler returns.
     */
    p_task->state = SCHED_TASK_STOPPING;
  }

  return true;
}

/***** Scheduler Task Pool Functions *****/

#if (SCHED_TASK_POOL_EN != 0)

// Internal function for initializing a scheduler task pool.
static void sched_task_pool_init(sched_task_pool_t *p_pool)
{

  assert(p_pool != NULL);
  assert(p_pool->p_data != NULL);
  assert(p_pool->p_tasks != NULL);

  for (uint8_t index = 0; index < p_pool->task_cnt; index++)
  {

    // Set the task's data pointer to the buffer location.
    p_pool->p_tasks[index].p_data = p_pool->p_data + (index * p_pool->buff_size);

    // Set the task's buffer size.
    p_pool->p_tasks[index].buff_size = p_pool->buff_size;

    p_pool->p_tasks[index].allocated = false;
    p_pool->p_tasks[index].state = SCHED_TASK_UNINIT;
  }

  p_pool->initialized = true;
}

sched_task_t *sched_task_alloc(sched_task_pool_t *p_pool)
{

  if (p_pool == NULL)
  {
    return NULL;
  }

  if (scheduler.state == SCHED_STATE_ACTIVE)
  {

    // Initialize the pool of buffered tasks if not previously initialized.
    if (p_pool->initialized == false)
    {
      sched_task_pool_init(p_pool);
    }

    // Pointer to the current and last tasks in the pool.
    sched_task_t *p_task_cur = p_pool->p_tasks;
    sched_task_t *p_task_last = &p_pool->p_tasks[p_pool->task_cnt];

    // Search for the first unallocated task.
    do
    {
      if (!p_task_cur->allocated)
      {

        /* Acquire the scheduler lock so the task can be allocated
         * without interruption.
         */
        sched_port_lock();

        // Check that the task is still unallocated
        if (p_task_cur->allocated)
        {

          /* The task allocation status must have been changed
           * from a different context before the task could be
           * allocated. Release the lock and continue the search.
           */
          sched_port_free();
        }
        else
        {

          // Mark the task as allocated.
          p_task_cur->allocated = true;

          // Release the lock
          sched_port_free();

          // Reset the task data size.
          p_task_cur->data_size = 0;

          // Return the allocated task.
          return p_task_cur;
        }
      }
      // Increment the task pointer
      p_task_cur++;

    } while (p_task_cur <= p_task_last);
  }

  // No unallocated tasks were found.
  return NULL;
}

uint8_t sched_pool_allocated(const sched_task_pool_t *p_pool)
{

  if ((p_pool == NULL) || (p_pool->initialized == false))
  {
    return 0;
  }

  // Count of the allocated pool tasks.
  uint8_t alloc_count = 0;

  // Pointer to the current and last tasks in the pool.
  sched_task_t *p_task_cur = p_pool->p_tasks;
  sched_task_t *p_task_last = &p_pool->p_tasks[p_pool->task_cnt];

  do
  {
    if (!p_task_cur->allocated)
    {
      alloc_count++;
    }
    // Increment the task pointer
    p_task_cur++;

  } while (p_task_cur <= p_task_last);

  return alloc_count;
}

uint8_t sched_pool_free(const sched_task_pool_t *p_pool)
{
  if (p_pool == NULL)
  {
    return 0;
  }
  return p_pool->task_cnt - sched_pool_allocated(p_pool);
}

#else // (SCHED_TASK_POOL_EN != 0)

sched_task_t *sched_task_alloc(sched_task_pool_t *p_pool)
{
  return NULL; // Task pools are disabled, always return NULL.
}

uint8_t sched_pool_allocated(const sched_task_pool_t *p_pool)
{
  return 0; // Task pools are disabled, always return 0.
}

uint8_t sched_pool_free(const sched_task_pool_t *p_pool)
{
  return 0; // Task pools are disabled, always return 0.
}
#endif

/***** External Scheduler Functions *****/

void sched_init(void)
{

  // Start the scheduler if not currently running.
  if (scheduler.state == SCHED_STATE_STOPPED)
  {

    // Perform any platform-specific initialization first.
    sched_port_init();

    // Clear the task references.
    scheduler.p_head = NULL;
    scheduler.p_tail = NULL;
#if (SCHED_TASK_CACHE_EN != 0)
    scheduler.updated = false;
#endif
    scheduler.state = SCHED_STATE_ACTIVE;
  }
}

void sched_start(void)
{

  /* Repeatably execute any expired tasks in the scheduler's task list,
   * sleeping in between, until the scheduler is stopped.
   */
  while (scheduler.state == SCHED_STATE_ACTIVE)
  {

    // Execute tasks in the que with expired task intervals.
    uint32_t next_task_ms = sched_execute_que();

    /* Sleep using the platform-specific sleep method until the next task
     * expires.
     */
    if (next_task_ms > 0)
    {
      sched_port_sleep(next_task_ms);
    }
  }

  // Finish stopping the scheduler before returning.
  sched_stop_finalize();
}

void sched_stop(void)
{
  // Move to the stopping state if not already stopped.
  if (scheduler.state != SCHED_STATE_STOPPED)
  {
    scheduler.state = SCHED_STATE_STOPPING;
  }
}

/***** Weak implementations of the optional port functions. *****/

__attribute__((weak)) void sched_port_sleep(uint32_t interval_ms)
{
  // Empty
}

__attribute__((weak)) void sched_port_init(void){
    // Empty
};

__attribute__((weak)) void sched_port_deinit(void){
    // Empty
};
