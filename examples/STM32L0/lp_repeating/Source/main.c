/*********************************************************************
*     Scheduler Example Program for the STM2L053C8 Processor         *
*                                             *
*                                             *
*                                             *
*  H/W Requirements:                                           *
*   STM2L053C8 Processor                                      *
*   Low Speed External (LSE) 32,768 Hz Crystal Oscillator            *
*   LED's                                          *
*                                             *
*                                             *
**********************************************************************/

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "stm32l0xx.h"
#include "stm32l0xx_hal_rcc.h"
#include "stm32l0xx_ll_rcc.h"

#include "scheduler.h"


// Low Speed External Crystal Drive Strength.
// The drive strength can be decreased for certain crystal types to reduce power.
#define LSE_DRIVE        LL_RCC_LSEDRIVE_MEDIUMHIGH        

// RCC Init Structure for both the Low Speed Clocks (LSE & LSI) Disabled 
const RCC_OscInitTypeDef RCC_LSI_LSE_DISABLE = {
    .OscillatorType   = RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_LSE,
    .PLL.PLLState     = RCC_PLL_NONE,
    .LSEState         = RCC_LSE_OFF,
    .LSIState         = RCC_LSI_OFF
  };

// RCC Init Structure with the Low Speed External Clock (LSE) Enabled and the LSI Disabled
const RCC_OscInitTypeDef RCC_LSE_ENABLE = {
    .OscillatorType   = RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_LSE,
    .PLL.PLLState     = RCC_PLL_NONE,
    .LSEState         = RCC_LSE_ON,
    .LSIState         = RCC_LSI_OFF
  };

 /**
 * @brief Function for enabling or disabling the Low Speed External Clock.
 *
 * @param[in] enable   True to enable the LSE clock
 *
 * @return    none.
 *
 */  
static void lse_clk_enable(bool enable) {

  if(enable) {

    // Read the Current RCC Clock Configuration
    RCC_OscInitTypeDef  rcc_osc_current;
    HAL_RCC_GetOscConfig(&rcc_osc_current);

    // Configure LSI / LSE Clock if it's in wrong state
    if(rcc_osc_current.LSIState == RCC_LSI_ON ||
      rcc_osc_current.LSEState == RCC_LSE_OFF ||
      rcc_osc_current.LSEState == RCC_LSE_BYPASS) {

      // Disable the Internal & External Low Speed Oscillators first.
      // LSE transitions to the ON are only allowed from Off State and 
      // not directly from Bypass Mode.
     assert(HAL_RCC_OscConfig((RCC_OscInitTypeDef *) &RCC_LSI_LSE_DISABLE) == HAL_OK);
    
      // Set the LSE Drive Strength
      LL_RCC_LSE_SetDriveCapability(LSE_DRIVE);

      // Enable the LSE Oscillator
      assert(HAL_RCC_OscConfig((RCC_OscInitTypeDef *) &RCC_LSE_ENABLE) == HAL_OK);
    }
  } else {
      // Disable the Internal & External Low Speed Oscillator
      assert(HAL_RCC_OscConfig((RCC_OscInitTypeDef *) &RCC_LSI_LSE_DISABLE) == HAL_OK);
  }

}


/**@brief Function for putting the processor in Stop Mode with the Low Power
 * Timer setup up to wake it up after a programable time period.
 
 * Note: The processor will be in Run after after waking  once the 
 *        specified time period has expired or if woken from a different IRQ source. 
 *
 * @param[in] period_ms The duration to sleep in mS.

 * @returns   RET_SUCCESS if the power set mode was sucessful.
 *
 *            RET_BUSY if one of the modules can not enter sleep.
 */

static void stop_mode_lptim(uint16_t period_ms) {
   
  if(period_ms > 2) {

    // Stop the SysTick IRQ / Clock
    HAL_SuspendTick();

    // Enable the LPTIM with the desired delay
    RET_ERROR_CHECK(lptim_set(period_ms));

    // WFI Logic:
    // Each of the ISR's starts a scheduler event so we need for the scheduler to run after
    // each interrupt. Theres no advantage to using the WFE or using auto sleep on ISR exit.
  
    // Clear the Wakup Flag & Enter Stop Mode
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
    
    // Update the SysTick with the sleep durration on wakeup.
    uwTick += lptim_ms_get();
    //log_info("uwTick %d", uwTick);

    // Disable the LPTIM
    lptim_disable();

    // Restart the SysTick Irq.
    HAL_ResumeTick();
    return RET_SUCCESS;

  } else {
    // Just sleep without disabling the SysTic for short time periods
    // The SysTic IRQ will wake the processor at the next Tick.
    return do_pwr_sleep();
  }

}

/*********************************************************************
*
*       main()
*
*  Function description
*   Application entry point.
*/
int main(void) {
  int i;

  for (i = 0; i < 100; i++) {
    printf("Hello World %d!\n", i);
  }
  do {
    i++;
  } while (1);
}

/*************************** End of file ****************************/
