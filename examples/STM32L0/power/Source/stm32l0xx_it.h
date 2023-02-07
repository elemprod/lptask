/******************************************************************************/
/*                                                                            */
/* @file  stm32l0xx_it.h                                                      */
/*                                                                            */
/* @brief Top Level Interrupt Service Routine Handler                         */
/*                                                                            */
/*        Exception ISR's are handled directly by the module.                 */
/*        Hardware ISR's are redirected to the hardware driver modules.       */
/*                                                                            */
/*        A top level ISR modules enables mapping the ISR's to multiple       */
/*        drivers which enables support for shared interrupts                 */
/*                                                                            */
/******************************************************************************/

#ifndef __STM32L0xx_IT_H
#define __STM32L0xx_IT_H

/******************************************************************************
 *            Cortex-M0+ Processor Exceptions Handlers                        *
 ******************************************************************************/

/* Unimplemented Exceptions:
 *
 *  MemManage_Handler
 *  BusFault_Handler
 *  UsageFault_Handler
 *  DebugMon_Handler
 */

/**
 * @brief NMI exception handler.
 */
void NMI_Handler(void);

/**
 * @brief Hard Fault exception handler.
 */
void HardFault_Handler(void);

/**
 * @brief  SVCall exception handler.
 */
void SVC_Handler(void);

/**
 * @brief  PendSVC exception handler.
 */
void PendSV_Handler(void);

/******************************************************************************/
/*                          System Clock Handler                              */
/******************************************************************************/

/**
 * @brief  SysTick handler.
 */
void SysTick_Handler(void);

/******************************************************************************/
/*                      Hardware Interrupt Handler's                          */
/******************************************************************************/

// Generic IRQ Handler Function Type
typedef void (*irq_handler_t)(void);

/**
 * @brief  Window Watchdog handler.
 */
void WWDG_IRQHandler(void);

/**
 * @brief  PVD through EXTI Line detecthandler.
 */
void PVD_IRQHandler(void);
/**
 * @brief  RTC through EXTI Line handler.
 */
void RTC_IRQHandler(void);
/**
 * @brief  FLASH handler.
 */
void FLASH_IRQHandler(void);
/**
 * @brief  Shared RCC and CRS handler.
 */
void RCC_CRS_IRQHandler(void);

/**
 * @brief  Shared External Interrupt Channel 0 & 1 handler.
 */
void EXTI0_1_IRQHandler(void);

/**
 * @brief  Shared External Interrupt Channel 2 & 3 handler.
 */
void EXTI2_3_IRQHandler(void);

/**
 * @brief  Shared External Interrupt Channel 4 to 15 handler.
 */
void EXTI4_15_IRQHandler(void);

/**
 * @brief  Touch Sensor IRQ handler.
 */
void TSC_IRQHandler(void);

/**
 * @brief  DMA1 Channel 1 handler.
 */
void DMA1_Channel1_IRQHandler(void);

/**
 * @brief  Shared DMA1 Channel 2 & 3 handler.
 */
void DMA1_Channel2_3_IRQHandler(void);

/**
 * @brief  Shared DMA1 Channel 4, 5, 6 & 7 handler.
 */
void DMA1_Channel4_5_6_7_IRQHandler(void);

/**
 * @brief  Shared ADC1, COMP1 and COMP2 handler.
 */
void ADC1_COMP_IRQHandler(void);

/**
 * @brief  LPTIM1 handler.
 */
void LPTIM1_IRQHandler(void);

/**
 * @brief  TIM2 handler.
 */
void TIM2_IRQHandler(void);

/**
 * @brief  Shared TIM6 & DAC handler.
 */
void TIM6_DAC_IRQHandler(void);

/**
 * @brief  TIM21 handler.
 */
void TIM21_IRQHandler(void);

/**
 * @brief  TIM22 handler.
 */
void TIM22_IRQHandler(void);

/**
 * @brief  I2C1 handler.
 */
void I2C1_IRQHandler(void);

/**
 * @brief  I2C2 handler.
 */
void I2C2_IRQHandler(void);

/**
 * @brief  SPI1 handler.
 */
void SPI1_IRQHandler(void);

/**
 * @brief  SPI2 handler.
 */
void SPI2_IRQHandler(void);

/**
 * @brief  USART1 handler.
 */
void USART1_IRQHandler(void);

/**
 * @brief  USART2 handler.
 */
void USART2_IRQHandler(void);

/**
 * @brief  Shared RNG & LPUART1 handler.
 */
void RNG_LPUART1_IRQHandler(void);

/**
 * @brief  LCD handler.
 */
void LCD_IRQHandler(void);

/**
 * @brief  USB handler.
 */
void USB_IRQHandler(void);

#endif /* __STM32L0xx_IT_H */