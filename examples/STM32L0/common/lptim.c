
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "lptim.h"
#include "stm32l0xx.h"
#include "stm32l0xx_hal.h"
#include "stm32l0xx_hal_lptim.h"
#include "stm32l0xx_ll_cortex.h"
#include "stm32l0xx_ll_exti.h"
#include "stm32l0xx_ll_lptim.h"
#include "stm32l0xx_ll_rcc.h"

// LPTIM Timeout Timeoput in mS
#define LPTIM_WRITE_TIMEOUT_MS 100

// LPTIM Auto Reload Register (Set to Max Value)
#define LPTIM_ARR_MAX 0xFFFF

// LPTIM Minimuim Settable time (mS)
#define LPTIM_TIME_MIN_MS 3

// Function for Writing  the LPTIM Compare Register
// Note LPTIM must be enabled before setting the register
static void lptim_compare_set(uint16_t cmp_reg) {

  if (LL_LPTIM_GetCompare(LPTIM1) != cmp_reg) {

    // Write the Compare Register
    LL_LPTIM_SetCompare(LPTIM1, cmp_reg);
    uint32_t tickstart = HAL_GetTick();

    // Wait for the Write to Complete
    while (!LL_LPTIM_IsActiveFlag_CMPOK(LPTIM1)) {
      // Write time out check
      assert((HAL_GetTick() - tickstart) < LPTIM_WRITE_TIMEOUT_MS);
    }
  }
}

// Function for Writting the LPTIM Auto Reload Register
// Note LPTIM must be enabled before setting the register
static void lptim_auto_reload_set(uint16_t arr_reg) {

  if (LL_LPTIM_GetAutoReload(LPTIM1) != arr_reg) {

    // Write the Auto Reload Value
    LL_LPTIM_SetAutoReload(LPTIM1, arr_reg);
    uint32_t tickstart = HAL_GetTick();
    // Wait for the Write to Complete
    while (!LL_LPTIM_IsActiveFlag_ARROK(LPTIM1)) {
      // Write time out check
      assert((HAL_GetTick() - tickstart) < LPTIM_WRITE_TIMEOUT_MS);
    }
  }
}

/*
 * LPTIM Register Summary
 *
 * NAME         ACCESS      RESTRICTION           DESCRIPTION
 * LPTIM_ISR    Read Only   Read Anytime          Interrupt Status
 * LPTIM_ICR    Write Only  Write Anytime         Interrupt Clear
 * LPTIM_IER    R/W         Write when Disabled   Interrupt Disable
 * LPTIM_CFGR   R/W         Write when Disabled   Configuration
 * LPTIM_CR     R/W         R/W Anytime           Control
 * LPTIM_CMP    R/W         Write when Enabled    Compare Register
 * LPTIM_ARR    R/W         Write when Enabled    Autoreload Register
 * LPTIM_CNT    Read Only   Read Any              Counter Register
 *                          (Double Read Required)
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
#define MY_LPTIM_CFGR (0b101 << LPTIM_CFGR_PRESC_Pos)

// Setup the low power timer to be clocked by the LSE Clock 32,768 Hz
// with a prescaler of 32.  This results in a 1,024 Hz LPTIM clock rate.
static void lptim_init(void) {

  // Enable the LPTIM Perpheral Clock
  __HAL_RCC_LPTIM1_CLK_ENABLE();

  // Select the LSE clock as LPTIM peripheral clock
  __HAL_RCC_LPTIM1_CONFIG(RCC_LPTIM1CLKSOURCE_LSE);

  // Set the LPTIM Configuration Register
  if (LPTIM1->CFGR != MY_LPTIM_CFGR) {
    // LPTIM must be disabled during Write
    LPTIM1->CR = 0x00000000;
    LPTIM1->CFGR = MY_LPTIM_CFGR;
  }

  // Set the LPTIM Interrupt Register to Compare Match Only
  if (LPTIM1->IER != (0b1 << LPTIM_ISR_CMPM_Pos)) {
    // LPTIM must be disabled during Write
    LPTIM1->CR = 0x00000000;
    // Interrupt on Compare Match Only
    LPTIM1->IER = (0b1 << LPTIM_ISR_CMPM_Pos);
  }

  // Enable the LPTIM
  LL_LPTIM_Enable(LPTIM1);

  // Set the Auto Reload Register to the Max Value
  lptim_auto_reload_set(LPTIM_ARR_MAX);

  // Enable the interrupt
  HAL_NVIC_SetPriority(LPTIM1_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(LPTIM1_IRQn);
}

void lptim_set(uint16_t period_ms) {

  uint32_t compare_value = LPTIM_TIME_MIN_MS;

  // Initialize the LPTIM is needed
  lptim_init();

  // Calculate the compare value using the scaling of 1024 cnts per 1000 mS
  // if above the min. period.  Limit the counter to the auto reload value
  if (period_ms > LPTIM_TIME_MIN_MS) {
    compare_value = ((period_ms * 1024) / 1000) & LPTIM_ARR_MAX;
  }

  // Write the Compare Register Value / LPTIM must be enabled
  LL_LPTIM_Enable(LPTIM1);
  lptim_compare_set(compare_value);

  // Clear all of the LPTIM interrupt flags (Write Any Access)
  LPTIM1->ICR = 0x00000000;

  // Start the LPTIM in one-shot / no reload Mode.  Sets the SNGSTRT bit
  // Counts up from zero, compare event happens when reaching the compare value.
  LL_LPTIM_StartCounter(LPTIM1, LL_LPTIM_OPERATING_MODE_ONESHOT);
}

uint32_t lptim_ms_get(void) {
  volatile uint32_t prev_count = LL_LPTIM_GetCounter(LPTIM1);
  volatile uint32_t cur_count = LL_LPTIM_GetCounter(LPTIM1);

  // The counter must be repeatably read until subsequent counter
  // value reads match which indicates the last counter read was valid.
  while (prev_count != cur_count) {
    prev_count = cur_count;
    cur_count = LL_LPTIM_GetCounter(LPTIM1);
  }
  // 1000 mS per 1024 cnts
  return (cur_count * 1000) / 1024;
}

void lptim_disable(void) {

  // Disable the LP Timer
  LPTIM1->CR = 0x00000000;
}

// Function for deinitializing the lptim.
void lptim_deinit(void) {

  // Disable the LP Timer
  LPTIM1->CR = 0x00000000;

  // Disable the LPTIM Peripheral Clock
  __HAL_RCC_LPTIM1_CLK_DISABLE();
}

void LPTIM1_IRQHandler(void) {

  // Any Write will Clear the Compare Match IRQ
  LL_LPTIM_ClearFLAG_CMPM(LPTIM1);
}