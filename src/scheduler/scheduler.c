#include "scheduler.h"
#include "scheduler_port.h"
#include <assert.h>

/**
 * Function like macro for checking if a task is active.
 *
 * Note that the task pointer is not NULL checked
 *
 *  @param[in] p_task   Pointer to the task.
 *  @return             True if the task is active else False.
 */
#define TASK_ACTIVE(p_task) (p_task->active == true)

/**
 * Function like macro for safely checking if a task is active.
 *
 *  @param[in] p_task   Pointer to the task.
 *  @return             True if the task is active.
 *                      False if the task is NULL or inactive.
 */
#define TASK_ACTIVE_SAFE(p_task) ((p_task != NULL) && (p_task->active == true))

/**
 * Function like macro for checking if a task has expired.
 *
 * Note that the task pointer is not NULL checked and the
 * task's Active status is not checked.
 *
 *  @param[in] p_task   Pointer to the task.
 *  @return             True if the task is expired else False.
 */
#define TASK_EXPIRED(p_task) ((scheduler_port_ms() - p_task->start_ms) >= p_task->interval_ms)

/**
 * Function like macro for safely checking if a task has expired.
 *
 *  @param[in] p_task   Pointer to the task.
 *  @return             True if the task is active and expired.
 *                      False if the task is inactive or NULL.
 */
#define TASK_EXPIRED_SAFE(p_task) (TASK_ACTIVE_SAFE(p_task) && TASK_EXPIRED(p_task))

typedef enum {
  SCHED_STATE_STOPPED = 0, // The module is stopped.
  SCHED_STATE_WAIT,        // The module is waiting for the next task to expire.
  SCHED_STATE_ACTIVE,      // The scheduler is handling any tasks with expired timers.
  SCHED_STATE_STOPPING,    // The scheduler is in the process of stopping.
} sched_state_t;


/**
 * The scheduler's interal data structure.
 *
 * The pointer to the head task gives the scheduler a starting point when
 * traversing the task linked list.
 *
 * The pointer to the tail task provides the scheduler with a reference to
 * the last  task in the linked list.  New tasks are always added to the end
 * of the list.
 *
 * The next expiring task is cached which avoids having to recalculate it
 * during each sched_execute() call.  This significantly improves effiency.
 *
 * The que must be locked prior to writing any of these pointers.
 */
typedef struct
{
  sched_task_t *p_head;         // Pointer to the head task in the task que.
  sched_task_t *p_tail;         // Pointer to the tail task in the task que.
  sched_task_t *p_next;         // Pointer to the next expiring task if known.
  volatile sched_state_t state; // The module's current state.
} scheduler_t;

// The module's internal data.
static scheduler_t scheduler = {.p_head = NULL,
    .p_tail = NULL,
    .p_next = NULL,
    .state = SCHED_STATE_STOPPED};

bool sched_init(void) {

  if (scheduler.state == SCHED_STATE_STOPPING) {
    // The scheduler can not be started if it is currently stopping.
    return false;
  }

  // Start the scheduler if currently stopped
  if (scheduler.state == SCHED_STATE_STOPPED) {
    // Perform any platform specific initialization first.
    scheduler_port_init();

    // Clear the task references.
    scheduler.p_head = NULL;
    scheduler.p_tail = NULL;
    scheduler.p_next = NULL;

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
  sched_task_t *p_current_task = scheduler.p_head;

  while (p_current_task != NULL) {

    // Mark each task as inactive and not added to the que.
    p_current_task->active = false;
    p_current_task->added = false;

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
  // Can't stop if the scheduler is currently executing the task que.
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
    /* The scheduler isn't executing the task que, 
     * it can stop immediately.
     */
    scheduler.state = SCHED_STATE_STOPPING;
    sched_stop_finalize();
    return true;

  case SCHED_STATE_ACTIVE:
    /* The scheduler is currently executing a task.
     * Move to the stopping state so the scheduler will be 
     * stopped once the current task completes.
     */
    scheduler.state = SCHED_STATE_STOPPING;
    return false;

  case SCHED_STATE_STOPPING:
    // Already stopping, nothing to do.
    return false;
  }
}

void sched_task_config(sched_task_t *p_task,
    sched_handler_t handler,
    void *p_context,
    uint32_t interval_ms,
    bool repeat) {

  // A pointer to the task to be added the scheduler que must be supplied.
  assert(p_task != NULL);

  // The task's handler must be supplied.
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

    // Release task que exclusive access.
    scheduler_port_que_free();

    p_task->added = true;
  }
}

#if 0
/**
 * Function for updating the next scheduler task reference.
 *
 * @param[in] p_task        Pointer to the next task.
 *                          Can be NULL.
 *
 * @return    none.
 */
static void sched_update_next(sched_task_t *p_next_task) {

  // Update the cached next task pointer if needed.
  if (scheduler.p_next != p_next_task) {
    scheduler_port_que_lock();
    scheduler.p_next = p_next_task;
    scheduler_port_que_free();
  }
}

#endif

void sched_task_start(sched_task_t *p_task) {

  // A pointer to the task to start must be supplied.
  assert(p_task != NULL);

  // The task must have been previously added to the que.
  assert(p_task->added);

  // Store the start time.
  p_task->start_ms = scheduler_port_ms();

  // Mark the task as active.
  p_task->active = true;

  /* Clear the reference to the cached next task. This 
   * task might expire before it does.
   */
  scheduler.p_next = NULL;
}

void sched_task_update(sched_task_t *p_task, uint32_t interval_ms) {

  // A pointer to the task to update must be supplied.
  assert(p_task != NULL);

  // Store the new interval limiting it to the max interval.
  p_task->interval_ms = (interval_ms & SCHEDULER_MS_MAX);

  // Start the task
  sched_task_start(p_task);
}

void sched_task_stop(sched_task_t *p_task) {

  // A pointer to the task to stop must be supplied.
  assert(p_task != NULL);

  /* Set the task as inactive but don't remove it from 
   * the que.  We don't need to clear the cached next task
   * since the active bit is checked before the task is used.
   */
  p_task->active = false;
}

bool sched_task_expired(sched_task_t *p_task) {
  assert(p_task != NULL);

  if (p_task->active) {
    return (scheduler_port_ms() - p_task->start_ms) >= p_task->interval_ms;
  } else {
    return false;
  }
}

uint32_t sched_task_remaining_ms(sched_task_t *p_task) {
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
uint32_t sched_task_elapsed_ms(sched_task_t *p_task) {
  assert(p_task != NULL && p_task->active);

  return scheduler_port_ms() - p_task->start_ms;
}

sched_task_t *sched_task_compare(sched_task_t *p_task_a, sched_task_t *p_task_b) {

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
static sched_task_t *sched_next_task(void) {

  // Pointer to the next expiring task.
  sched_task_t *p_expiring_task = NULL;

  /* Walk through each task in the linked list starting at the head task
   * and ending at the tail task.  Task access is read only so it's not
   * necessary to aquire exclusive accesss.
   */
  sched_task_t *p_current_task = scheduler.p_head;

  while (p_current_task != NULL) {
    if (p_current_task->active == true) {
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

  return p_expiring_task;
}

sched_task_t *sched_execute(void) {

  if (scheduler.state == SCHED_STATE_WAIT) {

    /* Skip executing the taks que if a cached next task 
     * is valid and hasn't expired yet. This check avoids 
     * having to check every task in the que for expiration.
     */
    if (TASK_ACTIVE_SAFE(scheduler.p_next) && 
        !TASK_EXPIRED(scheduler.p_next)) {
      return scheduler.p_next;
    }

    // Move to the active state.
    scheduler.state = SCHED_STATE_ACTIVE;

    // Loop through each task in the linked list starting with the head task
    sched_task_t *p_current_task = scheduler.p_head;

    while (p_current_task != NULL) {

      // Filter on active tasks with expired timers.
      if (p_current_task->active && TASK_EXPIRED(p_current_task)) {

        // Update the start time before calling the handler so the
        // handler's execution time doesn't introduce error into the
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
      p_current_task = p_current_task->p_next;
    }

    if (scheduler.state == SCHED_STATE_STOPPING) {
      /* Finishing stopping the scheduler if the stop function was
       * called while it was executing the task que. (active)
       */
      sched_stop_finalize();
    } else {
      // Calculate the next expiring task.
      scheduler.p_next = sched_next_task();

      // Move back to the wait state.
      scheduler.state = SCHED_STATE_WAIT;
    }
  } 

  // Note that the next task will be NULL for the stop states.
  return scheduler.p_next;
}

/**
 * Weak port implementation function definition to make it optional.
 */
__attribute__((weak)) void scheduler_port_init(void){
    // Empty
};

/**
 * Weak port implementation function definition to make it optional.
 */
__attribute__((weak)) void scheduler_port_deinit(void){
    // Empty
};