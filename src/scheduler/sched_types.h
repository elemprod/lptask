/**
 * @file sched_types.h
 * @author Ben Wirz
 * @brief Scheduler module custom data types.
 *
 */

#ifndef SCHED_TYPES_H__
#define SCHED_TYPES_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief A type representing a scheduler task state.
 *
 * @note The enum values need to be explicitly set since they are stored as
 * a nibble sized bit-field.
 */
typedef enum
{
  /// @brief The task has not been initialized yet.
  SCHED_TASK_UNINIT = 0x0,
  /// @brief The task has been initialized but it hasn't been started.
  SCHED_TASK_STOPPED = 0x1,
  /// @brief The task is active
  SCHED_TASK_ACTIVE = 0x2,
  /// @brief The task's handler is executing.
  SCHED_TASK_EXECUTING = 0x3,
  /**
   * @brief The task is in the proccess of stopping.
   *
   * The task enters the SCHED_TASK_STOPPING state if the sched_task_stop()
   * function is called while the task is executing its handler.  The task
   * moves to the SCHED_TASK_STOPPED state once the handler returns.
   */
  SCHED_TASK_STOPPING = 0x5
} sched_task_state_t;

/**
 * @brief A data structure for a single scheduler task.
 *
 * The task data structure should be defined with the SCHED_TASK_DEF() or
 * the SCHED_TASK_BUFF_DEF() macros or be allocated with the sched_task_alloc()
 * function from a task pool.  The task structure should only be accessed
 * using the supplied scheduler functions.
 *
 * @note The allocated flag and state variables must be volatile since they
 * can be modified from different contexts.
 */
typedef struct _sched_task
{

  /// @brief The task start time (mS).
  uint32_t start_ms;

  /// @brief The task interval (mS).
  uint32_t interval_ms;

  /// @brief The next task in the linked list.
  struct _sched_task *p_next;

  /**
   * @brief Pointer to the task's handler function.
   * @sa sched_handler_t
   */
  void *p_handler;

  /// @brief Pointer to the user data.
  uint8_t *p_data;

  /**
   * @brief Size of the internal data buffer. (bytes)
   *
   * Note that unbuffered tasks will have a buff_size of 0 to indicate that
   * they don't include an internal buffer.
   */
  uint8_t buff_size;

  /**
   * @brief Size of the stored user's data. (bytes)
   *
   * For buffered tasks, data_size represents the length of the actual data
   * stored in the task and will always be less than or equal to buff_size.
   */
  uint8_t data_size;

  /// @brief Is the task repeating?
  bool repeat : 1;

  /// @brief Has the task been been allocated?  Only used for task pools.
  volatile bool allocated : 1;

  /// @brief The task's current state.
  volatile sched_task_state_t state : 4;

} sched_task_t;

/**
 * @brief Scheduler Task Handler Function Prototype.
 *
 * A task's handler function will be called after task interval expiration.
 *
 * @param[in] p_task      Pointer to the task.
 * @param[in] p_data      Pointer to the task data.
 * @param[in] data_size   Size of the task data. (bytes)
 */
typedef void (*sched_handler_t)(sched_task_t *p_task, void *p_data, uint8_t data_size);

/**
 * @brief Buffered task pool configuration structure.
 *
 * A task pool should be defined with the SCHED_TASK_POOL_DEF() macro.
 */
typedef struct
{
  /// @brief Pointer to the shared pool data buffer.
  uint8_t *p_data;
  /// @brief Pointer to the array of tasks in the pool.
  sched_task_t *p_tasks;
  /// @brief Size of the data buffer for each task. (bytes)
  uint8_t buff_size;
  /// @brief The number of tasks in the pool.
  uint8_t task_cnt;
  /// @brief Has the pool been initialized?
  bool initialized : 1;
} sched_task_pool_t;

/**
 * @brief Macro for selecting the smaller of two numbers.
 *
 * @param[in] a The first number to compare.
 * @param[in] b The second number to compare.
 */
#define SCHED_MIN(a, b) (((a) < (b)) ? (a) : (b))

/**
 * @brief Macro for selecting the larger of two numbers.
 *
 * @param[in] a The first number to compare.
 * @param[in] b The second number to compare.
 */
#define SCHED_MAX(a, b) (((a) > (b)) ? (a) : (b))

/**
 * @brief Macro for limiting a buffer size parameter to the valid range.
 *
 * Buffer sizes are limited to be greater than 0 and less than UINT8_MAX.
 *
 * @param[in] value The buffer size value to limit.
 */
#define SCHED_BUFF_LIMIT(value) ((uint8_t)SCHED_MIN(SCHED_MAX(value, 1), UINT8_MAX))

/**
 * @brief Macro for limiting a task count parameter to the valid range.
 *
 * Task counts are limited to be greater than 0 and less than UINT8_MAX.
 *
 * @param[in] value The task count value to limit.
 */
#define SCHED_TASK_LIMIT(value) ((uint8_t)SCHED_MIN(SCHED_MAX(value, 1), UINT8_MAX))

#ifdef __cplusplus
}
#endif

#endif // SCHED_TYPES_H__
