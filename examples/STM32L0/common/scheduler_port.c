
#include "scheduler_port.h"
#include "pwr_mode.h"
#include "stm32l0xx_hal.h"
#include <stdio.h>

// Uncomment to Log the Sleep Method at Scheduler Init
#define LOG_SLEEP_METHOD

// Sleep Method Definition.

// Don't sleep between tasks
#define SLEEP_NONE 0
// Stop the processor and use the systick timer to wake it.
#define SLEEP_SYSTICK 1
// Stop the processor and use the LPTIMER to wake it.
#define SLEEP_LPTIMER 2

// Select the sleep method to use between scheduler tasks.
#define SLEEP_METHOD SLEEP_NONE

// IRQ Priority Mask
static uint32_t primask_bit;

void scheduler_port_que_lock(void) {
  // Store the current IRQ Priority Mask
  primask_bit = __get_PRIMASK();
  // Temporarily Disable Global Interrupts
  __disable_irq();
}

void scheduler_port_que_free(void) {
  // Restore previous IRQ Priority Mask
  __set_PRIMASK(primask_bit);
}

uint32_t scheduler_port_ms(void) {
  return (uint32_t)HAL_GetTick();
}

void scheduler_port_sleep(uint32_t interval_ms) {

#ifndef SLEEP_METHOD
#error "The sleep type must be defined"
#elif (SLEEP_METHOD == SLEEP_NONE)
  /*
   * This sleep method repeatably execute the scheduler event que without
   * sleeping between tasks.  This is the simplest but most power intensive
   * sleep implementation.
   */

#elif (SLEEP_METHOD == SLEEP_SYSTICK)
  /*
   * This implementation repeatably executes the scheduler event que
   * stopping for up to 1mS during each function call.  Once put into the
   * stop mode, the processor can be only be woken due to a hardware 
   * interrupt at which point the function returns and the scheduler 
   * executes any tasks with expired timers.  The systick timer is 
   * configured to trigger an interrupt once per mS which ensures tasks 
   * are executed as they expire with mS granularity.
   */
  HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);

#elif (SLEEP_METHOD == SLEEP_LPTIMER)
  /*
   * This implementation enters the processor's Stop Mode between active
   * tasks.  The LPTIMER is configured to wake the processor once
   * the next taks interval expires.  The sleep method provides a more
   * aggressive power reduction technique.

   * The systick timer is temporarily disabled in between active tasks and
   * the low power timer is instead utilized for timing the sleep intervals.
   * This method prevents the processor from needlessly waking every mS to
   * check if the task has expired.
   */
  pwr_stop_lptim(interval_ms);
#else
#error "Unrecognized Sleep Method"
#endif
}

void scheduler_port_init(void) {

// Log the Sleep Method during scheduler init if enabled.
#if (SLEEP_METHOD == SLEEP_NONE)
  printf("Sleep Method: NONE\n");
#elif (SLEEP_METHOD == SLEEP_SYSTICK)
  printf("Sleep Method: SYSTICK\n");
#elif (SLEEP_METHOD == SLEEP_LPTIMER)
  printf("Sleep Method: LPTIMER\n");
#else
  printf("Sleep Method: Unrecognized\n");
#endif

  // Initialize the Power Module
  pwr_init();
}