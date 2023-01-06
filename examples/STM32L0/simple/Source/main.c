/******************************************************************
*                                                                 *
*      Simple Scheduler Example for the STM2L0XXX Processor       *
*                                                                 *
*  Hardware Requirements:                                         *
*                                                                 *
*   STM2L053C8 Processor                                          *
*   LED & current limiting resistor connected to Port B, GPIO 10  *
*                                                                 *
*                                                                 *
*******************************************************************/

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "stm32l0xx.h"
#include "stm32l0xx_ll_gpio.h"
#include "stm32l0xx_hal.h"
#include "scheduler.h"

// LED0 GPIO & Port Definition's
// Thesee defintions will need to be update for the target board's hardware
#define LED0_PIN               GPIO_PIN_10
#define LED0_PORT              GPIOB   
#define LED0_PORT_CLK_EN()     __HAL_RCC_GPIOB_CLK_ENABLE()

// Function for initializing the GPIO Pins's.
void gpio_init() {

  // Enable the LED0 Port GPIO Clock
  LED0_PORT_CLK_EN();
  
  // Initialize the LED0 GPIO as a Low Speed Output
  GPIO_InitTypeDef   gpio_init;
  gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
  gpio_init.Pull = GPIO_NOPULL;
  gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
  gpio_init.Pin = LED0_PIN;
  HAL_GPIO_Init(LED0_PORT, &gpio_init);

}

// LED0 Toggle Scheduler Task
SCHED_TASK_DEF(led0_task);

// LED0 Toggle Scheduler Handler
static void led0_task_handler(void * p_context) {
  // Toggle the LED Output On / Off
  HAL_GPIO_TogglePin(LED0_PORT, LED0_PIN);
}

int main(void) {

  // Initialize the Scheduler
  sched_init();

  // Initialize the LED GPIO
  gpio_init();

  // Configure and start the LED0 Task to be called every 100 mS.
  sched_task_config(&led0_task, led0_task_handler, NULL, 100, true);
  sched_task_start(&led0_task);

  /* 
  * Repeatably excecute the scheduler event que,.  This is the simplest
  * but most power intensive implementation becausee it does not sleep
   * between task execution.
  */
  while(true) {
    sched_execute();
  }
}

/*************************** End of file ****************************/
