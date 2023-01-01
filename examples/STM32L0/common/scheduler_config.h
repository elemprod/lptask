/* 
 * STM32L0 Platform Specific Scheduler Configuration & Support Functions
 *
 */

#ifndef _SCHEDULER_CONFIG_H__
#define _SCHEDULER_CONFIG_H__

#include "stm32l0xx_hal.h"

 /**
  * @brief Macro for disabling global interrupts when entering a critical region of code
  * which requires exclusive write accesss to the scheduler's linked list event que.
  */
#define SCHED_CRITICAL_REGION_ENTER()         \
  // Store the current IRQ Priority Mask          \
  uint32_t primask_bit = __get_PRIMASK();         \
  // Temporarily Disable Gloabal Interrupts       \
  __disable_irq();

/**
  * @brief Macro for enabling global interrupts when exitting a critical region of code.
  */
#define SCHED_CRITICAL_REGION_EXIT()          \
    // Restore previous IRQ Priority Mask         \
    __set_PRIMASK(primask_bit);

 /**
 * @brief Function for getting the current value of the mS timer 
 * which is utilized by the scheduler.
 *
 * @return    The current timer value (mS).
 */
static inline volatile uint32_t sched_get_ms() {
  return (uint32_t) HAL_GetTick();
}


#endif // _SCHEDULER_CONFIG_H__