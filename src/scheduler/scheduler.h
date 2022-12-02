/********************************** scheduler.h ***********************************/ 
/*                                                                                */
/* The Scheduler module provides a simple method for scheduling tasks to be       */
/* executed a later point without the complexity of an RTOS.  The event's handler */ 
/* is called from the main context with an optional delay.  Events may be         */
/* configured as repeating in which case they will be executed periodically until */
/* stopped.                                                                       */
/*                                                                                */
/* Note that the scheduler does not provide the same Real Time capability that    */   
/* an RTOS offers.  Scheduled events are are executed in the order they are       */
/* added to the scheduler with no prioritization functionality provided.  Each    */
/* event executes until completion once it's timer expires.  Event execution is   */
/* only paused by a hardware IRQ.  Many embedded systems outside of motion and    */
/* process control don't require hard real time operation and therefore can live  */
/* within these constraints saving the RTOS overhead.                             */
/*                                                                                */
/* Events are are stored as nodes in a Single Linked List providing a fixed       */
/* compile time RAM utilization.  Each event must must be statically stored by    */
/* the calling module.                                                            */
/*                                                                                */
/* The scheduler disables global interupts while adding events from the events    */
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

// Maximum Event in mS. (~74 hours)
#define SCHEDULER_MS_MAX 0x0FFFFFFF

 /**@brief Scheduler Handler Function Prototype called after the event timer 
  * expires to execute the scheduled event.
 *
 * @param[in] p_context   Pointer to the stored event context.
 */
typedef void (* sched_handler_t) (void * p_context);

/* Data structure for a single scheduler event.
*
* The locally stored persistent scheduler event data structure must .
* Note that the data structure variables should not be modified directly. 
*/
typedef struct _sched_evt
{
  sched_handler_t       handler;            // Handler function for the event
  void                * p_context;          // Pointer to the user supplied context to pass to the handler.

  struct _sched_evt   * p_next;             // Pointer to the next event in the linked list
 
  uint32_t              expire_tick;        // Systick counter value after which the event is scheduled to execute

  bool                  repeat      : 1;    // Should the event be repeated after timer expiration?
  bool                  active      : 1;    // Is the event active?
  bool                  added       : 1;    // Has the event been added to the scheduler list? 
  bool                  pending     : 1;    // Does the expire_tick need to be calculated?

  uint32_t              period_ms   : 28;   // Period in mS between event executions

} sched_evt_t;


 /**@brief Macro for Defining a New Scheduler Event.
 */
#define SCHED_EVT_DEF(sched_evt)                  \
  static sched_evt_t sched_evt = {                \
      .active = false,                            \
      .added = false,                             \
    };                                            \

 /**@brief Macro for Checking if the Event is Active.
 *  @param[in] event   The event.
 */
#define SCHED_EVT_ACTIVE(event) (event.active == true)

 /**@brief Function for configuring a scheduler event and adding it to the 
 * scheduler event que.
 *
 * Note that the event will be inactive after sched_evt_config() and must be enabled
 * with sched_evt_start().
 * The function can also be used to reconfigure a previously configured function.
 *
 * @param[in] p_event   Pointer to the event to add to the scheduler
 * @param[in] handler   Event handler function.
 * @param[in] p_context Pointer to the user data to pass the event. (can be NULL)
 * @param[in] period_ms Event period for a repeating event or the delay for single 
 *                      event (mS). Valid values range from 0 to 268,435,455 (0xFFFFFFF).  
 *                      A 0 delay results in the task being executed during the the 
 *                      next sched_execute() call.
 * @param[in] repeat    True for a repeating events / False for single events.
 *
 * @return    none.
 *
 */                                                                           
void sched_evt_config(sched_evt_t     * p_event, 
                            sched_handler_t   handler, 
                            void            * p_context,
                            uint32_t          period_ms,
                            bool              repeat);

 /**@brief Function for starting previously configured event.
 *
 * Note that an event must have been previously configured with sched_evt_config()
 *
 * @param[in] p_event   Pointer to the event to add to the scheduler
 * @return    none.
 */
 void sched_evt_start(sched_evt_t * p_event);

  /**@brief Function for updating the event's period and starting a previously configured event.
 *
 * Note that an event must have been previously configured with sched_evt_config()
 *
 * @param[in] p_event   Pointer to the event to add to the scheduler
 * @param[in] period_ms Event period for a repeating event or the delay for single 
 *                      event (mS). Valid values range from 0 to 268,435,455 (0xFFFFFFF).  
 *                      A 0 delay results in the task being executed during the the 
 *                      next sched_execute() call.
 * @return    none.
 */
 void sched_evt_update(sched_evt_t * p_event, uint32_t period_ms);

 /**@brief Function for stopping an active scheduler event.
 *
 * Note that a stopped event remains in the scheduler event que but has its active flag cleared
 * so it will not execute.
 *
 * @param[in] p_event   Pointer to the event to stop to the scheduler
 * @return    none.
 */
 void sched_evt_stop(sched_evt_t * p_event);

/**@brief Function for executing all scheduled events.
 *
 * @details This function must be called from within the main loop. It will execute 
 *          all scheduled events having expired timers before returning.
 *
 * @returns Returns a pointer to the next expiring event or NULL if no other events are
 *          scheduled.  The return value can be used to control the processors sleep operation.
 *          If no events are scheduled, the processor can enter a deep sleep or stop mode. 
 *          If the next event expiration time is short, the processor may simply want to call
 *          sched_execute() again.  If the expiration time is longer, the processor may want
 *          to schedule a wakeup using a low power timer prior to sleeping.
 */
sched_evt_t * sched_execute(void);

/**@brief Function for initializing the scheduler module and clearing the event
 * que.
 *
 * @return    none.
 */
void sched_init(void);


#endif // SCHEDULER_H__
