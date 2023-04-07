/**
 * scheduler.h
 *
 * The scheduler module provides a simple method for scheduling tasks 
 * to be executed at a later time. 
 */

#ifndef SCHEDULER_H__
#define SCHEDULER_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "scheduler_types.h"
#include "scheduler_port.h"
#include "sched_helper.h"

/**
 * Macro for defining an unbuffered scheduler task.
 *
 * An unbuffered scheduler task does not contain an internal buffer 
 * for user data. Data is added to unbuffered tasks by reference 
 * rather than by copy.  The is different than the operation of 
 * buffered task which copies and stores the user data to its
 * internal buffer.
 *
 * Note that the data referenced by an unbuffered task needs 
 * to still be valid when the task handler is called at later
 * point in time.
 *
 * An unbuffered task can also be used for tasks which do 
 * not need to pass data to their handlers.  For example,
 * an LED blink task may not need to store any data 
 * internally.  It can simple invert the LED GPIO output on
 * call.
 * 
 * @param[in] TASK_ID  Unique task name.
 */
#define SCHED_TASK_DEF(TASK_ID)       \
  static sched_task_t TASK_ID = {     \
      .p_data = NULL,                 \
      .buff_size = 0,                 \
      .data_size = 0,                 \
      .repeat = false,                \
      .allocated = false,             \
      .state = TASK_STATE_UNINIT,     \
  } 

/**
 * Macro for defining a buffered scheduler task.
 *
 * A buffered scheduler task and an internal data buffer
 * are defined. The buffer is used to pass a copy of the 
 * user supplied data to the task handler.  The buffer 
 * size is configurable on per task basis and should be 
 * set the maximum data size which will be stored in the task.
 *
 * @param[in] TASK_ID     Unique task name.
 * @param[in] BUFF_SIZE   The maximum buffer size available to the task.
 *                        0 to UINT8_MAX (bytes)
 */
#define SCHED_TASK_DEF_BUFF(TASK_ID, BUFF_SIZE)               \
  static uint8_t TASK_ID##_BUFF[SCHED_BUFF_LIMIT(BUFF_SIZE)]; \
  static sched_task_t TASK_ID = {                             \
      .p_data = TASK_ID##_BUFF,                               \
      .buff_size = SCHED_BUFF_LIMIT(BUFF_SIZE),               \
      .data_size = 0,                                         \
      .repeat = false,                                        \
      .state = TASK_STATE_UNINIT,                             \
  }                                

/**
 * Macro for defining a pool of buffered scheduler tasks.
 *
 * A buffered scheduler task and an internal data buffer
 * are defined. The buffer is used to pass a copy of the 
 * user supplied data to the task handler.
 *
 * All tasks in the pool have the same length data buffer.
 * The buffer lengths should be set to the maximum data size
 * which will be stored in the task.
 *
 * @param[in] POOL_ID       Unique name for the pool of tasks.
 * @param[in] BUFF_SIZE     The maximum buffer size available for each task.
 *                          0 to UINT8_MAX (bytes)
 * @param[in] TASK_CNT      The number of tasks available in the pool.
 *                          0 to UINT8_MAX (tasks)
 */
#define SCHED_TASK_POOL_DEF(POOL_ID, BUFF_SIZE, TASK_CNT)                                   \
  static uint8_t POOL_ID##_BUFF[SCHED_TASK_LIMIT(TASK_CNT) * SCHED_BUFF_LIMIT(BUFF_SIZE)];  \
  static sched_task_t POOL_ID##_TASKS[SCHED_TASK_LIMIT(TASK_CNT)];                          \
  static sched_task_pool_t POOL_ID = {                                                      \
    .p_data = POOL_ID##_BUFF,                                                               \
    .p_tasks = POOL_ID##_TASKS,                                                             \
    .buff_size = SCHED_BUFF_LIMIT(BUFF_SIZE),                                               \
    .task_cnt = SCHED_TASK_LIMIT(TASK_CNT),                                                 \
    .initialized = false                                                                    \
  }

/**
 * Function for allocating a single buffered scheduler task from the task pool.
 * The pool serves as a simple mechanism for creating and tracking multiple
 * reusable tasks.
 * 
 * Once allocated, the task can be configured and accessed in the same
 * way a normal task would be used.  A task remains allocated until it is stopped 
 * either due to task expiration and subsequent handler return for a non-repeating 
 * task or if it is stopped for a repeating task. Once the task stops, it returns 
 * to the task pool and will be available for reuse at the next sched_task_alloc() 
 * call.
 * 
 * 
 * @param[in] p_pool  Pointer to pool configuration structure.
 * @return            The allocated task or NULL if no free tasks are available.
 *                    A NULL return would typically indicate that the pool task 
 *                    count needs to be increased.
 */
sched_task_t * sched_task_alloc(sched_task_pool_t * const p_pool);


/**
 * Function for configuring or reconfiguring a scheduler task.
 *
 * The task will be stopped after calling sched_task_config() and it
 * must started with with sched_task_start() function before it
 * becomes active.
 *
 * The task interval for a repeating task is the desired time in mS between 
 * task handler calls.   The interval for non-repeating task is the time
 * delay from now until the task handler is called. An interval of 0 will 
 * result in handler being called as soon as possible.
 *
 * The function can also be used to reconfigure a previously configured task.
 * The scheduler must be initialized prior to calling the function and the 
 * task's handler must not currently executing when the function is called.
 *
 * @param[in] p_task        Pointer to the task.
 * @param[in] handler       Task handler function.
 * @param[in] interval_ms   The task interval (mS)
 * @param[in] repeat        True for a repeating tasks.
 *                          False for single shot tasks.
 *
 * @return                  True if the configuration succeeded.
 *
 *                          False if the configuration failed because the 
 *                          task handler is currently executing, the task
 *                          pointer was NULL or the task handler was NULL.
 */
bool sched_task_config(sched_task_t *p_task,
    sched_handler_t handler,
    uint32_t interval_ms,
    bool repeat);

/**
 * Function for updating a task's user data.
 *
 * A pointer to the task data and the data size will be supplied
 * to the task's handler function at task expiration.  The operation
 * of the function is different depending on the type of task as 
 * described below.
 *
 * Unbuffered Tasks:
 *
 * Data is added to unbuffered tasks by reference.  The 
 * supplied data pointer and data size are stored in the task but
 * the actual data not copied into the task since unbuffered 
 * tasks don't have internal buffers.  The referenced task 
 * data must still be valid when the task's handler is called.  
 *
 * Buffered Tasks:
 *
 * The user data at the supplied pointer address  is copied to 
 * the task's internal memory during sched_task_data() calls for 
 * buffered tasks.  A pointer to the task's internal
 * data buffer is then supplied to the handler along with the data_size
 * after the task expires.  
 * Note that data_size is limited to 
 * be less than or equal to the task's buffer size which was was
 * supplied to the SCHED_TASK_DEF_BUFF() macro.
 * 
 * Note that a task must be stopped before it's data can be updated 
 * to avoid potential data access conflicts. Attempts to update a 
 * task's data which is not currently stopped will return 0 indicating 
 * that the task data was not updated. 
 *
 * @param[in] p_task        Pointer to the task.
 * @param[in] p_data        Pointer to the user data to add to the task.
 * @param[in] data_size     The length of the user data (bytes).  
 *
 * @return                  The size of the data copied into a buffered 
 *                          task or the data size value to be passed to 
 *                          the handler for an unbuffered task.  
 *
 *                          0 if the task pointer was NULL, the task
 *                          has not previously been configured or the
 *                          the task not stopped.
 */
uint8_t sched_task_data(sched_task_t * p_task, void * p_data, uint8_t data_size);

/**
 * Function for updating a scheduler task with a new interval and starting it.
 *
 * Note that a task must have been previously configured to update the 
 * interval.
 *
 * @param[in] p_task        Pointer to the task to add to the scheduler
 * @param[in] interval_ms   Task interval for a repeating task or a delay for
 *                          single-shot task (mS).
 *
 * @return                  True if the interval was successfully updated.
 *
 *                          False if the interval could not be updated because
 *                          it has not been previously configured or the task
 *                          pointer was NULL.
 */
bool sched_task_update(sched_task_t *p_task, uint32_t interval_ms);

/**
 * Function for starting a scheduler task.
 *
 * Note that a task must have been previously configured with 
 * the sched_task_config() function.
 *
 * @return  True if the task was successfully started.
 *
 *          False if the task could not be started because it
 *          has not previously been configured or the task 
 *          pointer was NULL.
 */
bool sched_task_start(sched_task_t *p_task);

/**
 * Function for stopping a scheduler task.
 *
 * The task's handler will finish execution if it is currently
 * running before the task is stopped.
 *
 * @param[in] p_task  Pointer to the task to stop.
 * @return    True if the task was successfully stopped.
 *
 *            False if the task could not be stopped because it
 *            has not previously been configured or the task 
 *            pointer was NULL.
 */
bool sched_task_stop(sched_task_t *p_task);

/**
 * Function for initializing the scheduler module.
 
 * Note that if the scheduler was previously started and 
 * then stopped, this function must not be called until
 * the stop completes as indicated by sched_start() 
 * returning.
 */
void sched_init(void);

/**
 * Function for starting the scheduler.  
 *
 * The function repeatably executes scheduled tasks as they expire.
 * This function must be called from the main context, typically after 
 * all platform initialization has completed. The function does not 
 * return, once called, until the scheduler is stopped.
 *
 * @return    none.
 */
void sched_start(void);

/**
 * Function for stopping the scheduler module and clearing the 
 * scheduler's que. 
 *
 * Note the function call does not immediately stop the scheduler.
 * The scheduler finishes executing any tasks with expired timers 
 * before completing the stop.
 * 
 * @return    none.
 */
void sched_stop(void);

#endif // SCHEDULER_H__