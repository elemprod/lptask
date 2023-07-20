/**
 * @file scheduler.h
 * @author Ben Wirz
 *
 * @brief The module is a cooperative task scheduler implementation which
 * provides an easy-to-use mechanism for scheduling tasks to be executed in
 * the future without the complexity or overhead of an operating system.  Once
 * scheduled, a task's handler is executed from the main context once its
 * interval timer expires.
 */

#ifndef SCHEDULER_H__
#define SCHEDULER_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "sched_types.h"
#include "sched_port.h"
#include "sched_helper.h"

/**
 * @brief Macro for defining an unbuffered scheduler task.
 *
 * An unbuffered scheduler task does not contain an internal buffer for user
 * data, data is added to unbuffered tasks by reference rather than by copy.
 * The data referenced by an unbuffered task must still be valid when the task
 * handler is called at later point in time.
 *
 * An unbuffered task can also be used for tasks which do not require that
 * data be passed to their handlers.  For example, an LED blink task may not
 * need to store task data since the LED output can simply be inverted during
 * each handler call.
 *
 * @note Since the macro statically allocates a task, it should only be invoked
 * once per task.
 *
 * @param[in] TASK_ID  Unique task name.
 */
#define SCHED_TASK_DEF(TASK_ID)   \
  static sched_task_t TASK_ID = { \
      .p_data = NULL,             \
      .buff_size = 0,             \
      .data_size = 0,             \
      .repeat = false,            \
      .allocated = false,         \
      .state = SCHED_TASK_UNINIT, \
  }

/**
 * @brief Macro for defining a single buffered scheduler task.
 *
 * A buffered scheduler task and an internal data buffer are defined by the
 * macro. The task data buffer is used to pass a copy of the user supplied data
 * to the task handler.  The buffer size is configurable on per task basis
 * and should be set the maximum data size which will be stored in the task.
 *
 * @note Since this macro statically allocates a task and its data buffer, it
 * should only be invoked once per task.
 *
 * @param[in] TASK_ID     Unique task name.
 * @param[in] BUFF_SIZE   The maximum buffer size available to store task data.
 *                        1 to 255 (bytes)
 */
#define SCHED_TASK_BUFF_DEF(TASK_ID, BUFF_SIZE)               \
  static uint8_t TASK_ID##_BUFF[SCHED_BUFF_LIMIT(BUFF_SIZE)]; \
  static sched_task_t TASK_ID = {                             \
      .p_data = TASK_ID##_BUFF,                               \
      .buff_size = SCHED_BUFF_LIMIT(BUFF_SIZE),               \
      .data_size = 0,                                         \
      .repeat = false,                                        \
      .allocated = false,                                     \
      .state = SCHED_TASK_UNINIT,                             \
  }

/**
 * @brief Macro for defining a pool of buffered scheduler tasks.
 *
 * In addition to defining the pool structure, an array of buffered scheduler
 * tasks and their internal data buffers are defined. The internal buffer is
 * used to pass a copy of the user supplied data to the task handlers.
 *
 * All tasks in the pool have the same length data buffer. The buffer length
 * should be set to the maximum data size which any task will need to store.
 * The length of the data stored at each sched_task_data() function call can
 * be variable provided it is less than the defined buffer size.
 *
 * @note Since this macro statically allocates an array of tasks, a data buffer
 * and a task pool structure, it should only be invoked once per task pool.
 *
 * @param[in] POOL_ID       Unique name for the pool of tasks.
 * @param[in] BUFF_SIZE     The maximum buffer size available for task data.
 *                          1 to 255 (bytes)
 * @param[in] TASK_CNT      The number of tasks available in the pool.
 *                          1 to 255 (tasks)
 */
#define SCHED_TASK_POOL_DEF(POOL_ID, BUFF_SIZE, TASK_CNT)          \
  static uint8_t POOL_ID##_BUFF[SCHED_TASK_LIMIT(TASK_CNT) *       \
                                SCHED_BUFF_LIMIT(BUFF_SIZE)];      \
  static sched_task_t POOL_ID##_TASKS[SCHED_TASK_LIMIT(TASK_CNT)]; \
  static sched_task_pool_t POOL_ID = {                             \
      .p_data = POOL_ID##_BUFF,                                    \
      .p_tasks = POOL_ID##_TASKS,                                  \
      .buff_size = SCHED_BUFF_LIMIT(BUFF_SIZE),                    \
      .task_cnt = SCHED_TASK_LIMIT(TASK_CNT),                      \
      .initialized = false}

/**
 * @brief Function for allocating a buffered scheduler task from a task pool.
 *
 * A task pool serves as a simple mechanism for creating and tracking
 * multiple reusable scheduler tasks.
 *
 * Once allocated, a task is configured and accessed in the same way a normal
 * buffered task is.  A task remains allocated until it is stopped either due
 * to task expiration and subsequent handler return for a non-repeating task or
 * if it is stopped by the sched_task_stop() function for repeating task. Once
 * the task stops, it is returned to the task pool and will be available for
 * reuse at the next sched_task_alloc() call.  Allocated task should be
 * configured before use.
 *
 * @param[in] p_pool  Pointer to the task pool structure.
 *
 * @retval A pointer to the allocated task.
 * @retval NULL if no free tasks are available which would typically indicating
 *         that the pool's task count needs to be increased.
 */
sched_task_t *sched_task_alloc(sched_task_pool_t *p_pool);

/**
 * @brief Function for configuring or reconfiguring a scheduler task.
 *
 * The configuration function can only be used tasks on tasks which have
 * stopped.  The function can be used to reconfigure a previously configured
 * task but the task stop must complete before doing so.
 *
 * The scheduler must be initialized prior to configuring a task.
 *
 * The task interval for a repeating task is the desired time in mS between
 * task handler calls.   The interval for non-repeating task is the time
 * delay from now until the task handler is called. An interval of 0 will
 * result in handler being called as soon as possible.
 *
 * @param[in] p_task        Pointer to the task.
 * @param[in] handler       Task handler function.
 * @param[in] interval_ms   The task interval (mS)
 * @param[in] repeat        True for a repeating tasks or False for single
 *                          shot tasks.
 *
 * @retval True if the configuration succeeded.
 * @retval False if the configuration failed because the task's has not been
 *         stopped, the task pointer was NULL, the task handler was NULL or
 *         the scheduler has not been initialized.
 */
bool sched_task_config(sched_task_t *p_task,
                       sched_handler_t handler,
                       uint32_t interval_ms,
                       bool repeat);

/**
 * @brief Function for updating a task's user data.
 *
 * A pointer to the task data and the data's size are supplied to the task's
 * handler function at task expiration.  The handling of the data is different
 * depending on the type of task as described below.
 *
 * Unbuffered Tasks:
 *
 * Data is added to unbuffered tasks by reference.  The supplied data pointer
 * and data size are stored in the task but the actual data is not copied into
 * the task since unbuffered tasks don't have internal data buffers.  The
 * referenced task data must still be valid when the task's handler is called.
 *
 * Buffered Tasks:
 *
 * The user data at the supplied pointer address is copied to the task's
 * internal memory during sched_task_data() function calls for buffered tasks.
 * A pointer to the task's internal data buffer will then supplied to the
 * handler along with the data_size once the task expires.  Note that data_size
 * is limited to be less than or equal to the task's buffer size which was was
 * supplied to the SCHED_TASK_BUFF_DEF() macro.
 *
 * A task must be stopped before its data can be updated to avoid potential
 * data access conflicts. Attempts to update a task's data which is not
 * currently stopped will return 0 indicating that the task data was not
 * updated. The task data would typically be set just before the task is
 * started.
 *
 * @param[in] p_task        Pointer to the task.
 * @param[in] p_data        Pointer to the user data to add to the task.
 * @param[in] data_size     The length of the user data (bytes).
 *
 * @retval The size of the data copied into a buffered task which may be less
 *         than data_size if data_size exceeds the task's buffer size.
 * @retval The unaltered data size value stored in for an unbuffered task.
 * @retval 0 if no bytes were copied due to the supplied to the task pointer
 *         being NULL, the task being unconfigured or if the task was not
 *         in the stopped state when the function was called.
 */
uint8_t sched_task_data(sched_task_t *p_task,
                        const void *p_data,
                        uint8_t data_size);

/**
 * @brief Function for updating a task with a new interval and starting it.
 *
 * Note that a task must have been previously configured before its interval
 * can be updated.
 *
 * @param[in] p_task        Pointer to the task to add to the scheduler
 * @param[in] interval_ms   Task interval for a repeating task or a delay for
 *                          single-shot task (mS).
 *
 * @retval True if the interval was successfully updated.
 * @retval False if the interval could not be updated because it was not
 *         previously configured or the task pointer was NULL.
 */
bool sched_task_update(sched_task_t *p_task, uint32_t interval_ms);

/**
 * @brief Function for starting a scheduler task.
 *
 * A task must have been previously configured with the sched_task_config()
 * function.
 *
 * @note Repeatably restarting a task inside its own handler with an
 * interval of 0 mS may starve the other tasks of CPU time and should be used
 * with caution.
 *
 * @param[in] p_task  Pointer to the task to add to the scheduler
 *
 * @retval True if the task was successfully started.
 * @retval False if the task could not be started because it has not previously
 *         been configured or the task pointer was NULL.
 */
bool sched_task_start(sched_task_t *p_task);

/**
 * @brief Function for stopping a scheduler task.
 *
 * The task's handler will finish execution and return if it is currently
 * running after which the the task will be stopped.
 *
 * @param[in] p_task  Pointer to the task to stop.
 *
 * @retval True if the task was successfully stopped.
 * @retval False if the task could not be stopped because it has not previously
 *         been configured or the task pointer was NULL.
 */
bool sched_task_stop(sched_task_t *p_task);

/**
 * @brief Function for initializing the scheduler module.
 *
 * Note that if the scheduler module was previously started and then stopped,
 * this function should not be called until the stop completes as indicated
 * by the sched_start() function returning.
 */
void sched_init(void);

/**
 * @brief Function for starting the scheduler.
 *
 * The function repeatably executes scheduled tasks as they expire.
 * This function must be called from the main context, typically after
 * all platform initialization has completed. The function does not
 * return, once called, until the scheduler is stopped.
 */
void sched_start(void);

/**
 * @brief Function for stopping the scheduler module.
 *
 * Note the function call may not immediately stop the scheduler. The scheduler
 * will finish executing any expired task before completing the stop.
 */
void sched_stop(void);

#endif // SCHEDULER_H__
