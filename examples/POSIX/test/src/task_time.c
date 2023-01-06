//
//  task_time.c
//

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "task_time.h"


// Calculate the time interval in seconds between a pair of time specs.
double diff_timespec(const struct timespec *time1, const struct timespec *time0) {
  return (time1->tv_sec - time0->tv_sec)
      + (time1->tv_nsec - time0->tv_nsec) / 1000000000.0;
}

void task_time_init(uint32_t interval_ms, task_time_t * p_task_time) {
  
  // Clear the structure
  memset(p_task_time, 0x00, sizeof(task_time_t));
  
  // Set the start time & last call time to now.
  clock_gettime(CLOCK_REALTIME, &p_task_time->time_start);
  clock_gettime(CLOCK_REALTIME, &p_task_time->time_last);
  
  // Store the programmed interval for later use
  p_task_time->interval = interval_ms / 1000;
  
}

void task_time_update(task_time_t * p_task_time) {
  
  // Get the current time.
  struct timespec time_now;
  clock_gettime(CLOCK_REALTIME, &time_now);
  
  //  Calculate the time interval since the last call.
  double interval_current = diff_timespec(&time_now, &p_task_time->time_last);
  
  // Calculate the running average of the interval.
  if(fabs(p_task_time->interval_avg) < 0.0001) {
    // First interval measurement
    p_task_time->interval_avg = interval_current;
    p_task_time->interval_min = interval_current;
    p_task_time->interval_max = interval_current;
  } else {
    p_task_time->interval_avg = (p_task_time->interval_avg + interval_current) / 2;
  }
  
  // Conditionally store the min and max intervals
  if(interval_current > p_task_time->interval_max)
    p_task_time->interval_max = interval_current;
  
  if(interval_current < p_task_time->interval_min)
    p_task_time->interval_min = interval_current;
  
  // Set the last call time to now for future use.
  memcpy(&p_task_time->time_last, &time_now, sizeof(time_now));
}
                
void task_time_log(task_time_t * p_task_time) {
  
  // Calculate the Difference from the Programmed Interval
  double avg_error = (p_task_time->interval_avg - p_task_time->interval) * 1000;
  double min_error = (p_task_time->interval_min - p_task_time->interval) * 1000;
  double max_error = (p_task_time->interval_max - p_task_time->interval) * 1000;

  printf("Error Min: %.1lf Avg: %.1lf Max: %.1lf (mS)\n", min_error, avg_error, max_error);
}
