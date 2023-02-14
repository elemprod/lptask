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
 * The handler function is called after the task's interval has
 * expired.
 *
 * @param[in] p_context   Pointer to the user task context.
 */
typedef void (*sched_handler_t)(void *p_context);

/**
 * The data structure for a single scheduler task.
 *
 * The task data structure can be defined and initialized with the
 * SCHED_TASK_DEF() macro.
 *
 * The task should only be modified using the supplied scheduler
 * functions and macros.
 */
typedef struct _sched_task {
  sched_handler_t handler; // The task handler function.
  void *p_context;         // Pointer to the user supplied context.

  struct _sched_task *p_next; // Pointer to the next task in the linked list

  uint32_t start_ms; // Time at task start or restart (mS).

  bool repeat : 1; // Should the task be repeated?
  bool active : 1; // Is the task active?
  bool added : 1;  // Has the task been added task que?

  uint32_t interval_ms : 29; // Task execution interval (mS).
} sched_task_t;


// TODO the added bit could potentially be replaced by checking if p_next is not NULL
// combined with checking the scheduler's last tasks not equal to the task.

/**
 * Function like macro for defining a scheduler task.
 */
#define SCHED_TASK_DEF(sched_evt)   \
  static sched_task_t sched_evt = { \
      .active = false,              \
      .added = false,               \
  };

/**
 * Function for configuring or reconfiguring a scheduler task and adding it
 * to the scheduler task que.
 *
 * The task will be inactive after calling sched_task_config() and it
 * must be enabled with with sched_task_start() function
 *
 * The task interval for a repeating task is desired time between task
 * handler calls.   The interval for non-repeating task is the time delay
 * until the task handler is called.
 *
 * The function can also be used to reconfigure a previously configured function.
 *
 * @param[in] p_task        Pointer to the task to add to the scheduler
 * @param[in] handler       Task handler function.
 * @param[in] p_context     Pointer to the user data to pass the task. (can be NULL)
 * @param[in] interval_ms   The task interval (mS)
 * @param[in] repeat        True for a repeating tasks / False for single shot tasks.
 *
 * @return    none.
 */
void sched_task_config(sched_task_t *p_task,
    sched_handler_t handler,
    void *p_context,
    uint32_t interval_ms,
    bool repeat);

/**
 * Function for starting a scheduler task.
 *
 * Note that an task must have been previously configured with 
 * the sched_task_config() function.
 *
 * @param[in] p_task   Pointer to the task to add to the scheduler
 * @return    none.
 */
void sched_task_start(sched_task_t *p_task);

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
 * Function for stopping an active scheduler task.
 *
 * Note that a stopped task remains in the scheduler task que but has its 
 * active flag cleared to disable it.
 *
 * @param[in] p_task  Pointer to the task to stop to the scheduler
 * @return    none.
 */
void sched_task_stop(sched_task_t *p_task);

/**
 * Function for initializing and starting the scheduler module if not already
 * started.  This must be called before configuring the task.
 *
 * @return  True if the scheduler was initialized.
 *          False if the scheduler could not be initalized because it is currently
 *          in the stopping state.  Try again once the stop completes.
 */
bool sched_init(void);

/**
 * Function for starting scheduler execution.  
 *
 * The function repeatably executes all scheduled tasks as they expire.
 * This function must be called from main, typically after all platform 
 * initialization has completed. The function does not return until 
 * scheduler has stopped.
 *
 * Note that if at any point there are no tasks in the scheduler que or 
 * there are no active tasks in the que, the scheduler will attempt to 
 * sleep for the maximum sleep interval.  In this case, new tasks can 
 * only be added to the que through a interrupt event.
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