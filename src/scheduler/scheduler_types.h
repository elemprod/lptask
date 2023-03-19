/**
 * scheduler_types.h
 *
 */

#ifndef SCHEDULER_TYPES_H__
#define SCHEDULER_TYPES_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/**
 * Scheduler Task Handler Function Prototype.
 *
 * The handler function which is called at task interval
 * expiration.
 *
 * @param[in] p_data   Pointer to the task data.
 */
typedef void (*sched_handler_t)(void *p_data, uint8_t data_size);

/**
 * Scheduler Task State Type.
 *
 */
typedef enum {
  TASK_STATE_UNINIT   = 0,  // The task has not been initialized yet.
  TASK_STATE_STOPPED,       // The task has been added to the que but isn't active.
  TASK_STATE_ACTIVE,        // The task is active.
  TASK_STATE_EXECUTING,     // The task handler is executing.
  TASK_STATE_STOPPING,      // The task stop function was called while the handler was executing.
                            // The task will stop once it's handler returns.
} sched_task_state_t;

/**
 * The data structure for a single scheduler task.
 *
 * The task data structure should be defined and initialized with the
 * SCHED_TASK_DEF() or the SCHED_BUFF_TASK_DEF() macros.
 *
 * Scheduler tasks should only be modified using the supplied scheduler
 * functions and macros.
 *
 * Note that the allocated flag and task state must be volatile since they 
 * can be modified from interrupt contexts during handler is execution.
 */
typedef struct _sched_task {

  uint32_t start_ms;                      // The task start time (mS).
  uint32_t interval_ms;                   // The task interval (mS).

  struct _sched_task *p_next;             // Pointer to the next task in the linked list

  sched_handler_t handler;                // The task handler function.

  uint8_t *p_data;                        // Pointer to the user task data.

  uint8_t buff_size;                      // Size of the data buffer available for user data. (bytes)
                                          // 0 for unbuffered tasks.

  uint8_t data_size;                      // Size of the user data stored in the task. (bytes)

  bool repeat : 1;                        // Does the task repeat?
  volatile bool allocated: 1;             // Has the task been been allocated from the buffer pool?
  volatile sched_task_state_t state : 4;  // The current task state.

} sched_task_t;

/**
 * Scheduler Buffered Task Pool configuration structure.
 *
 * The scheduler task pool should be defined with the
 * SCHED_TASK_POOL_DEF() macro.
 *
 */
typedef struct {
  uint8_t *p_data;          // Pointer to the task pool data buffer.
  sched_task_t *p_tasks;    // Pointer to the array of pool tasks.
  uint8_t buff_size;        // Size of the data buffer available in each task. (bytes)  
  uint8_t task_cnt;         // The number of tasks in the pool.
  bool initialized : 1;     // Has the pool been initialized?
} sched_task_pool_t;

/**
 * Macro for limiting a buffer size paramater to be 
 * greater than 0 and less than UINT8_MAX.
 */
#define SCHED_BUFF_LIMIT(value) (((value) > 1 ? (value) : 1) % UINT8_MAX)

/**
 * Macro for limiting a task count paramater to be 
 * greater than 0 and less than UINT8_MAX.
 */
#define SCHED_TASK_LIMIT(value) (((value) > 1 ? (value) : 1) % UINT8_MAX)

#endif // SCHEDULER_TYPES_H__