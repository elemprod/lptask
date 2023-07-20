/**
 * @file sched_port.h
 * @author Ben Wirz
 * @brief Platform-specific scheduler support functions.
 *
 * @note The functions marked as mandatory must be implemented by the user in
 * order to utilize the scheduler module.
 */

#ifndef SCHED_PORT_H__
#define SCHED_PORT_H__

#include <stdint.h>

/**
 * @brief Mandatory platform-specific function for acquiring exclusive
 * access to the scheduler's shared data structure.
 *
 * The lock prevents different contexts of code from modifying the schedulers
 * shared data at the same time which could lead to corruption of the que.
 */
void sched_port_lock(void);

/**
 * @brief Mandatory platform-specific function for releasing exclusive
 * access to the scheduler's shared data structure.
 *
 * The sched_port_free() function will be called once to following each
 * sched_port_lock() call.
 */
void sched_port_free(void);

/**
 * @brief Mandatory platform-specific function for getting the current
 * value of the mS timer to be utilized by the scheduler for task
 * timing.
 *
 * The timer counter must be monatomic, it should increment once for each mS of
 * real time after initialization with no discontinuities or jumps.  It is
 * expected to roll over to 0 after UINT32_MAX.
 *
 * @return The current timer value (mS).
 */
uint32_t sched_port_ms(void);

/**
 * @brief Optional platform-specific sleep function.
 *
 * If no user implementation is supplied, the scheduler will
 * simply busy wait between tasks.
 *
 * @param[in] interval_ms  The requested sleep interval (mS)
 *                         0 to SCHED_MS_MAX (mS)
 */
void sched_port_sleep(uint32_t interval_ms);

/**
 * @brief Optional platform-specific specific function for performing any
 * initialization required for scheduler operation.
 *
 * The mS timer should be setup and enabled here if not previously
 * enabled.
 */
void sched_port_init(void);

/**
 * @brief Optional function for performing in any platform-specific
 * deinitialization and tear down.  Any resources initialized in
 * sched_port_init() should be deinitialized here.
 */
void sched_port_deinit(void);

#endif // SCHED_PORT_H__
