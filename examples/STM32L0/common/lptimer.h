/********************************************************************/
/*             STM32L0 Low Power Timer Module                       */
/*                                                                  */
/* The modules provides Low Power Timer (LPTIM) configuration and   */
/* support functions. The LPTIM hardware moduele can be used to     */
/* provides short duration wake from stop mode functionality.       */
/*                                                                  */
/* The Low Power Timer's clock source is the LSE Clock 32,768 Hz.   */                                                              
/* An internal pre-scacler divides the clock by 32 providing an     */
/* internal 1024 Hz clock.  The interface functions have mS units   */
/* which would result in a small 2.4% error if the counter is       */
/* not corrected with the LPTIM_ACCURACY define.                    */
/*                                                                  */
/********************************************************************/

#ifndef _LPTIMER_H__
#define _LPTIMER_H__

#ifndef LPTIM1
  #error LPTIM1 Hardware Module Not Available!
#endif

// The accuracy of the the Low Power Timer can be improved by aproximately 2.4%
// at the expense of some additional processing overhead if LPTIM_ACCURACY == 0.
#define LPTIM_ACCURACY 0

/**@brief Function for setting up LPTIM to expire at a time period in the 
 * future.  This function initializes the LPTIM, if needed  and enables 
 * the LPTIM counter and interrupt functionality.
 *
 * @param[in] period_ms The counter duration in mS.
 *
 * @return  RET_SUCESS
 *          RET_ERR_TIMEOUT if counter access fails.
 */
ret_code_t lptim_set(uint16_t period_ms);

/**@brief Function for getting the current value of LPTIM Counter converted 
 * to mS.  When waking from stop follwing an lptim_set() call, this value
 * represents the time period stopped 
 *
 * @return The current counter converted to mS.
 */
uint32_t lptim_ms_get();

// Function disabling the LPTIM Module. 
void lptim_disable();

// Function stopping and deinitializing the LPTIM Module. 
void lptim_deinit();


// LPTIM Interrupt Handler - Wakes the processor.
void do_pwr_lptim_irq_handler();

#endif // _LPTIMER_H__