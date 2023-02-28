/**
 * scheduler.h
 *
 * The scheduler module provides a simple method for scheduling tasks 
 * to be executed at a later time without the complexity of an RTOS.  
 */

#ifndef SCHEDULER_H__
#define SCHEDULER_H__

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
 * The data structure for a single scheduler task.
 *
 * The task data structure should be defined and initialized with the
 * SCHED_TASK_DEF() or the SCHED_BUFF_TASK_DEF() macros.
 *
 * Scheduler tasks should only be modified using the supplied scheduler
 * functions and macros.
 */
typedef struct _sched_task {

  uint32_t start_ms;    // The task start time (mS).
  uint32_t interval_ms; // The task interval (mS).

  struct _sched_task *p_next; // Pointer to the next task in the linked list

  sched_handler_t handler; // The task handler function.

  uint8_t *p_data; // Pointer to the data location.

  uint8_t buff_size; // Size of the data buffer. (bytes)
                     // 0 for unbuffered tasks.
  uint8_t data_size; // The size of the data stored in the task. (bytes)

  bool repeat : 1;    // Should the task repeat?
  bool active : 1;    // Is the task active?
  bool added : 1;     // Has the task been added to the que?
  bool executing : 1; // Is the task's handler currently executing?

} sched_task_t;

/**
 * Function like macro for defining an unbuffered scheduler task.
 *
 * An unbuffered scheduler task doesn't have a data buffer 
 * dedicated to task but users can still store and pass a data 
 * pointer and data size to task handler.
 *
 * @param[in] TASK_ID  Unique task name.
 */
#define SCHED_TASK_DEF(TASK_ID)       \
  static sched_task_t TASK_ID = {     \
      .p_data = NULL,                 \
      .buff_size = 0,                 \
      .data_size = 0,                 \
      .repeat = false,                \
      .active = false,                \
      .added = false,                 \
      .executing = false,             \
  } 

/**
 * Function like macro for defining a buffered scheduler task.
 *
 * A buffered schedulelr task contains a data buffer for passing user
 * data to the task handler.  User data is  can be copied to the
 * tasks internal buffer and is supplied to the task handler when
 * called.
 *
 * @param[in] TASK_ID     Unique task name.
 * @param[in] BUFF_SIZE   The buffer size to allocate for the task (bytes)
 *                        Must be > 0 and <= UINT8_MAX
 */
#define SCHED_BUFF_TASK_DEF(TASK_ID, BUFF_SIZE)           \
  static uint8_t TASK_ID##_BUFF[(BUFF_SIZE) % UINT8_MAX]; \
  static sched_task_t TASK_ID = {                         \
      .p_data = TASK_ID##_BUFF,                           \
      .buff_size = (BUFF_SIZE) % UINT8_MAX,               \
      .data_size = 0,                                     \
      .repeat = false,                                    \
      .active = false,                                    \
      .added = false,                                     \
      .executing = false,                                 \
  }                                


// TODO it would be nice to have some way for defining arrays of tasks.
// We would require an array_init function in addition to the define macro.

/**
 * Function for configuring or reconfiguring a scheduler task and adding it
 * to the scheduler task que.
 *
 * The task will be inactive after calling sched_task_config() and it
 * must be made active with with sched_task_start() function.
 *
 * The task interval for a repeating task is the desired time in mS between 
 * task handler calls.   The interval for non-repeating task is the time
 * delay from now until the task handler is called. An interval of 0 will 
 * result in handler being called as soon as possible.
 *
 * The function can also be used to reconfigure a previously configured task.
 *
 * The scheduler must be initialized prior to calling this function.
 *
 * @param[in] p_task        Pointer to the task.
 * @param[in] handler       Task handler function.
 * @param[in] interval_ms   The task interval (mS)
 * @param[in] repeat        True for a repeating tasks.
 *                          False for single shot tasks.
 *
 * @return                  None.
 */
void sched_task_config(sched_task_t *p_task,
    sched_handler_t handler,
    uint32_t interval_ms,
    bool repeat);

/**
 * Function for configuring the task data.
 * 
 * Note that the task data can not be modified if a 
 * previously started task's handler is currently executing.
 * Attempts to set a task's data which is currently executing
 * its handler will return 0 indicating that no data was 
 * stored in the task.  
 *
 * The data is copied to the task's internal memory for buffered 
 * tasks during the function call. Note that data_size is limited to 
 * be less than or equal to the buffer size memory for buffered tasks.
 * 
 * Only the data pointer nd the data size are stored for unbuffered 
 * tasks.  Since the data at the pointer location is not copied for 
 * unbuffered task, the data must still be valid when the task's 
 * handler is callled.  
 *
 * @param[in] p_task        Pointer to the task.
 * @param[in] p_data        Pointer to the data.
 * @param[in] data_size     The length of the data (bytes).  
 *
 * @return                  The size of the data copied into a buffered 
 *                          task or the data size value to be passed to 
 *                          the handler for an upbuffered task.  
 */
uint8_t sched_task_data(sched_task_t * p_task, void * p_data, uint8_t data_size);

/**
 * Function for updating a scheduler task with a new interval and starting it.
 *
 * Note that an task must have been previously configured with the 
 * sched_task_config() function.
 *
 * @param[in] p_task        Pointer to the task to add to the scheduler
 * @param[in] interval_ms   Task interval for a repeating task or a delay for
 *                          single-shot task (mS).
 * @return    none.
 */
void sched_task_update(sched_task_t *p_task, uint32_t interval_ms);

/**
 * Function for starting a scheduler task.
 *
 * Note that an task must have been previously configured with 
 * the sched_task_config() function.
 *
 * @return                  None.
 */
void sched_task_start(sched_task_t *p_task);

/**
 * Function for stopping an active scheduler task.
 *
 * The task's handler will finish execution if it is currently
 * running but the handler will not be ran again after the 
 * function is called.
 *
 * Note that a stopped task remains in the scheduler task que but
 * has its active flag cleared to disable it.  The repeating
 * flag is also cleared, A repeating task will need to be 
 * reconfigured before starting it again.
 *
 * @param[in] p_task  Pointer to the task to stop.
 * @return    none.
 */
void sched_task_stop(sched_task_t *p_task);

/**
 * Function for initializing the scheduler module.
 
 * Note that if the scheduler has been previously started and 
 * then stopped, this function must not be called until the stop
 * completes as indicated by sched_start() returning.
 *
 */
void sched_init(void);

/**
 * Function for starting scheduler execution.  
 *
 * The function repeatably executes all scheduled tasks as they expire.
 * This function must be called from main, typically after all platform 
 * initialization has completed. The function does not return until 
 * scheduler has been stopped.
 *
 * The scheduler must have been previously initalized before 
 * calling this function.
 *
 * @return    none.
 */
void sched_start(void);

/**
 * Function for stopping the scheduler module and clearing the scheduler's que.
 *
 * Note that scheduler does not immediately stop.  The scheduler will finish
 * executing any tasks with expired timers before completing the stop. 
 * 
 * @return    none.
 */
void sched_stop(void);

#endif // SCHEDULER_H__