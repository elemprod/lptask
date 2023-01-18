/* 
 * Platform Specific Scheduler Support Functions
 *
 * These functions must be implemented by the user for a
 * platform which utilizes the scheduler library.  See the
 * library's example directory for sample implementations.
 *
 */

#ifndef _SCHEDULER_PORT_H__
#define _SCHEDULER_PORT_H__

#include <stdint.h>

 /**
  * Mandatory platform specific function for aquiring exclusive 
  * access to the scheduler's linked list task que.  The function 
  * is called by the scheduler, from the main context, whenever 
  * it needs to modify the task que.  
  *
  * The lock prevents different sections of code from modifying 
  * the task que pointers at the same time which could lead to 
  * corruption of the que.
  *
  * Exlusive access can typically be achieved by either temporarily
  * disabling global interrupts or by utilizing a mutex lock.  The 
  * scheduler_port_que_unlock() function will always be called the after 
  * scheduler_port_que_lock() call once the que modifications have 
  * completed.
  */
void scheduler_port_que_lock(void);

/**
  * Mandatory platform specific function for releasing exclusive 
  * access to the scheduler's linked list.
  */
void scheduler_port_que_free(void);

 /**
  * Mandatory platform specific function for reading the current 
  * value of the mS timer to be utilized by the scheduler for task
  * timing. 
  *
  * The timer counter must be monatomic, it must increment one time 
  * for each mS of real time after initialilzation with no 
  * discontinuities or jumps.  It is expected  to roll back to 0 
  * after the UINT32_MAX value.  The counter should not be modified, 
  * other than due to the normal mS increment behaviour, once the 
  * scheduler has  been started. 
  *
  * @return    The current timer value (mS).
  */
uint32_t scheduler_port_ms(void);

 /**
  * Optional function for performing in any platform specific 
  * initialization required for scheduler operation.  
  *
  * The mS timer should be setup and enabled here if not previously
  * enabled.
  *
  * @return    none
  */
void scheduler_port_init(void);

 /**
  * Optional function for performing in any platform specific deinitialization 
  * and tear down.  Any resources initialized in scheduler_port_init() should be 
  * deinitialized here.   
  *
  * @return    none
  */
void scheduler_port_deinit(void);

#endif // _SCHEDULER_PORT_H__
