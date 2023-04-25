

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "stm32l0xx_hal.h"
#include "stm32l0xx_ll_cortex.h"

#include "stm32l0xx.h"
#include "stm32l0xx_hal.h"
#include "stm32l0xx_hal_lptim.h"
#include "stm32l0xx_ll_cortex.h"
#include "stm32l0xx_ll_exti.h"
#include "stm32l0xx_ll_lptim.h"
#include "stm32l0xx_ll_rcc.h"

#include "lptim.h"
#include "pwr_mode.h"
#include "stm32l0xx_ll_pwr.h"
#include "stm32l0xx_ll_rcc.h"

/* Low Speed External (LSE) Crystal Drive Strength.
 * The oscillator drive strength should be matched to minimuim drive
 * required by the external crystal utilized by the target hardware.
 * Refer to AN2867 Oscillator Design Guide Table 7
 *
 * Drive Strengths for STM32L053 per Datasheet
 * LL_RCC_LSEDRIVE_LOW           0.5 μA/V
 * LL_RCC_LSEDRIVE_MEDIUMLOW     0.75 μA/V
 * LL_RCC_LSEDRIVE_MEDIUMHIGH    1.7 μA/V
 * LL_RCC_LSEDRIVE_HIGH          2.7 μA/V
 *
 * The CM7V-T1A-32.768KHZ-6PF-20PPM-TB-QA Crystals requires a min. drive of 0.6152 uA/V
 *
 */
#ifndef PWR_LSE_DRIVE
#define PWR_LSE_DRIVE LL_RCC_LSEDRIVE_MEDIUMLOW
#endif

/* RCC Init Structure with both the Low Speed Clocks (LSE & LSI) Disabled
 * Only used for Stop Mode.
 */
const RCC_OscInitTypeDef RCC_LSI_LSE_DISABLE = {
    .OscillatorType = RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_LSE,
    .PLL.PLLState = RCC_PLL_NONE,
    .LSEState = RCC_LSE_OFF,
    .LSIState = RCC_LSI_OFF};

/* RCC Init Structure with the Low Speed External Clock (LSE) Enabled and
 * the LSI Disabled.  Used for all power modes except Stop Mode
 */
const RCC_OscInitTypeDef RCC_LSE_ENABLE = {
    .OscillatorType = RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_LSE,
    .PLL.PLLState = RCC_PLL_NONE,
    .LSEState = RCC_LSE_ON,
    .LSIState = RCC_LSI_OFF};

// Function for enabling / disabling the Low Speed Clock
static void ls_clk_enable(bool enable) {
  HAL_StatusTypeDef ret;

  // Read the RCC Current Clock Configuration
  RCC_OscInitTypeDef rcc_osc_current;
  HAL_RCC_GetOscConfig(&rcc_osc_current);

  if (enable) {
    // Configure LSI / LSE Clock if its in wrong state
    if (rcc_osc_current.LSIState == RCC_LSI_ON ||
        rcc_osc_current.LSEState == RCC_LSE_OFF ||
        rcc_osc_current.LSEState == RCC_LSE_BYPASS) {

      // Disable the Internal & External Low Speed Oscillators first.
      // LSE transitions to the ON are only allowed from Off State (Not directly from Bypass)
      ret = HAL_RCC_OscConfig((RCC_OscInitTypeDef *) &RCC_LSI_LSE_DISABLE);
      assert(ret == HAL_OK);

      // Set the LSE Drive Strength
      LL_RCC_LSE_SetDriveCapability(PWR_LSE_DRIVE);

      // Enable the LSE Oscillator
      ret = HAL_RCC_OscConfig((RCC_OscInitTypeDef *) &RCC_LSE_ENABLE);
      assert(ret == HAL_OK);
    }
  } else {
    // Disable the Internal & External Low Speed Oscillator
    ret = HAL_RCC_OscConfig((RCC_OscInitTypeDef *) &RCC_LSI_LSE_DISABLE);
    assert(ret == HAL_OK);
  }
}

// Function for enabling the Regular Run Mode.
void pwr_run_msi() {
  HAL_StatusTypeDef ret;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;

  // Configure the Voltage Range (Up to 4.2 MHz)
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  // Enable MSI Oscillator to run at 4.194 MHz
  const RCC_OscInitTypeDef RCC_OSC_INIT_RUN_MODE = {
      .OscillatorType = RCC_OSCILLATORTYPE_MSI,
      .MSIState = RCC_MSI_ON,
      .MSIClockRange = RCC_MSIRANGE_6,
      .MSICalibrationValue = 0x00,
      .PLL.PLLState = RCC_PLL_NONE};
  ret = HAL_RCC_OscConfig((RCC_OscInitTypeDef *)&RCC_OSC_INIT_RUN_MODE);
  assert(ret == HAL_OK);

  // Select MSI as system clock source and configure the HCLK, PCLK1 and PCLK2 clocks dividers
  const RCC_ClkInitTypeDef RCC_CLK_INIT_RUN_MODE = {
      .ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2),
      .SYSCLKSource = RCC_SYSCLKSOURCE_MSI,
      .AHBCLKDivider = RCC_SYSCLK_DIV1,
      .APB1CLKDivider = RCC_HCLK_DIV1,
      .APB2CLKDivider = RCC_HCLK_DIV1};
  ret = HAL_RCC_ClockConfig((RCC_ClkInitTypeDef *)&RCC_CLK_INIT_RUN_MODE, FLASH_LATENCY_0);
  assert(ret == HAL_OK);

  // Set the Wake from Stop clock to the Medium Speed Internal Oscillator
  __HAL_RCC_WAKEUPSTOP_CLK_CONFIG(RCC_STOP_WAKEUPCLOCK_MSI);
}

void pwr_run_hsi() {
  HAL_StatusTypeDef ret;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;

  // Configure the Voltage Range (Up to 16 MHz / Medium Power)
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  // Enable the HSI Oscillator to run at 16 MHz
  const RCC_OscInitTypeDef RCC_OSC_INIT_HS = {
      .OscillatorType = RCC_OSCILLATORTYPE_HSI,
      .HSEState = RCC_HSE_OFF,
      .HSIState = RCC_HSI_ON,
      .PLL.PLLState = RCC_PLL_NONE,
      .HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT};
  ret = HAL_RCC_OscConfig((RCC_OscInitTypeDef *)&RCC_OSC_INIT_HS);
  assert(ret == HAL_OK);

  // Select HSI as system clock source and configure the HCLK, PCLK1 and PCLK2 clocks dividers
  // All buses set to run at 1 MHz
  const RCC_ClkInitTypeDef RCC_CLK_INIT_HSI = {
      .ClockType = (RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2),
      .SYSCLKSource = RCC_SYSCLKSOURCE_HSI,
      .AHBCLKDivider = RCC_SYSCLK_DIV1,
      .APB1CLKDivider = RCC_HCLK_DIV1,
      .APB2CLKDivider = RCC_HCLK_DIV1,
  };
  ret = HAL_RCC_ClockConfig((RCC_ClkInitTypeDef *)&RCC_CLK_INIT_HSI, FLASH_LATENCY_1);
  assert(ret == HAL_OK);

  // Set the Wake from Stop clock to the High Speed Internal Oscillator
  __HAL_RCC_WAKEUPSTOP_CLK_CONFIG(RCC_STOP_WAKEUPCLOCK_HSI);
}

void pwr_sleep() {
  HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
}

// Estimated Overhead in mS of using the Stop LPTIM Method
#define STOP_LPTIM_OVERHEAD_MS 4

void pwr_stop_lptim(uint16_t period_ms) {

  if (period_ms > STOP_LPTIM_OVERHEAD_MS) {

    // Stop the SysTick IRQ
    HAL_SuspendTick();

    // Enable the LPTIM with the desired delay minus the estimated overhead.
    lptim_set(period_ms - STOP_LPTIM_OVERHEAD_MS);

    // WFI Logic:
    // Each of the ISR's starts a scheduler event so we need for the scheduler to run after
    // each interrupt. Theres no advantage to using the WFE or using auto sleep on ISR exit.

    // Clear the Wakup Flag & Enter Stop Mode
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

    /* After waking up from stop, correct the SysTick counter with the time interval
     * that the processor was stopped for.  This interval may be different than the
     * programmed interval if the processor was woken from a different interrupt.
     */
    uwTick = uwTick + lptim_ms_get() + STOP_LPTIM_OVERHEAD_MS;

    // Disable the LPTIM
    lptim_disable();

    // Restart the SysTick Irq.
    HAL_ResumeTick();
  } else {
    // Just sleep without disabling the SysTick for short time periods
    // The SysTick IRQ will wake the processor at the next Tick.
    pwr_sleep();
  }
}

void pwr_init() {

  // Enable the RCC Power Control Clock
  __HAL_RCC_PWR_CLK_ENABLE();

  // Enable the Low Speed Clock
  ls_clk_enable(true);

  // Configure the Power Voltage Detector Threshold to 2.3V
  LL_PWR_SetPVDLevel(LL_PWR_PVDLEVEL_2);

  // Enable the Ultra Low Power (ULP) Mode for Stop and Standby modes.
  // Disables the internal Voltage Reference in Sleep and Stop.
  // The BOR, PVD, ADC, HSI48, LCD and comparators are all automatically disabled
  // with ULP is set in Stop and Standby modes.
  SET_BIT(PWR->CR, PWR_CR_ULP);

  // Enable the Fast Wakeup from Sleep & Stop.
  // The processor does not wait for the internal Voltage Reference after wake.
  // The VREFINTRDYF flag should be polled before use the internal reference.
  SET_BIT(PWR->CR, PWR_CR_FWU);

  // Start the medium speed run mode
  pwr_run_msi();
}