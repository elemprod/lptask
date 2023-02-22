/**
 * scheduler_int.h
 *
 * Internal scheduler function declaration and defines which aren't typically
 * needed by end users.  The declaration's are placed here to reduce the 
 * complexity of the top level header while still giving end users access to
 * them.
 */

#ifndef SCHEDULER_INT_H__
#define SCHEDULER_INT_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "scheduler.h"
/**
 * The maximum task interval time in mS. (~6.2 days)
 */
#define SCHEDULER_MS_MAX 0x1FFFFFFF

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

/**
 * Function for checking if a task's timer has expired.
 *
 * Note that NULL or Inactive tasks return false since they
 * can't be expired.
 *
 * @param[in] p_task  Pointer to the task to stop to the scheduler
 * @return            true if task's timer has expired.
 */
bool sched_task_expired(sched_task_t *p_task);

/**
 * Function for calculating the time until a task's timer expires.
 *
 * Note that NULL or Inactive tasks will return SCHEDULER_MS_MAX.
 *
 * @param[in] p_task  Pointer to the task to stop to the scheduler
 * @return            The time in mS until the task expires or 0 if
 *                    the task has already expired.
 */
uint32_t sched_task_remaining_ms(sched_task_t *p_task);

/**
 * Function for calculating the time since a task's timer was started
 * or restarted in the case of a repeating timer.
 *
 * Note that NULL or Inactive tasks will be return 0.
 *
 * @param[in] p_task  Pointer to the task to stop to the scheduler
 * @return    The time in mS since the task was started.
 */
uint32_t sched_task_elapsed_ms(sched_task_t *p_task);

/**
 * Function for comparing the time until expiration of two tasks and returning
 * a pointer to one which expires sooner.
 *
 * @return    Returns a pointer to the task which will expire sooner or
 *            NULL if both tasks are NULL or Inactive.
 */
sched_task_t *sched_task_compare(sched_task_t *p_task_a, sched_task_t *p_task_b);

#endif // SCHEDULER_INT_H__