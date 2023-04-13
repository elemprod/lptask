/**
 * sched_types.h
 */

#ifndef SCHED_TYPES_H__
#define SCHED_TYPES_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/**
 * @brief Scheduler Task State Type.
 */
typedef enum {
  TASK_STATE_UNINIT   = 0,  // The task has not been initialized yet.
  TASK_STATE_STOPPED,       // The task has been added to the que but is inactive.
  TASK_STATE_ACTIVE,        // The task is active.
  TASK_STATE_EXECUTING,     // The task handler is executing.
  TASK_STATE_STOPPING,      // The task is in the proccess of stopping.
} sched_task_state_t;

/**
 * @brief The data structure for a single scheduler task.
 *
 * Note that task data structure should be defined with the SCHED_TASK_DEF() or 
 * the SCHED_BUFF_TASK_DEF() macros or allocated from a task pool.
 *
 * Scheduler tasks should only be accessed using the supplied scheduler
 * functions.
 *
 * Note that the allocated flag and task state must be volatile since they 
 * can be modified from interrupt contexts during the task's handler execution.
 * 
 * Unbuffered tasks will have a buff_size of 0 to indicate that they don't
 * have an internal buffer.
 * 
 * data_size represents the length of the actual data stored in the task and
 * should always be <= buff_size for buffered tasks.
 */
typedef struct _sched_task {

  uint32_t start_ms;            // The task start time (mS).
  uint32_t interval_ms;         // The task interval (mS).

  struct _sched_task *p_next;   // The next task in the linked list

  void * p_handler;             // The handler function (sched_handler_t).

  uint8_t *p_data;              // Pointer to the user data.

  uint8_t buff_size;            // Size of the internal data buffer. (bytes)

  uint8_t data_size;            // Size of the user's data. (bytes)

  bool repeat : 1;                        // Should the task repeat?
  volatile bool allocated: 1;             // Has the task been been allocated?
  volatile sched_task_state_t state : 4;  // The task's current state.

} sched_task_t;

/**
 * @brief Scheduler Task Handler Function Prototype.
 *
 * A task handler function is called at task interval expiration.
 *
 * @param[in] p_task      Pointer to the task.
 * @param[in] p_data      Pointer to the task data.
 * @param[in] data_size   Size of the task data. (bytes) 
 * 
 * @return none.
 */
typedef void (*sched_handler_t)(sched_task_t *p_task, void *p_data, uint8_t data_size);

/**
 * @brief Buffered task pool configuration structure.
 *
 * The task pool should be defined with the SCHED_TASK_POOL_DEF() macro.
 */
typedef struct {
  uint8_t *p_data;       // Pointer to the task pool data buffer.
  sched_task_t *p_tasks; // Pointer to the array of tasks in the pool.
  uint8_t buff_size;     // Size of the taks data buffer. (bytes)
  uint8_t task_cnt;      // The number of tasks in the pool.
  bool initialized : 1;  // Has the pool been initialized?
} sched_task_pool_t;

/**
 * @brief Macro for limiting a buffer size paramater to be greater than 0 and 
 * less than UINT8_MAX.
 */
#define SCHED_BUFF_LIMIT(value) (((value) > 1 ? (value) : 1) % UINT8_MAX)

/**
 * @brief Macro for limiting a task count paramater to be greater than 0 and 
 * less than UINT8_MAX.
 */
#define SCHED_TASK_LIMIT(value) (((value) > 1 ? (value) : 1) % UINT8_MAX)

#endif // SCHED_TYPES_H__