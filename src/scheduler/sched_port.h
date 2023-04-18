/**
 * @file sched_port.h
 * @author Ben Wirz
 * @brief Platform specific scheduler support functions.
 * 
 * The library's example folder contains sample implementations.
 * 
 * @note The functions marked as mandatory must be implemented by the user in 
 * order to utilize the scheduler module.  
 */

#ifndef SCHED_PORT_H__
#define SCHED_PORT_H__

#include <stdint.h>

/**
 * @brief Mandatory platform specific function for acquiring exclusive
 * access to the scheduler's shared data structure.
 *
 * The lock prevents different contexts of code from modifying
 * the task que pointers at the same time which could lead to
 * corruption of the que.
 *
 * @return void
 */
void sched_port_lock(void);

/**
 * @brief Mandatory platform specific function for releasing exclusive
 * access to the scheduler's shared data structure.
 */
void sched_port_free(void);

/**
 * @brief Mandatory platform specific function for reading the current
 * value of the mS timer to be utilized by the scheduler for task
 * timing.
 *
 * The timer counter must be monatomic, it should increment once for each mS of 
 * real time after initialization with no discontinuities or jumps.  It is 
 * expected to roll back to 0 after the UINT32_MAX value.  
 *
 * @return The current timer value (mS).
 */
uint32_t sched_port_ms(void);

/**
 * @brief Optional platform specific sleep function.
 *
 * If no user implementation is supplied, the scheduler will
 * simply busy wait between tasks.
 *
 * @param[in] interval_ms  The requested sleep interval (mS)
 *                         0 to SCHED_MS_MAX (mS)
 *
 * @return    none
 */
void sched_port_sleep(uint32_t interval_ms);

/**
 * @brief Optional function for performing in any platform specific
 * initialization required for scheduler operation.
 *
 * The mS timer should be setup and enabled here if not previously
 * enabled.
 *
 * @return    none
 */
void sched_port_init(void);

/**
 * @brief Optional function for performing in any platform specific
 * deinitialization and tear down.  Any resources initialized in
 * sched_port_init() should be deinitialized here.
 *
 * @return    none
 */
void sched_port_deinit(void);

#endif // SCHED_PORT_H__
