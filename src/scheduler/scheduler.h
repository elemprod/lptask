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

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

// Maximum Task Period in mS. (~74 hours)
#define SCHEDULER_MS_MAX 0x0FFFFFFF

 /* 
  * Scheduler Task Handler Function Prototype.
  *
  * The handler function is called once the task expires.
  *
  * @param[in] p_context   Pointer to the user taks context.
  */
typedef void (* sched_handler_t) (void * p_context);

/* 
 * Data structure for a single scheduler task.
 *
 * The locally stored scheduler task data structure.
 * Note that the data structure variables should only be modified
 * through the scheduler functions.
 */
typedef struct _sched_task
{
  sched_handler_t       handler;            // Handler function for the task
  void                * p_context;          // Pointer to the user supplied context to pass to the handler.

  struct _sched_task  * p_next;             // Pointer to the next task in the linked list
 
  uint32_t              expire_tick;        // Systick counter value after which the task is scheduled to execute

  bool                  repeat      : 1;    // Should the task be repeated after timer expiration?
  bool                  active      : 1;    // Is the task active?
  bool                  added       : 1;    // Has the task been added to the scheduler list? 
  bool                  pending     : 1;    // Does the expire_tick need to be calculated?

  uint32_t              period_ms   : 28;   // Period in mS between task executions

} sched_task_t;


 /**@brief Macro for Defining a New Scheduler Task.
 */
#define SCHED_TASK_DEF(sched_evt)                  \
  static sched_task_t sched_evt = {                \
      .active = false,                            \
      .added = false,                             \
    };                                            \

 /**@brief Macro for Checking if the Task is Active.
 *  @param[in] task   The task.
 */
#define SCHED_TASK_ACTIVE(task) (task.active == true)

 /**@brief Function for configuring a scheduler task and adding it to the 
 * scheduler task que.
 *
 * Note that the task will be inactive after sched_task_config() and must be enabled
 * with sched_task_start().
 * The function can also be used to reconfigure a previously configured function.
 *
 * @param[in] p_task   Pointer to the task to add to the scheduler
 * @param[in] handler   Task handler function.
 * @param[in] p_context Pointer to the user data to pass the task. (can be NULL)
 * @param[in] period_ms Task period for a repeating task or the delay for single 
 *                      task (mS). Valid values range from 0 to 268,435,455 (0xFFFFFFF).  
 *                      A 0 delay results in the task being executed during the the 
 *                      next sched_execute() call.
 * @param[in] repeat    True for a repeating tasks / False for single shot tasks.
 *
 * @return    none.
 *
 */                                                                           
void sched_task_config(sched_task_t    * p_task, 
                       sched_handler_t   handler, 
                       void            * p_context,
                       uint32_t          period_ms,
                       bool              repeat);

 /**@brief Function for starting a scheduler task.
 *
 * Note that an task must have been previously configured with sched_task_config()
 *
 * @param[in] p_task   Pointer to the task to add to the scheduler
 * @return    none.
 */
 void sched_task_start(sched_task_t * p_task);

 /**@brief Function for starting a a scheduler task.
 *
 * Note that an task must have been previously configured with sched_task_config()
 *
 * @param[in] p_task   Pointer to the task to add to the scheduler
 * @param[in] period_ms Task period for a repeating task or the delay for single 
 *                      task (mS). Valid values range from 0 to 268,435,455 (0xFFFFFFF).  
 *                      A 0 delay results in the task being executed during the the 
 *                      next sched_execute() call.
 * @return    none.
 */
 void sched_task_update(sched_task_t * p_task, uint32_t period_ms);

 /**@brief Function for stopping an active scheduler task.
 *
 * Note that a stopped task remains in the scheduler task que but has its active 
 * flag cleared to disable it.
 *
 * @param[in] p_task   Pointer to the task to stop to the scheduler
 *
 * @return    none.
 */
 void sched_task_stop(sched_task_t * p_task);

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


// TODO add function for checking the time remaining until the task expires.
#endif // SCHEDULER_H__
