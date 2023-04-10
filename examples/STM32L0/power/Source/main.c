/******************************************************************
 *                                                                 *
 *      Simple Scheduler Example for the STM2L0XXX Processor       *
 *                                                                 *
 *  Hardware Requirements:                                         *
 *                                                                 *
 *   STM2L053C8 Processor                                          *
 *   32.768 kHz External Low Speed Crystal                         *
 *   LED & current limiting resistor connected to a GPIO pin.      *
 *                                                                 *
 *******************************************************************/

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "pwr_mode.h"
#include "scheduler.h"
#include "stm32l0xx.h"
#include "stm32l0xx_hal.h"
#include "stm32l0xx_ll_gpio.h"

// LED0 GPIO & Port Definition
// These defintions may need to be updated for the target board's hardware

#if 1
  #define LED0_PIN GPIO_PIN_8
#else
  #define LED0_PIN GPIO_PIN_7
#endif

#define LED0_PORT GPIOB
#define LED0_PORT_CLK_EN() __HAL_RCC_GPIOB_CLK_ENABLE()

// Function for initializing the GPIO Pins's.
void gpio_init() {

  // Enable the LED0 Port GPIO Clock
  LED0_PORT_CLK_EN();

  // Initialize the LED0 GPIO as a Low Speed Output
  GPIO_InitTypeDef gpio_init;
  gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
  gpio_init.Pull = GPIO_NOPULL;
  gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
  gpio_init.Pin = LED0_PIN;
  HAL_GPIO_Init(LED0_PORT, &gpio_init);

  HAL_GPIO_WritePin(LED0_PORT, LED0_PIN, GPIO_PIN_SET);
}

// LED0 Toggle Scheduler Task
SCHED_TASK_DEF(led0_task);

// LED0 Toggle Scheduler Handler
static void led0_task_handler(sched_task_t *p_task, void *p_data, uint8_t data_size) {
  // Toggle the LED Output On / Off
  HAL_GPIO_TogglePin(LED0_PORT, LED0_PIN);
}

int main(void) {

  // Initialize the SDK
  HAL_Init();

  // Initialize the Power Hardware
  pwr_init();

  // Initialize the LED GPIO
  gpio_init();

  // Initialize the scheduler before configuring the tasks.
  sched_init();

  // Configure and start the LED0 Task to be called every 250 mS.
  sched_task_config(&led0_task, led0_task_handler, 250, true);
  sched_task_start(&led0_task);

  printf("STM32L0 LED Fast Blink Example\n"); 

  // Start the scheduler (doesn't return)
  sched_start();
}

/*************************** End of file ****************************/