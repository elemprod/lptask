/* 
 * Platform (Processor Type) Specific Scheduler Configuration
 *
 */



 /* Macro for disabling global interrupts when entering a critical region of code
  * which requires exclusive write accesss to the scheduler's linked list event que.
 */
#define SCHEDULER_CRITICAL_REGION_ENTER()         \
  // Store the current IRQ Priority Mask          \
  uint32_t primask_bit = __get_PRIMASK();         \ 
  // Temporarily Disable Gloabal Interrupts       \
  __disable_irq();

/* Macro for enabling global interrupts when exitting a critical region of code.
 */
#define SCHEDULER_CRITICAL_REGION_EXIT()          \
    // Restore previous IRQ Priority Mask         \
    __set_PRIMASK(primask_bit);