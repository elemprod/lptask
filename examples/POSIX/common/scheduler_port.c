
/*
 * POSIX Platform Scheduler Configuration & Support Functions
 *
 */

#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <math.h>
#include <pthread.h>
#include "scheduler_port.h"

// Scheduler Que Mutex
static pthread_mutex_t sched_mutex =  PTHREAD_MUTEX_INITIALIZER;

void scheduler_port_que_lock(void) {
  assert(pthread_mutex_lock(&sched_mutex) == 0);
}

void scheduler_port_que_free(void) {
  assert(pthread_mutex_unlock(&sched_mutex) == 0);
}

uint32_t scheduler_port_ms(void) {
  // Create a timer from the current time.
  struct timespec time;
  assert(clock_gettime(CLOCK_MONOTONIC, &time) == 0);
  int64_t time_ms = (time.tv_sec * 1000) + lround(time.tv_nsec/1e6);
  return time_ms % UINT32_MAX;
}

