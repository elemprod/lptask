//
//  task_time.h
//
// Module for tracking a Scheduler Tasks Call Time Statistics.
//

#ifndef TASK_TIME_H__
#define TASK_TIME_H__

#include <time.h>
#include <stdint.h>
#include <assert.h>

/*
 * Scheduler Task Call Time Tracking Data structure
 */
typedef struct {
  struct timespec time_start;     // Time the task was started
  struct timespec time_last;      // Time the task was last called.
  
  double interval;                // Currently Programmed interval (secs)
  double interval_avg;            // Running average of the interval error (secs)
  double interval_min;            // Minimum measured interval error. (secs)
  double interval_max;            // Maximum measured interval error (secs)
} task_time_t;

/*
 * Function for initializing a Task's Time Tracking Data structure
 */
void task_time_init(task_time_t * p_task_time, uint32_t interval_ms);

/*
 * Function for storing a new interval for a previously initialized Task Time
 * Data structure.
 */
void task_time_set_interval(task_time_t * p_task_time, uint32_t interval_ms);

/*
 * Function for calculating a Task's Time structure statistics.
 * The function should be called at the start of the task's schedule
 * handler.
 */
void task_time_update(task_time_t * p_task_time);

/*
 * Function for logging a Task's Time Tracking Data structure.
 */
void task_time_log(task_time_t * p_task_time);

#endif // TASK_TIME_H__

