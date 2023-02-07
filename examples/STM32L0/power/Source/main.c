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

// Definitions for the different sleep types to use between scheduler tasks.
#define SLEEP_NONE 0    // Don't sleep between tasks
#define SLEEP_SYSTICK 1 // Sleep until the next systick interrupt
#define SLEEP_LPTIMER 2 // Stop the processor and setup the LPTIMER to wake up.

// Select which type of sleep to use.
#define SLEEP_METHOD SLEEP_LPTIMER

// LED0 GPIO & Port Definition's
// These defintions will need to be updated for the target board's hardware
#define LED0_PIN GPIO_PIN_8
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
static void led0_task_handler(void *p_context) {
  // Toggle the LED Output On / Off
  HAL_GPIO_TogglePin(LED0_PORT, LED0_PIN);
}

int main(void) {

  HAL_Init();

  // Initialize the Scheduler
  assert(sched_init());

  // Initialize the Power Hardware
  pwr_init();

  // Initialize the LED GPIO
  gpio_init();

  // Configure and start the LED0 Task to be called every 100 mS.
  sched_task_config(&led0_task, led0_task_handler, NULL, 100, true);
  sched_task_start(&led0_task);

  printf("STM32L0 Scheduler Example Boot");
#ifndef SLEEP_METHOD
#error "The sleep type must be defined"
#elif (SLEEP_METHOD == SLEEP_NONE)

  /*
   * Repeatably excecute the scheduler event que ignoring the returned task.
   * This is the simplest but most power intensive implementation.  It does
   * not sleep between task execution.
   */
  while (true) {
    sched_execute();
  }

#elif (SLEEP_METHOD == SLEEP_SYSTICK)
  /*
   * This implementation repeatably excecutes the scheduler event que ignoring
   * the return result and sleeping between active tasks. After entering sleep,
   * the processor can be woken from an hardware interrupts a which point it will
   * execute any tasks with expired timers.   The systick timer is setup to
   * trigger an interrupt once per mS which ensures tasks are executed
   * as they expire with mS granularity.
   */
  while (true) {
    sched_execute();
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
  }

#elif (SLEEP_METHOD == SLEEP_LPTIMER)
  /*
   * This implementation enters Stop Mode between executing tasks.
   * The next expiring task returned by the scheduler_execute() function is
   * used to determine the optimal sleep time.  Calculating the interval
   * until the next task expiration allows the implementation of a more
   * agressive power reduction technque.

   * The processor's systick timer is temporarily disabled in between task and
   * the devices low power timer is instead utilized for timing the
   * sleep intervals.   This prevents the processsor from needlessly waking
   * every mS to check if the task has expired.
   *
   */

  while (true) {
    // Execute the scheduler Que saving a reference to the next expiring task
    sched_task_t *p_next_task = sched_execute();
    if (p_next_task != NULL) {
      // Calculate the sleep interval
      uint32_t sleep_interval_ms = sched_task_remaining_ms(p_next_task);
      // TODO LPTIMER Sleep
    }
  }
#else
#error "Unrecognized Sleep Method"
#endif
}

/*************************** End of file ****************************/