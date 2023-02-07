#include "stm32l0xx_it.h"
#include "stm32l0xx_hal.h"
#include <stdio.h>
#include <stdlib.h>

/******************************************************************************
 *            Cortex-M0+ Processor Exceptions Handlers                        *
 ******************************************************************************/

void NMI_Handler(void) {
}

void HardFault_Handler(void) {
  printf("HardFault_Handler()");

  // Go to infinite loop if Hard Fault exception occurs.
  while (1) {
  }
}

void SVC_Handler(void) {
  printf("SVC_Handler()");
}

void PendSV_Handler(void) {
  printf("PendSV_Handler()");
}

void SysTick_Handler(void) {
  HAL_IncTick();
}

/**
 * @brief  Window Watchdog handler.
 */
void WWDG_IRQHandler(void) {
  printf("WWDG_IRQHandler()");
}

/**
 * @brief  PVD through EXTI Line detect handler.
 */
void PVD_IRQHandler(void) {
  printf("PVD_IRQHandler()");
}
/**
 * @brief  RTC through EXTI Line handler.
 */
void RTC_IRQHandler(void) {
  printf("RTC_IRQHandler()");
}
/**
 * @brief  FLASH handler.
 */
void FLASH_IRQHandler(void) {
  printf("FLASH_IRQHandler()");
}
/**
 * @brief  Shared RCC and CRS handler.
 */
void RCC_CRS_IRQHandler(void) {
  printf("RCC_CRS_IRQHandler()");
}

/**
 * @brief  Shared External Interrupt Channel 0 & 1 handler.
 */
void EXTI0_1_IRQHandler(void) {
  printf("EXTI0_1_IRQHandler()");
}

/**
 * @brief  Shared External Interrupt Channel 2 & 3 handler.
 */
void EXTI2_3_IRQHandler(void) {
  printf("EXTI2_3_IRQHandler()");
}

/**
 * @brief  Shared External Interrupt Channel 4 to 15 handler.
 */
void EXTI4_15_IRQHandler(void) {
  printf("EXTI4_15_IRQHandler()");
}

/**
 * @brief  Touch Sensor IRQ handler.
 */
void TSC_IRQHandler(void) {
  printf("TSC_IRQHandler()");
}

/**
 * @brief  DMA1 Channel 1 handler.
 */
void DMA1_Channel1_IRQHandler(void) {
  printf("DMA1_Channel1_IRQHandler()");
}

/**
 * @brief  Shared DMA1 Channel 2 & 3 handler.
 */
void DMA1_Channel2_3_IRQHandler(void) {
  printf("DMA1_Channel2_3_IRQHandler()");
}

/**
 * @brief  Shared DMA1 Channel 4, 5, 6 & 7 handler.
 */
void DMA1_Channel4_5_6_7_IRQHandler(void) {
  printf("DMA1_Channel4_5_6_7_IRQHandler()");
}

/**
 * @brief  Shared ADC1, COMP1 and COMP2 handler.
 */
void ADC1_COMP_IRQHandler(void) {
  printf("ADC1_COMP_IRQHandler()");
}

/**
 * @brief  Low Power Timer 1 handler.
 */
#if 0
void LPTIM1_IRQHandler(void) {
  // Implemented in the LPTIM module.
  printf("LPTIM1_IRQHandler()");
}
#endif

/**
 * @brief  TIM2 handler.
 */
void TIM2_IRQHandler(void) {
  printf("TIM2_IRQHandler()");
}

/**
 * @brief  Shared TIM6 & DAC handler.
 */
void TIM6_DAC_IRQHandler(void) {
  printf("TIM6_DAC_IRQHandler()");
}

/**
 * @brief  TIM21 handler.
 */
void TIM21_IRQHandler(void) {
  printf("TIM21_IRQHandler()");
}

/**
 * @brief  TIM22 handler.
 */
void TIM22_IRQHandler(void) {
  printf("TIM22_IRQHandler()");
}

/**
 * @brief  I2C1 handler.
 */
void I2C1_IRQHandler(void) {
  printf("I2C1_IRQHandler()");
}

/**
 * @brief  I2C2 handler.
 */
void I2C2_IRQHandler(void) {
  printf("I2C2_IRQHandler()");
}

/**
 * @brief  SPI1 handler.
 */
void SPI1_IRQHandler(void) {
  printf("SPI1_IRQHandler()");
}

/**
 * @brief  SPI2 handler.
 */
void SPI2_IRQHandler(void) {
  printf("SPI2_IRQHandler()");
}

/**
 * @brief  USART1 handler.
 */
void USART1_IRQHandler(void) {
  printf("USART1_IRQHandler()");
}

/**
 * @brief  USART2 handler.
 */
void USART2_IRQHandler(void) {
  printf("USART2_IRQHandler()");
}

/**
 * @brief  Shared RNG & LPUART1 handler.
 */
void RNG_LPUART1_IRQHandler(void) {
  printf("RNG_LPUART1_IRQHandler()");
}

/**
 * @brief  LCD handler.
 */
void LCD_IRQHandler(void) {
  printf("LCD_IRQHandler()");
}

/**
 * @brief  USB handler.
 */
void USB_IRQHandler(void) {
  printf("USB_IRQHandler()");
}