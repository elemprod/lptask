#include "do_pwr_lptim.h"
#include "stm32l0xx_ll_lptim.h"
#include "stm32l0xx_hal_lptim.h"
#include "stm32l0xx_ll_exti.h"
#include "stm32l0xx_ll_rcc.h"
#include "stm32l0xx_ll_cortex.h"
#include "stm32l0xx_hal.h"
#include "stm32l0xx.h"

// Define the Module ID for generating return codes.
#define RET_MOD_ID RET_MOD_ID_LPTIM

// LPTIM Timeout Timeoput in mS
#define DO_PWR_LPTIM_TIMEOUT_MS 100

// Function for Writing the LPTIM Compare Register
// Note LPTIM must be enabled before setting the register
static ret_code_t lptim_compare_set(uint32_t cmp_reg) {
  
  if(LL_LPTIM_GetCompare(LPTIM1) == cmp_reg)
    RET_ERR_CODE(RET_SUCCESS);     // Nothing to do

  // Write the Compare Register
  LL_LPTIM_SetCompare(LPTIM1, cmp_reg);
  uint32_t tickstart = HAL_GetTick();

  // Wait for the Write to Complete
  while(!LL_LPTIM_IsActiveFlag_CMPOK(LPTIM1)) {
    if ((HAL_GetTick() - tickstart) > DO_PWR_LPTIM_TIMEOUT_MS) {
      log_warn("lptim_compare_set() timeout");
      RET_ERR_CODE(RET_ERR_TIME_OUT);
    }
  }
  RET_ERR_CODE(RET_SUCCESS);
}

// Function for Writting the LPTIM Auto Reload Register
// Note LPTIM must be enabled before setting the register
static ret_code_t lptim_auto_reload_set(uint32_t arr_reg) {
  // Write the Auto Reload Value
  LL_LPTIM_SetAutoReload(LPTIM1, arr_reg);
  uint32_t tickstart = HAL_GetTick();
  // Wait for the Write to Complete
  while(!LL_LPTIM_IsActiveFlag_ARROK(LPTIM1)) {
    if ((HAL_GetTick() - tickstart) > DO_PWR_LPTIM_TIMEOUT_MS) {
      log_warn("lptim_auto_reload_set() timeout");
      RET_ERR_CODE(RET_ERR_TIME_OUT);
    }
  }
  RET_ERR_CODE(RET_SUCCESS);
}

/* LPTIM Register Summary
*
* LPTIM_ISR   Read Only   Read Any                 Interrupt Status
* LPTIM_ICR   Write Only  Write Any                Interrupt Clear
* LPTIM_IER   R/W         Write when Disabled      Interrupt Disable
* LPTIM_CFGR  R/W         Write when Disabled      Configuration
* LPTIM_CR    R/W         R/W Any                  Control
* LPTIM_CMP   R/W         Write when Enabled       Compare Register
* LPTIM_ARR   R/W         Write when Enabled       Autoreload Register
* LPTIM_CNT   Read Only   Read Any                 Counter Register (Double Read Required)
*/

/* LPTIM Configuration Register Value (LPTIM1->CFGR)
 *
 * ENC          0b0     Disabled      
 * COUNTMODE    0b0     Internal
 * PRELOAD      0b0     Registers Updated Immediately
 * WAVPOL       0b0 
 * WAVE         0b0
 * TIMOUT       0b0
 * TRIGEN       0b00    Software Trigger
 * TRIGSEL      0b000 
 * PRESC        0b101   Clock Divide by 32
 * TRGFLT       0b00
 * CKFLT        0b00
 * CKPOL        0b00
 * CKSEL        0b0     Internal Clock    
 */
#define DOP_LPTIM_CFGR  (0b101 << LPTIM_CFGR_PRESC_Pos)

// Setup the low power timer to be clocked by the LSE Clock 32,768 Hz
// with a divider of 32.  This results in a 1,024 Hz LPTIM clock rate. 
static ret_code_t lptim_init() {

  // Enable the LPTIM Perpheral Clock
  __HAL_RCC_LPTIM1_CLK_ENABLE();

  // Select the LSE clock as LPTIM peripheral clock
  __HAL_RCC_LPTIM1_CONFIG(RCC_LPTIM1CLKSOURCE_LSE);

  // Set the LPTIM Configuration Register
  if(LPTIM1->CFGR != DOP_LPTIM_CFGR) {
    // LPTIM must be disabled during Write 
    LPTIM1->CR = 0x00000000;
    LPTIM1->CFGR = DOP_LPTIM_CFGR; 
    //log_info("LPTIM1->CR Set 0x%X", LPTIM1->CFGR); 
  }

  // Set the LPTIM Interrupt Register to Compare Match Only
  if(LPTIM1->IER != (0b1  << LPTIM_ISR_CMPM_Pos)) {
      // LPTIM must be disabled during Write  
      LPTIM1->CR = 0x00000000;
      // Interrupt on Compare Match Only
      LPTIM1->IER = (0b1  << LPTIM_ISR_CMPM_Pos); 
  }

  // Enable the LPTIM
  LL_LPTIM_Enable(LPTIM1);

  // Set the Auto Reload Register to the Max Value 
  if(LL_LPTIM_GetAutoReload(LPTIM1) != 0XFFFF) {
    RET_ERROR_CHECK(lptim_auto_reload_set(0XFFFF));
  }

  HAL_NVIC_SetPriority(LPTIM1_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(LPTIM1_IRQn);

  RET_ERR_CODE(RET_SUCCESS);
}

ret_code_t lptim_set(uint16_t period_ms) {

  // Initialize the LPTIM is needed
  RET_ERROR_CHECK(lptim_init());

  // Calculate the compare value
#if (LPTIM_ACCURACY == 1)
  uint32_t compare_value = (period_ms * 1024) / 1000; // 1024 cnts per 10000 mS
#else
  uint32_t compare_value = period_ms;
#endif

  if(compare_value < 3)
    compare_value = 3;

  // The Compare value must be less than Auto Reload Value 
  assert(compare_value <  LL_LPTIM_GetAutoReload(LPTIM1));
    
  // Write the Compare Register Value / LPTIM must be enabled
  //log_info("Compare: %d", compare_value);
  LL_LPTIM_Enable(LPTIM1);
  RET_ERROR_CHECK(lptim_compare_set(compare_value));

  // Clear all of the LPTIM interrupt flags (Write Any)
  LPTIM1->ICR = 0x00000000;

  // Start the LPTIM in one-shot / no reload Mode.  Sets the SNGSTRT bit
  // Counts up from zero, compare event happens when reaching the compare value.
  LL_LPTIM_StartCounter(LPTIM1, LL_LPTIM_OPERATING_MODE_ONESHOT);
  
  RET_ERR_CODE(RET_SUCCESS);
  
}

// LPTIM Interrupt Handler, Only serves to wake the processor.
void do_pwr_lptim_irq_handler() {
  
  //log_info("LPTIM1_IRQHandler");

  // Any Write will Clear the Compare Match IRQ
  LL_LPTIM_ClearFLAG_CMPM(LPTIM1);
}


uint32_t lptim_ms_get() {
  volatile uint32_t prev_count = LL_LPTIM_GetCounter(LPTIM1);
  volatile uint32_t cur_count = LL_LPTIM_GetCounter(LPTIM1);

  // The counter must be repeatably read until subsquent counter
  // value reads match which indicates the last counter read was valid.
  while(prev_count != cur_count) {
    prev_count = cur_count;
    cur_count = LL_LPTIM_GetCounter(LPTIM1);
  }
#if (LPTIM_ACCURACY == 1)
  return (cur_count * 1000) / 1024; // 1000 mS per 1024 cnts per
#else
  return cur_count;
#endif
}




void lptim_disable() {

  // Disable the LP Timer
  LPTIM1->CR = 0x00000000;
  
  // Disable the LPTIM Peripheral Clock
  //__HAL_RCC_LPTIM1_CLK_DISABLE();

}

// Function deinitializing the lptim.
void lptim_deinit() {

  // Disable the LP Timer
  LPTIM1->CR = 0x00000000;
  // Disable the LPTIM Peripheral Clock
  __HAL_RCC_LPTIM1_CLK_DISABLE();
  //TODO should we do a full reset??
}