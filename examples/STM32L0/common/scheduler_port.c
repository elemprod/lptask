
#include "scheduler_port.h"
#include "stm32l0xx_hal.h"

// IRQ Priority Mask 
static uint32_t primask_bit;

void scheduler_port_que_lock(void) {
  // Store the current IRQ Priority Mask  
  primask_bit = __get_PRIMASK();
  // Temporarily Disable Global Interrupts
  __disable_irq();
}

void scheduler_port_que_free(void) {
  // Restore previous IRQ Priority Mask
  __set_PRIMASK(primask_bit);
}

uint32_t scheduler_port_ms(void) {
  return (uint32_t) HAL_GetTick();
}