#include "scheduler.h"
#include "scheduler_config.h"
#include <assert.h>

/* The scheduler's interal data structure.
 */
typedef struct
{
  sched_task_t * p_head; // Pointer to the head task in the linked list
  sched_task_t * p_tail; // Pointer to the tail task in the linked list
} scheduler_t;

// The scheduler module's internal data.
static scheduler_t scheduler;

void sched_init(void) {
  // The only required initialization is to set the head and tail tasks to NULL.
  scheduler.p_head = NULL;
  scheduler.p_tail = NULL;
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

  // Add the task to the scheduler's que if itt hasn't been previously added.
  if (!p_task->added) {

    // The new task will be the last one in the list so it's next task will always be NULL.
    p_task->p_next = NULL;

    // Take exclusive write access of scheduler's task linked list.
    SCHED_CRITICAL_REGION_ENTER();

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
    SCHED_CRITICAL_REGION_EXIT();
  }
}

void sched_task_start(sched_task_t * p_task) {

  // A pointer to the task to start must be supplied.
  assert(p_task != NULL);

  // The task must have been previously added to the que.
  assert(p_task->added);

  // Store the start time.
  p_task->start_ms = sched_get_ms();

  // Mark the task as active
  p_task->active = true;
}

void sched_task_update(sched_task_t * p_task, uint32_t interval_ms) {

  // A pointer to the task to update must be supplied.
  assert(p_task != NULL);

  // Store the new period.
  p_task->interval_ms = interval_ms;

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
    return (sched_get_ms() - p_task->start_ms) >= p_task->interval_ms;
  } else {
    return false;
  }
}

uint32_t sched_task_remaining_ms(sched_task_t * p_task) {
  assert(p_task != NULL && p_task->active);

  uint32_t elapsed_ms = sched_get_ms() - p_task->start_ms;
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

  return sched_get_ms() - p_task->start_ms;
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

  while (p_current_task != NULL) {
    if (p_current_task->active == true) {
      if (p_expiring_task == NULL) {
        // No expiring task was previously set so this task is the next expiring task
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

  // Loop through each task in the linked list starting with the head task
  sched_task_t * p_current_task = scheduler.p_head;

  while (p_current_task != NULL) {

    // Store the pointer to next task since the current task could be removed
    // inside the task handler callback.
    sched_task_t * p_next_task = p_current_task->p_next;

    // Tasks should have alredy been marked as added.
    assert(p_current_task->added);

    // Filter on Active tasks with expired timers.
    if (p_current_task->active && sched_task_expired(p_current_task)) {

      // Update the start time before calling the handler to prevent the
      // the handler execution time from introducing error into the start time.
      p_current_task->start_ms = sched_get_ms();

      // Update the active flag before calling the handler to avoid overwriting
      // any active state changes made inside of the handler.
      p_current_task->active = p_current_task->repeat;

      // Execute the task Handler with the user supplied context pointer.
      p_current_task->handler(p_current_task->p_context);
    }

    // Move to the next task in the list
    p_current_task = p_next_task;
  }

  // The next expiring task needs to be calculated after the scheduler has fully executed since a task's
  // active status or its interval may have changed during event handler execution.

  return sched_next_task();
}