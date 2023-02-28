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
  if (scheduler.state == SCHED_STATE_STOPPING) {

    // Clear the que.
    sched_clear_que();

    // Perform any platform specific deinitialization last.
    scheduler_port_deinit();

    scheduler.state = SCHED_STATE_STOPPED;
  }
}

void sched_stop(void) {
  if (scheduler.state != SCHED_STATE_STOPPED) {
    scheduler.state = SCHED_STATE_STOPPING;
  }
}

void sched_task_config(sched_task_t *p_task, sched_handler_t handler,
    uint32_t interval_ms, bool repeat) {

  // A pointer to the task to add must be supplied.
  SCHED_NULL_CHECK(p_task);

  // A task handler must be supplied.
  SCHED_NULL_CHECK(handler);

  // Mark the task as inactive.
  p_task->active = false;

  // Store the task handler.
  p_task->handler = (sched_handler_t)handler;

  // Store the task interval limiting it to the max interval.
  p_task->interval_ms = (interval_ms & SCHED_MS_MAX);

  // Store the repeating status.
  p_task->repeat = repeat;

  // Add the task to the scheduler's que if it hasn't been previously added.
  if (!p_task->added) {

    // The new task will be the last one in the list so it's next task will always be NULL.
    p_task->p_next = NULL;

    // Take exclusive write access of scheduler's task que.
    scheduler_port_que_lock();

    if (scheduler.p_head == NULL) {
      // No other tasks exists in the list so the new task will be the head & tail tasks.
      scheduler.p_head = p_task;
    } else {
      // If other tasks exist, this task will be the next task for the current tail task.
      assert(scheduler.p_tail != NULL);
      scheduler.p_tail->p_next = p_task;
    }
    // Set the new task to the tail task which adds it to the end of the list.
    scheduler.p_tail = p_task;

    // Release the task que exclusive access.
    scheduler_port_que_free();

    p_task->added = true;
  }
}

void sched_task_start(sched_task_t *p_task) {

  // A pointer to the task to start must be supplied.
  SCHED_NULL_CHECK(p_task);

  // The task must have been previously added to the que to start it.
  if (p_task->added) {

    // Store the start time.
    p_task->start_ms = scheduler_port_ms();

    // Mark the task as active.
    p_task->active = true;

    /* Clear the reference to the cached next task since
     * it may no longer be valid.
     */
    scheduler.p_next = NULL;
  }
}

void sched_task_update(sched_task_t *p_task, uint32_t interval_ms) {

  // A pointer to the task to update must be supplied.
  SCHED_NULL_CHECK(p_task);

  // Store the new interval limiting it to the max interval.
  p_task->interval_ms = (interval_ms & SCHED_MS_MAX);

  // Start the task
  sched_task_start(p_task);
}


uint8_t sched_task_data(sched_task_t * p_task, void * p_data, uint8_t data_size) {
  // A pointer to the task must be supplied and data can only be
  // added to inactive tasks.
  if(p_task == NULL || p_task->executing) {
    return 0;
  }

  // Set the data size.
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

void sched_task_stop(sched_task_t *p_task) {

  // A pointer to the task to stop must be supplied.
  SCHED_NULL_CHECK(p_task);

  /* Set the task as inactive but don't remove it from
   * the que.  The repeating flag is also cleared so that
   * the task isn't restarted if the function is called 
   * while the task que is being executed.
   *
   * Don't need to clear the cached next task since 
   * the active bit will be checked before the task
   * is executed.
   */
  p_task->repeat = false;
  p_task->active = false;
}

bool sched_task_expired(sched_task_t *p_task) {

  if (p_task == NULL || (p_task->active == false)) {
    return false;
  } else {
    return (scheduler_port_ms() - p_task->start_ms) >= p_task->interval_ms;
  }
}

uint32_t sched_task_remaining_ms(sched_task_t *p_task) {

  if (p_task == NULL || (p_task->active == false)) {
    return SCHED_MS_MAX;
  } else {

    uint32_t elapsed_ms = scheduler_port_ms() - p_task->start_ms;

    if (elapsed_ms < p_task->interval_ms) {
      return p_task->interval_ms - elapsed_ms;
    } else {
      // Expired
      return 0;
    }
  }
}

uint32_t sched_task_elapsed_ms(sched_task_t *p_task) {
  if (p_task == NULL || (p_task->active == false)) {
    return 0;
  } else {
    return scheduler_port_ms() - p_task->start_ms;
  }
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
   * necessary to acquire exclusive access.
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

/**
 * Function for executing scheduled tasks in the que which have expired timers.
 *
 * This function must be called from within the main loop. It will execute
 * all scheduled tasks having expired timers before returning.
 *
 * @return  none
 */
void sched_execute_que(void) {

  /* Skip executing the task que if the cached next task
   * is valid and has not expired yet. This check avoids
   * having to test every task in the que for expiration.
   */
  if (TASK_ACTIVE_SAFE(scheduler.p_next) &&
      !TASK_EXPIRED(scheduler.p_next)) {
    return;
  }

  // Loop through each task in the linked list starting with the head task
  sched_task_t *p_current_task = scheduler.p_head;

  while (p_current_task != NULL) {

    // Filter on active tasks with expired timers.
    if (p_current_task->active && TASK_EXPIRED(p_current_task)) {

      // Mark the task as executing so the task data can not be 
      // changed in the middle of the handler call.
      p_current_task->executing = true;

      /* Set the start time before calling the handler so the
       * handler's execution time doesn't introduce error into the
       * start time calculation.
       */
      p_current_task->start_ms = scheduler_port_ms();

      /* Update the active flag before calling the handler to avoid overwriting
       * any active flag changes made inside of the handler.  The active flag 
       * would change if the task was started again inside the handler for example.
       * A task will only be active at this point if it is a repeating task.
       */
      p_current_task->active = p_current_task->repeat;

      // Call the task Handler.
      p_current_task->handler(p_current_task->p_data, p_current_task->data_size);

      // Clear the executing state.
      p_current_task->executing = false;
    }

    // Move to the next task in the list
    p_current_task = p_current_task->p_next;
  }

  // Cache the next expiring task.
  scheduler.p_next = sched_next_task();
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
  // Repeatably execute tasks in the que with expired timers.
  while (scheduler.state == SCHED_STATE_ACTIVE) {

    // Execute any tasks in the que with expired timers.
    sched_execute_que();

    // Calculate the time interval until the next expiring task.
    uint32_t sleep_interval = sched_task_remaining_ms(scheduler.p_next);

    // Sleep until the the task expires using the platform specific sleep method.
    if (sleep_interval > 0) {
      scheduler_port_sleep(sleep_interval);
    }
  }

  // Finish stopping the scheduler before returning.
  sched_stop_finalize();
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