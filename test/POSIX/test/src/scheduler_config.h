/* 
 * POSIX Platform Scheduler Configuration & Support Functions
 *
 */

#ifndef _SCHEDULER_CONFIG_H__
#define _SCHEDULER_CONFIG_H__

#include <time.h>
#include <math.h>
#include <pthread.h>

// Scheduler mutex
static pthread_mutex_t sched_mutex =  PTHREAD_MUTEX_INITIALIZER;

 /**
  * @brief Macro for ensuring exclusive access to the scheduler's linked list
  * event que.  The macro is utillized by the scheduler when enterring a critical
  * region of code.
  */
#define SCHEDULER_CRITICAL_REGION_ENTER()         \
  pthread_mutex_lock(&sched_mutex);

/**
* @brief Macro for releasing exclusive access to the scheduler's linked list
 * event que .  The macro is utillized by the scheduler when exiting a critical
 * region of code.
  */
#define SCHEDULER_CRITICAL_REGION_EXIT()          \
  pthread_mutex_unlock(&sched_mutex);

 /**
 * @brief Function for checking if the mS timer utilized by the scheduler 
 * is currently active.  The timer is always active on a PC Platform.
 *
 * @return    True if the scheduler's timer is currently active.
 */
static inline bool scheduler_tick_active() {
  return true;
}

 /**
 * @brief Function for getting the current value of the mS timer 
 * which is utilized by the scheduler.
 *
 * @return    The current timer value (mS).
 */
static inline uint32_t scheduler_get_tick() {
  struct timespec time;
  clock_gettime(CLOCK_REALTIME, &time);
  return (time.tv_sec*1000 + lround(time.tv_nsec/1e6)) & UINT32_MAX;
}

#endif // _SCHEDULER_CONFIG_H__
