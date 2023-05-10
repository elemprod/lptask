
/*
 * POSIX Platform Scheduler Configuration & Support Functions
 */

#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <math.h>
#include <pthread.h>
#include "sched_port.h"

// Scheduler Que Mutex
static pthread_mutex_t sched_mutex =  PTHREAD_MUTEX_INITIALIZER;

void sched_port_lock(void) {
  int ret = pthread_mutex_lock(&sched_mutex);
  assert(ret == 0);
}

void sched_port_free(void) {
  int ret = pthread_mutex_unlock(&sched_mutex);
  assert(ret == 0);
}

uint32_t sched_port_ms(void) {
  // Get the current time.
  struct timespec time;
  int ret = clock_gettime(CLOCK_MONOTONIC, &time);
  assert(ret == 0);
  // Convert the current time to mS
  int64_t time_ms = (time.tv_sec * 1000) + lround(time.tv_nsec/1e6);
  // Limit the returned value.
  return (uint32_t) time_ms % UINT32_MAX;
}

#if 0
/* Platform sleep function which sleeps for 1mS 
 * regardless of the supplied interval parameter.
 */
void sched_port_sleep(uint32_t interval_ms) {
  (void) interval_ms;
  const struct timespec ts = {.tv_sec = 0, tv_nsec = 1000000};
  nanosleep(&ts, NULL) = 0);
}
#else 
/* Platform sleep function which attempts to sleep
 * for the supplied interval.  Note that nanosleep() 
 * can be woken by any thread signal.
 */
void sched_port_sleep(uint32_t interval_ms) {
  struct timespec ts;
  ts.tv_sec = interval_ms / 1000;
  ts.tv_nsec = (interval_ms % 1000) * 1000000;
  nanosleep(&ts, NULL);
}
#endif
