/**
 * STM32L0XX Power Mode Module
 *
 *
 * The module provides a high level interface for configuring the
 * STM32L0XX processor with one of the several different Power Modes.
 *
 *
 */

#ifndef PWR_MODE_H__
#define PWR_MODE_H__

#include "stm32l0xx.h"
#include <stdbool.h>

/**
 * Function for enabling the Run Power Mode with the Medium Speed
 * Internal Oscillator.
 *
 */
void pwr_run_msi(void);

/**
 * Function for enabling the Run Power Mode with the 16 MHz High Speed
 * Internal Oscillator.
 */
void pwr_run_hsi(void);

/**@brief Function for putting the processor in the Sleep Power Mode.
 *
 */
void pwr_sleep(void);

/**
 * Function for putting the processor in the Stop Power Mode with a Wake Up Timer.
 *
 * Note that processor will return to the run mode after waking once the
 * specified time period has expired or if woken from a different IRQ source.
 *
 * The LPTIM timer is configured to generate an interrupt after the supplied
 * period to wake the processor from the stop mode.
 *
 * The systick timer and interupt are disabled in the stop mode. The systick
 * timer is corrected for the duration of the stop once the processor is woken.
 * The systick time is not corrected until this function returns.
 *
 * Note that this function is blocking.  It does not return until the time
 * interval expires or another hardware interrupt is received.
 *
 * @param[in] period_ms The duration to sleep in mS.
 *
 */
void pwr_stop_lptim(uint16_t period_ms);

/**@brief Function for initializing the power module and starting the run mode.
 *
 * Must be called before any of the other functions to initialize the module.
 *
 * @returns   RET_SUCCESS if the power set mode was sucessful.
 *            Or other lower level module error.
 */
void pwr_init(void);

#endif /* PWR_MODE_H__ */