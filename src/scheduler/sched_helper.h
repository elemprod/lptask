/**
 * @file sched_helper.h
 * @author Ben Wirz
 * @brief Helper functions declarations and defines for the scheduler module.
 * 
 * The declaration's are placed in a separate header file to reduce the 
 * complexity of the schedulers top level header while still giving the end 
 * users access to them if needed.
 */ 

#ifndef SCHED_HELPER_H__
#define SCHED_HELPER_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include "scheduler.h"
#include "sched_types.h"

// The number of mS in one second.
#define SCHED_MS_SECOND     ((uint32_t) (1000))

// The number of mS in one minute.
#define SCHED_MS_MINUTE     ((uint32_t) (60 * SCHED_MS_SECOND))

// The number of mS in one hour.
#define SCHED_MS_HOUR       ((uint32_t) (60 * SCHED_MS_MINUTE))

// The number of mS in one day.
#define SCHED_MS_DAY        ((uint32_t) (24 * SCHED_MS_HOUR))

/**
 * @brief Function for converting a days, hours, minutes, seconds and mS 
 * interval into a scheduler task interval with units of miliseconds.
 *
 * @param[in] days    The number of days.
 * @param[in] hours   The number of hours.
 * @param[in] mins    The number of minutes.
 * @param[in] secs    The number of seconds. 
 * @param[in] ms      The number of miliseconds. 
 * 
 * @return The interval in mS corresponding to the values. 
 */
static inline uint32_t sched_ms(uint8_t days, 
                                uint8_t hours, 
                                uint8_t mins, 
                                uint8_t secs, 
                                uint8_t ms) {
  return ((days * SCHED_MS_DAY) + (hours * SCHED_MS_HOUR) + 
         (mins * SCHED_MS_MINUTE) + (secs * SCHED_MS_SECOND) + ms) % UINT32_MAX;
}

/**
 * @brief Function for checking if a task is buffered.
 *
 * @param[in] p_task   Pointer to the task.
 * 
 * @retval True if the task is buffered.
 * @retval False if the task is unbuffered or the task pointer is NULL.
 */
static inline bool sched_task_buffered(const sched_task_t *p_task) {
  if(p_task == NULL) {
    return false;
  } else {
    return (p_task->buff_size > 0);
  }
}

/**
 * @brief Function for checking if a task is active.
 *
 * @param[in] p_task   Pointer to the task.
 * 
 * @retval True if the task is in the active or executing state.
 * @retval False if the task is inactive or the task pointer is NULL.
 */
static inline bool sched_task_active(const sched_task_t *p_task) {
  if(p_task == NULL) {
    return false;
  } else {
    return (p_task->state == SCHED_TASK_ACTIVE) || 
          (p_task->state == SCHED_TASK_EXECUTING);
  }
}

/**
 * @brief Function for checking if a task's timer has expired.
 *
 * Note that NULL or Inactive tasks return false since they
 * can't be expired.
 *
 * @param[in] p_task  Pointer to the task to stop to the scheduler
 * 
 * @retval True if task's timer has expired.
 * @retval False if the task pointer was NULLor the task is unexpired.
 */
bool sched_task_expired(const sched_task_t *p_task);

/**
 * @brief Function for calculating the time until a task's timer expires.
 *
 * Note that NULL or Inactive tasks will return UINT32_MAX since
 * they will never expire.
 *
 * @param[in] p_task  Pointer to the task.
 * 
 * @retval The time in mS until the task expires.
 * @retval 0 if the task has already expired.
 * @retval SCHED_MS_MAX if the task pointer is NULL or the task is inactive.         
 */
uint32_t sched_task_remaining_ms(const sched_task_t *p_task);

/**
 * @brief Function for calculating the time since a task's timer was started.
 * 
 * Note that repeating tasks are restarted at each task handler call.  The 
 * returned value will be the time since the last handler call for a repeating
 * task which has previously expired.
 * 
 * @param[in] p_task  Pointer to the task to stop to the scheduler
 *
 * @retval The time in mS since the task was started.
 * @retval 0 if task pointer is NULL or the task is inactive.
 */
uint32_t sched_task_elapsed_ms(const sched_task_t *p_task);

/**
 * @brief Function for comparing the expiration time of two tasks and returning
 * a pointer to the one which expires sooner.
 * 
 * @param[in] p_task_a  Pointer to the first task to compare.
 * @param[in] p_task_b  Pointer to the second task to compare.
 *  
 * @retval A pointer to the task which expires sooner.
 * @retval NULL if both task pointers are NULL or both tasks are inactive.
 */
sched_task_t *sched_task_compare(const sched_task_t *p_task_a, 
                                 const sched_task_t *p_task_b);

/**
 * @brief Function for determining the number of allocated tasks for a scheduler 
 * task pool.
 * 
 * @param[in] p_pool  Pointer to pool configuration structure.
 * 
 * @return The number of tasks which are currently allocated.
 */
uint8_t sched_pool_allocated(const sched_task_pool_t * p_pool);

/**
 * @brief Function for determining the number of unallocated tasks for a 
 * scheduler task pool.
 * 
 * @param[in] p_pool  Pointer to pool configuration structure.
 * 
 * @return The number of tasks which are currently unallocated.
 */
uint8_t sched_pool_free(const sched_task_pool_t * p_pool);

/**
 * @brief Function for getting a scheduler task's state.
 *
 * @param[in] p_task  Pointer to the task.
 * 
 * @retval  The current task state.
 * @retval  SCHED_TASK_UNINIT if the task pointer is NULL.
 */
static inline sched_task_state_t sched_task_state(const sched_task_t *p_task) {
  if(p_task == NULL) {
    return SCHED_TASK_UNINIT;
  } else {
    return p_task->state;
  }
}

#endif // SCHED_HELPER_H__
