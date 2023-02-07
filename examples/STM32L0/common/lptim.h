/**
 * STM32L0XX Low Power Timer Module
 *
 * The module provides a high level interface for using the Low
 * Power Timer module as a one shot timer with mS resolution.
 * Putting processor into the stop mode with the LPTIM configured
 * to generate a wakeup interrrupt provides one of the lowest
 * power sleep modes for the STM32L0XX processor.
 *
 * The Low Power Timer's clock source is configured for the LSE
 * (Low Speed External) Clock which is driven by an external
 * 32,768 Hz watch crystal.  The LPTIM internal pre-scaler divides
 * the clock by 32 providing an 1024 Hz clock source with mS accuracy.
 *
 * The timer set / get function have units of mS which are internally
 * converted to timer counts.  This results in a small interval
 * rounding error which will be negligible for most use cases.
 *
 */

#ifndef LPTIM_H__
#define LPTIM_H__

#include "stm32l0xx.h"

#ifndef LPTIM1
#error "LPTIM1 Hardware Module Not Available!"
#endif

// The minimuim LPTIM programmaable time (mS)
#define LPTIM_MIN_MS 3

/**@brief Function for setting up LPTIM in one-shot mode with an interrupt
 * to expire at time period in the future.  This function initializes the LPTIM
 * if needed and enables the LPTIM and its interrupt.
 *
 * Note that the minimuim time interval is 3 mS.  Time intervals less than
 * this will be increased to 3 mS.
 *
 * @param[in] period_ms The counter duration in mS.
 */
void lptim_set(uint16_t period_ms);

/**@brief Function for getting the current value of LPTIM Counter
 * converted to mS.  When waking from stop, this time will represent
 * the interval stopped assuming the LPTIM had been started just
 * prior to stopping the processor.

 * This value can read on wakeup to determine if the processor was
 * stopped for the full programmed interval. It may have instead
 * been woken by a different source and not stopped for the full
 * desired interval.
 *
 * @return The current LPTIM counter value converted to mS.
 */
uint32_t lptim_ms_get(void);

/**
 *
 * Function disabling the LPTIM Module.
 */
void lptim_disable(void);

/**
 * Function stopping and deinitializing the LPTIM Module.
 * This places the LPTIM in the lowest poweer state.
 */
void lptim_deinit(void);

/**
 * @brief  Low Power Timer 1 Handler.
 * The LPTIM interrupt can be configured to wake the procesor from
 * the stop mode.   Note that interrupt handler vector must be set
 * in the startup code.
 */
void LPTIM1_IRQHandler(void);

#endif // LPTIM_H__