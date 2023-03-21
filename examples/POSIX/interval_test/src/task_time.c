//
//  task_time.c
//

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "task_time.h"

#define TIME_TASK_CLOCK CLOCK_MONOTONIC

// Calculate the time interval in seconds between a pair of time specs.
double diff_timespec(const struct timespec * p_time1, const struct timespec * p_time0) {
  return (p_time1->tv_sec - p_time0->tv_sec)
      + ((p_time1->tv_nsec - p_time0->tv_nsec) * 1e-9);
}

void task_time_init(task_time_t * p_task_time, uint32_t interval_ms) {
  assert(p_task_time != NULL);
  
  // Clear the structure
  memset(p_task_time, 0x00, sizeof(task_time_t));
  
  // Set the start time & last call time to now.
  clock_gettime(TIME_TASK_CLOCK, &p_task_time->time_start);
  clock_gettime(TIME_TASK_CLOCK, &p_task_time->time_last);
  
  // Store the programmed interval for later use
  p_task_time->interval = (double) interval_ms / 1000.0;
  
}

void task_time_update(task_time_t * p_task_time) {
  assert(p_task_time != NULL);
  
  // Get the current time.
  struct timespec time_now;
  clock_gettime(TIME_TASK_CLOCK, &time_now);
  
  //  Calculate the time interval since the last call.
  double interval_measured = diff_timespec(&time_now, &p_task_time->time_last);
  double interval_error = interval_measured - p_task_time->interval;
  
  // Calculate the running average of the interval.
  if(fabs(p_task_time->interval_avg) < 0.0001) {
    // Quick average the first interval error measurement.
    p_task_time->interval_avg = interval_error;
  } else {
    p_task_time->interval_avg = (p_task_time->interval_avg + interval_error) / 2;
  }
  
  // Conditionally store the min and max intervals
  if(interval_error > p_task_time->interval_max)
    p_task_time->interval_max = interval_error;
  
  if(interval_error < p_task_time->interval_min)
    p_task_time->interval_min = interval_error;
  
  // Set the last call time to now for next call use.
  memcpy(&p_task_time->time_last, &time_now, sizeof(time_now));
}

void task_time_set_interval(task_time_t * p_task_time, uint32_t interval_ms) {
  assert(p_task_time != NULL);
  
  // Set the last call time to now.
  clock_gettime(TIME_TASK_CLOCK, &p_task_time->time_last);
  
  // Store the updated interval.
  p_task_time->interval = (double) interval_ms / 1000.0;
  
}
                
void task_time_log(task_time_t * p_task_time) {
  assert(p_task_time != NULL);
  
  // Convert the error to mS.
  double avg_error = p_task_time->interval_avg * 1000.0;
  double min_error = p_task_time->interval_min * 1000.0;
  double max_error = p_task_time->interval_max * 1000.0;

  printf("Interval Error Min: %.1lf Avg: %.1lf Max: %.1lf (mS)\n", min_error, avg_error, max_error);
}
