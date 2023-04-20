/**
 * @file sched_types.h
 * @author Ben Wirz
 * @brief Scheduler module custom types.
 * 
 */

#ifndef SCHED_TYPES_H__
#define SCHED_TYPES_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/**
 * @brief A type representing a scheduler task state.
 */
typedef enum {
  /// @brief The task has not been initialized yet.
  SCHED_TASK_UNINIT = 0,
  /// @brief The task has been initialized but is inactive.
  SCHED_TASK_STOPPED,
  /// @brief The task is active
  SCHED_TASK_ACTIVE,
  /// @brief The task handler is executing.
  SCHED_TASK_EXECUTING,
  /**
   * @brief The task is in the proccess of stopping.
   * 
   * The task enters the SCHED_TASK_STOPPING state if the sched_task_stop() 
   * function is called while the task is executing its handler.  The task will
   *  move to the SCHED_TASK_STOPPED state once it's handler completes.
   */ 
  SCHED_TASK_STOPPING
} sched_task_state_t;

/**
 * @brief The data structure for a single scheduler task.
 *
 * The task data structure should be defined with the SCHED_TASK_DEF() or 
 * the SCHED_TASK_BUFF_DEF() macros or allocated with the sched_task_alloc() 
 * function from a task pool.  the scheduler task structure should only be 
 * accessed using the supplied scheduler functions.
 * 
   * @note The allocated flag and state variables must be volatile since they 
   * can be modified from an different contexts during task's handler execution.
 */
typedef struct _sched_task {

  /// @brief The task start time (mS).
  uint32_t start_ms;

  /// @brief The task interval (mS).
  uint32_t interval_ms;

  /// @brief The next task in the linked list. 
  struct _sched_task *p_next; 

  /** 
   * @brief The task's handler.
   * @sa sched_handler_t
   */ 
  void * p_handler;

  /// @brief Pointer to the users data.
  uint8_t *p_data;  

  /** 
   * @brief Size of the internal data buffer. (bytes)
   * 
   * Unbuffered tasks will have a buff_size of 0 to indicate that they 
   * don't have an internal buffer.
   */ 
  uint8_t buff_size;

  /** 
   * @brief Size of the user's data. (bytes)
   * 
   * For buffered tasks, data_size represents the length of the actual data 
   * stored in the task and will always be <= buff_size.
   */   
  uint8_t data_size;

  /// @brief Does the task repeat?
  bool repeat : 1;  

  /// @brief Has the task been been allocated?
  volatile bool allocated: 1;        

  /// @brief The task's current state.     
  volatile sched_task_state_t state : 4;

} sched_task_t;

/**
 * @brief Scheduler Task Handler Function Prototype.
 *
 * The task's handler function is called at task interval expiration.
 *
 * @param[in] p_task      Pointer to the task.
 * @param[in] p_data      Pointer to the task data.
 * @param[in] data_size   Size of the task data. (bytes) 
 * 
 * @return void
 */
typedef void (*sched_handler_t)(sched_task_t *p_task, void *p_data, uint8_t data_size);

/**
 * @brief Buffered task pool configuration structure.
 *
 * A task pool should be defined with the SCHED_TASK_POOL_DEF() macro.
 */
typedef struct {
  /// @brief Pointer to the pool data buffer.
  uint8_t *p_data;
  /// @brief Pointer to the array of tasks in the pool.   
  sched_task_t *p_tasks;
  /// @brief Size of the taks data buffer. (bytes)  
  uint8_t buff_size;
  /// @brief The number of tasks in the pool.  
  uint8_t task_cnt;
  /// @brief Has the pool been initialized?  
  bool initialized : 1;
} sched_task_pool_t;

/**
 * @brief Macro for limiting a buffer size parameter.
 * 
 * Buffer sizes are limited to be greater than 0 and less than UINT8_MAX.
 * 
 * @param[in] value The buffer size value to limit.
 */
#define SCHED_BUFF_LIMIT(value) (((value) > 1 ? (value) : 1) % UINT8_MAX)

/**
 * @brief Macro for limiting a task count parameter.
 * 
 * Task counts are limited to be greater than 0 and less than UINT8_MAX.
 *
 * @param[in] value The task count value to limit.
 */
#define SCHED_TASK_LIMIT(value) (((value) > 1 ? (value) : 1) % UINT8_MAX)

#endif // SCHED_TYPES_H__