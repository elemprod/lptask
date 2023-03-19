/**
 * scheduler_int.h
 *
 * Internal scheduler function declarations and macros which aren't 
 * typically ussed by end users.  The declaration's are placed here 
 * to reduce the complexity of the top level header while still 
 * giving end users access to them if needed.
 */

#ifndef SCHEDULER_INT_H__
#define SCHEDULER_INT_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include "scheduler.h"

/**
 * The maximum task interval time in mS. (~6.2 days)
 */
#define SCHED_MS_MAX 0x1FFFFFFF

/*
 * If SCHED_TASK_BUFF_CLEAR is defined to be != 0, the task 
 * data buffer will be cleared each time the task is allocated.  
 * The default implementation is to clear the buffer but the 
 * end user can overide this by defining SCHED_TASK_BUFF_CLEAR 
 * to be 0 if desired.  Clearing a large task data buffer can
 * be costly and may be unnecessary in some situations.
 */
#ifndef SCHED_TASK_BUFF_CLEAR
#define SCHED_TASK_BUFF_CLEAR 1
#endif

/**
 * Macro for checking if a task is buffered.
 *
 * Note: The task pointer is not NULL checked
 *
 *  @param[in] p_task   Pointer to the task.
 *  @return             True if the task is buffered.
 *                      False if the task is unbuffered.
 */
#define TASK_BUFFERED(p_task) ((p_task)->buff_size > 0)

/**
 * Macro for checking if a task is active.
 *
 * Note: The task pointer is not NULL checked
 *
 *  @param[in] p_task   Pointer to the task.
 *  @return             True if the task is active else False.
 */
#define TASK_ACTIVE(p_task) (((p_task)->state == TASK_STATE_ACTIVE) || ((p_task)->state == TASK_STATE_EXECUTING))

/**
 * Macro for safely checking if a task is active.
 *
 *  @param[in] p_task   Pointer to the task.
 *  @return             True if the task is active.
 *                      False if the task pointer is NULL 
 *                      or the tassk is inactive.
 */
#define TASK_ACTIVE_SAFE(p_task) (((p_task) != NULL) && TASK_ACTIVE(p_task))

/**
 * Macro for checking if a task has expired.
 *
 * Note: The task pointer is not NULL checked and the
 * task's Active status is also not checked.
 *
 *  @param[in] p_task   Pointer to the task.
 *  @return             True if the task is expired else False.
 */
#define TASK_EXPIRED(p_task) ((scheduler_port_ms() - (p_task)->start_ms) >= (p_task)->interval_ms)

/**
 * Macro for safely checking if a task has expired.
 *
 *  @param[in] p_task   Pointer to the task.
 *  @return             True if the task is active and expired.
 *                      False if the task is expired, inactive or NULL.
 */
#define TASK_EXPIRED_SAFE(p_task) (TASK_ACTIVE_SAFE(p_task) && TASK_EXPIRED(p_task))

/**
 * Function for checking if a task's timer has expired.
 *
 * Note that NULL or Inactive tasks return false since they
 * can't be expired.
 *
 * @param[in] p_task  Pointer to the task to stop to the scheduler
 * @return            True if task's timer has expired.
 *                    False if the task pointer is NULL or the 
 *                    task is unexpired.
 */
bool sched_task_expired(sched_task_t *p_task);

/**
 * Function for calculating the time until a task's timer expires.
 *
 * Note that NULL or Inactive tasks will return SCHED_MS_MAX.
 *
 * @param[in] p_task  Pointer to the task.
 * @return            The time in mS until the task expires.
 *                    SCHED_MS_MAX if the task pointer is NULL or 
 *                    the task is inactive.
 */
uint32_t sched_task_remaining_ms(sched_task_t *p_task);

/**
 * Function for calculating the time since a task's timer was started
 * or restarted in the case of a repeating timer.
 *
 * Note that NULL or Inactive tasks will be return 0.
 *
 * @param[in] p_task  Pointer to the task to stop to the scheduler
 *
 * @return    The time in mS since the task was started.
 *            0 if task pointer is NULL or the task is 
 *            inactive.
 */
uint32_t sched_task_elapsed_ms(sched_task_t *p_task);

/**
 * Function for comparing the expiration time of two tasks and returning
 * a pointer to one which expires sooner.
 *
 * @return    Returns a pointer to the task which will expire sooner or
 *            NULL if both tasks are NULL or both task are inactive.
 */
sched_task_t *sched_task_compare(sched_task_t *p_task_a, sched_task_t *p_task_b);

#endif // SCHEDULER_INT_H__