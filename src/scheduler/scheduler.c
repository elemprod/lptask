#include <assert.h>
#include "scheduler.h"
#include "scheduler_config.h"

/* The scheduler's interal data structure.
*/
typedef struct
{            
  sched_evt_t   * p_head;   // Pointer to the head event in the linked list
  sched_evt_t   * p_tail;   // Pointer to the tail event in the linked list
} scheduler_t;

// The scheduler module's internal data.
static scheduler_t scheduler;           

void sched_init(void) {
  // The only required initialization is to set the head and tail events to NULL.
  scheduler.p_head    = NULL;
  scheduler.p_tail    = NULL;
}



void sched_evt_config(sched_evt_t     * p_event, 
                            sched_handler_t   handler, 
                            void            * p_context,
                            uint32_t          period_ms,
                            bool              repeat) {

  // A pointer to the event to be added the schedueler que must be supplied.
  assert(p_event != NULL);

  // The event's hander must be supplied.
  assert(handler != NULL);

  // Mark the event as inactive.
  p_event->active = false;      
  // Set the event handler.          
  p_event->handler = (sched_handler_t) handler;   
  // Save the user context.                               
  p_event->p_context = (void *) p_context;      
  // Limit the period.                                  
  p_event->period_ms = period_ms & SCHEDULER_MS_MAX;  
  // Setting the repeating status.                        
  p_event->repeat = (bool) repeat;  

  // Add the event to the scheduler's que if not previously added
  if(!p_event->added) {

    // Disable Global Interrupts to gain exclusive access to linked list
    SCHEDULER_CRITICAL_REGION_ENTER();

    // The new event will be the last one in the list so it's next event will always be NULL.
    p_event->p_next = NULL;

    if(scheduler.p_head == NULL) {
      // No other events exists in the list so the new event will be both the head & tail event.
      scheduler.p_head = p_event;
    } else {
      // If other events exist, this event will be the next event for the current tail event.
      assert(scheduler.p_tail != NULL);
      scheduler.p_tail->p_next = p_event;
    }
    // Set the new event to the tail event since it is added to the end of the list.
    scheduler.p_tail = p_event;

    p_event->added = true;
    p_event->pending = false;

    // Renable Global Interrupts
    SCHEDULER_CRITICAL_REGION_EXIT();
  }
}

void sched_evt_start(sched_evt_t * p_event) {

  // A pointer to the event to start must be supplied.
  assert(p_event != NULL);

  // The event must have been previously added to the que.
  assert(p_event->added);

  if((SysTick->CTRL & SysTick_CTRL_TICKINT_Msk)) {
    // Update the event's expire tick if SysTick Interrupt is running
    p_event->expire_tick = p_event->period_ms + HAL_GetTick();
    p_event->pending = false;
  } else {
    // Mark the event as pending so the expire_tick gets updated
    // This could happen if the event is added from an IRQ in Stop Power Mode
    p_event->pending = true;
  }
  
  // Mark the event as active
  p_event->active = true;

}

void sched_evt_update(sched_evt_t * p_event, uint32_t period_ms) {

  // A pointer to the event to update must be supplied.
  assert(p_event != NULL);

  // Store the new period.
  p_event->period_ms = period_ms;
  
  // start the event
  sched_evt_start(p_event);   
}

void sched_evt_stop(sched_evt_t * p_event) {

  // A pointer to the event to start must be supplied.
  assert(p_event != NULL);
  // Set the event as inactive but don't remove it from the que.
  p_event->active = false;

}

// Function for checking if an event's timer has expired
static bool sched_evt_expired(sched_evt_t * p_event) {
  return (HAL_GetTick() - p_event->expire_tick) < 0x80000000;
}

// Function for finding the next expiring scheduler event
static sched_evt_t * sched_next_evt(void) {

  // Pointer to the next expiring event.
  sched_evt_t * p_next_expiring_evt = NULL;  

  /* Loop through each event in the linked list starting at the head event
   * Event access is read only so it's not necessary to disable interrupts
   * in this case.
   */
  sched_evt_t * p_current_evt  = scheduler.p_head;         
  
  while(p_current_evt != NULL) {
    if(p_current_evt->active == true) {
      if(p_next_expiring_evt == NULL) {
        // No expiring event was previously stored so this event is the next expiring event
        p_next_expiring_evt = p_current_evt;
      } else if(p_current_evt->expire_tick < p_next_expiring_evt->expire_tick) {
        //TODO does this correctly handle the SysTick roll over or do we need to compare each time subtracked from SysTick???
        // The current event expires sooner than the previous event so set it as the next expiring event
        p_next_expiring_evt = p_current_evt;
      } 
    }

    // Move to the next event in the list
    p_current_evt = p_current_evt->p_next;
  }

  return p_next_expiring_evt;
}

sched_evt_t * sched_execute(void)
{
  // Loop through each event in the linked list starting with the head event
  sched_evt_t * p_current_evt  = scheduler.p_head;         
  
  while(p_current_evt != NULL) {

    // Store pointer to next event since the current event may be removed
    // if non-repeating and expired or removed in the event handler
    sched_evt_t * p_next_evt  = p_current_evt->p_next;
    
    assert(p_current_evt->added);   // all events should be marked as added to the list

    // Filter on Active Events
    if(p_current_evt->active) {

      // Update the expire tick if its pending
      if(p_current_evt->pending) {
        p_current_evt->expire_tick = p_current_evt->period_ms + HAL_GetTick();
        p_current_evt->pending = false;
      }

      // Handle any events with expired timers   
      if(sched_evt_expired(p_current_evt)) {
        if(p_current_evt->repeat) {
          // Recalculate the expiration ticks to reschedule the event before calling the
          // handler so the handler execution time does not delay the expiration calculation.  
          p_current_evt->expire_tick = p_current_evt->period_ms + HAL_GetTick();

        } else {
          // Mark the non-repeating event as inactive before calling the handler since it may be restarted in the handler.
          p_current_evt->active = false;
        }
        // Execute the Event Handler
        p_current_evt->handler(p_current_evt->p_context);
      }
    }

    // Move to the next event in the list
    p_current_evt = p_next_evt;
  }

  // The next expiring event needs to be determined after the scheduler has fully executed since an event's
  // active status may be changed after its handler has executed.  An event's active status may have been modified 
  // by another event's handler later in the que.
  return sched_next_evt();    
}


