/********************************** scheduler.h ***********************************/
/*                                                                                */
/* The Scheduler module provides a simple method for scheduling tasks to be       */
/* executed at a later time without the complexity of an RTOS.  The task's        */
/* handler function is called from the main context with an optional delay.       */
/* tasks may be */
/* configured as repeating, in which case they will be executed periodically      */
/* until stopped.                                                                 */
/*                                                                                */
/* Note that the scheduler does not provide the same real time capability that    */
/* an RTOS offers.  Scheduled tasks are are executed in the order they are       */
/* added to the scheduler with no prioritization functionality provided.  Each    */
/* task executes until completion after it's timer expires.  Task execution is  */
/* only paused by a hardware IRQ.  Many embedded systems outside of motion and    */
/* process control don't require hard real time operation and therefore can live  */
/* within these constraints saving complexity and overhead .                      */
/*                                                                                */
/* tasks are are stored as nodes in a Single Linked List providing a fixed       */
/* compile time RAM utilization.  Each task must must be statically stored by    */
/* the calling module.                                                            */
/*                                                                                */
/* The scheduler disables global interupts while adding tasks from the tasks    */
/* que in order to gain exclusive access to the list. This technique works with   */
/* a single core processor and eliminates the need for mutex a lock.              */
/*                                                                                */
/*                                                                                */
/**********************************************************************************/

#ifndef SCHEDULER_H__
#define SCHEDULER_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

// Maximum Task Interval in mS. (~6.2 days)
#define SCHEDULER_MS_MAX 0x1FFFFFFF

/*
 * Scheduler Task Handler Function Prototype.
 *
 * The handler function is called once the task expires.
 *
 * @param[in] p_context   Pointer to the user taks context.
 */
typedef void (*sched_handler_t)(void * p_context);

/**@brief The locally stored data structure for a single
 * sheduler task.
 *
 * Notes:
 * The task structure can be defined and intialized with the 
 * SCHED_TASK_DEF() macro.
 *
 * The task data structure should only be accessed using the
 * supplied scheduler functions.
 *
 */
typedef struct _sched_task {

  sched_handler_t       handler;          // The task handler function.
  void                * p_context;        // Pointer to the user supplied context.

  struct _sched_task  * p_next;           // Pointer to the next task in the linked list

  uint32_t              start_ms;         // Time at task start or restart (mS).

  bool                  repeat : 1;       // Should the task be repeated?
  bool                  active : 1;       // Is the task active?
  bool                  added : 1;        // Has the task been added taks que?

  uint32_t              interval_ms : 29; // Task execution interval (mS).
} sched_task_t;

/**@brief Macro for Defining a New Scheduler Task.
 */
#define SCHED_TASK_DEF(sched_evt)   \
  static sched_task_t sched_evt = { \
      .active = false,              \
      .added = false,               \
  };

/**@brief Macro for Checking if the Task is Active.
 *
 *  @param[in] task   The task.
 */
#define SCHED_TASK_ACTIVE(task) (task.active == true)

/**@brief Function for configuring or reconfiguring a scheduler task and adding it 
 * to the scheduler task que.
 *
 * Note that the task will be inactive after calling sched_task_config() and it 
 * must be enabled with with sched_task_start().function
 * 
 * The function can also be used to reconfigure a previously configured function.
 *
 * @param[in] p_task        Pointer to the task to add to the scheduler
 * @param[in] handler       Task handler function.
 * @param[in] p_context     Pointer to the user data to pass the task. (can be NULL)
 * @param[in] interval_ms   Task interval for a repeating task or a delay for
 *                          single-shot task (mS). 
 *
 * @param[in] repeat        True for a repeating tasks / False for single shot tasks.
 *
 * @return    none.
 *
 */
void sched_task_config( sched_task_t    * p_task,
                        sched_handler_t   handler,
                        void            * p_context,
                        uint32_t          interval_ms,
                        bool              repeat);

/**@brief Function for starting ascheduler task.
 *
 * Note that an task must have been previously configured with sched_task_config()
 *
 * @param[in] p_task   Pointer to the task to add to the scheduler
 * @return    none.
 */
void sched_task_start(sched_task_t * p_task);

/**@brief Function for updating a scheduler task with a new interval and starting it.
 *
 * Note that an task must have been previously configured with sched_task_config()
 *
 * @param[in] p_task        Pointer to the task to add to the scheduler
 * @param[in] interval_ms   Task interval for a repeating task or a delay for
 *                          single-shot task (mS). 
 *                          next sched_execute() call.
 * @return    none.
 */
void sched_task_update(sched_task_t * p_task, uint32_t interval_ms);

/**@brief Function for stopping an active scheduler task.
 *
 * Note that a stopped task remains in the scheduler task que but has its active
 * flag cleared to disable it.
 *
 * @param[in] p_task  Pointer to the task to stop to the scheduler
 * @return    none.
 */
void sched_task_stop(sched_task_t * p_task);


/**@brief Function for checking if a task's timer has expired.
 *
 * Note that an inactive tasks never expire.
 *
 * @param[in] p_task  Pointer to the task to stop to the scheduler
 * @return            true if task's timer has expired.
 */
bool sched_task_expired(sched_task_t * p_task);

/**@brief Function for calculating the time until a task's timer expires.
 *
 * Note that the task must be active to check the remaining time.
 *
 * @param[in] p_task  Pointer to the task to stop to the scheduler
 * @return            The time in mS until the task expires or 0 if 
 *                    the task has already expired.
 */
uint32_t sched_task_remaining_ms(sched_task_t * p_task);

/**@brief Function for calculating the time since a task's timer was started
 * or restarted in the case of a repeating timer.
 *
 * Note that the task must be active to calculate the elapsed time.
 *
 * @param[in] p_task  Pointer to the task to stop to the scheduler
 * @return    The time in mS since the task was started.
 */
uint32_t sched_task_elapsed_ms(sched_task_t * p_task);

/**@brief Function for executing all scheduled tasks.
 *
 * @details This function must be called from within the main loop. It will execute
 *          all scheduled tasks having expired timers before returning.
 *
 * @returns A pointer to the next expiring task or NULL if no other tasks are
 *          scheduled.  The return value can be used to control the processors sleep operation.
 *          If no tasks are scheduled, the processor can enter a deep sleep or stop mode.
 *          If the next task expiration time is short, the processor may simply want to call
 *          sched_execute() again.  If the expiration time is longer, the processor may want
 *          to schedule a wakeup using a low power timer prior to sleeping.
 */
sched_task_t * sched_execute(void);

/**@brief Function for initializing the scheduler module and clearing the task
 * que.
 *
 * @return    none.
 */
void sched_init(void);

#endif // SCHEDULER_H__