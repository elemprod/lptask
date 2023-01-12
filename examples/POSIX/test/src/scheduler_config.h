/* 
 * POSIX Platform Scheduler Configuration & Support Functions
 *
 */

#ifndef _SCHEDULER_CONFIG_H__
#define _SCHEDULER_CONFIG_H__

#include <time.h>
#include <assert.h>
#include <math.h>
#include <pthread.h>

// Scheduler mutex
static pthread_mutex_t sched_mutex =  PTHREAD_MUTEX_INITIALIZER;

 /**
  * @brief Macro for aquiring exclusive access to the scheduler's linked list
  * event que.  The macro is utillized by the scheduler when enterring a critical
  * region of code.
  */
#define SCHED_CRITICAL_REGION_ENTER()         \
  pthread_mutex_lock(&sched_mutex);

/**
  * @brief Macro for releasing exclusive access to the scheduler's linked list.
  */
#define SCHED_CRITICAL_REGION_EXIT()          \
  pthread_mutex_unlock(&sched_mutex);

 /**
 * @brief Function for getting the current value of the mS timer 
 * utilized by the scheduler.
 *
 * @return    The current timer value (mS).
 */
static inline uint32_t sched_get_ms() {
  struct timespec time;
  assert(clock_gettime(CLOCK_MONOTONIC, &time) == 0);
  int64_t time_ms = (time.tv_sec * 1000) + lround(time.tv_nsec/1e6);
  return (uint32_t) time_ms % UINT32_MAX;
}


static inline void sched_port_init() {
  // Empty
};
static inline void sched_port_deinit() {
  // Empty
};

#endif // _SCHEDULER_CONFIG_H__
