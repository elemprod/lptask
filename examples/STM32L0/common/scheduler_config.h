/* 
 * STM32L0 Platform Specific Scheduler Configuration & Support Functions
 *
 */

#include "stm32l0xx_hal.h"

 /**
  * @brief Macro for disabling global interrupts when entering a critical region of code
  * which requires exclusive write accesss to the scheduler's linked list event que.
  */
#define SCHEDULER_CRITICAL_REGION_ENTER()         \
  // Store the current IRQ Priority Mask          \
  uint32_t primask_bit = __get_PRIMASK();         \
  // Temporarily Disable Gloabal Interrupts       \
  __disable_irq();

/**
  * @brief Macro for enabling global interrupts when exitting a critical region of code.
  */
#define SCHEDULER_CRITICAL_REGION_EXIT()          \
    // Restore previous IRQ Priority Mask         \
    __set_PRIMASK(primask_bit);

 /**
 * @brief Function for checking if the mS timer utilized by the scheduler 
 * is currently active.
 *
 * Most implementations can simply return true here.  Platforms which 
 * implement a timer that can be stopped, during a low power mode
 * for example, should return false if the mS timer is not currently 
 * running.
 *
 * @return    True if the scheduler's timer is currently active.
 */
static inline bool scheduler_tick_active() {
  return (bool) (SysTick->CTRL & SysTick_CTRL_TICKINT_Msk);
}

 /**
 * @brief Function for getting the current value of the mS timer 
 * which is utilized by the scheduler.
 *
 * @return    The current timer value (mS).
 */
static inline uint32_t scheduler_get_tick() {
  return (uint32_t) HAL_GetTick();
}