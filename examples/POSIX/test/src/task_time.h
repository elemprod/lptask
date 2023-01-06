//
//  task_time.h
//
// Module for tracking a Scheduler Tasks Call Time Statistics.
//

#ifndef TASK_TIME_H__
#define TASK_TIME_H__

#include <time.h>
#include <stdint.h>

/*
 * Scheduler Task Call Time Tracking Data structure
 */
typedef struct {
  struct timespec time_start;     // Time the task was started
  struct timespec time_last;      // Time the task was last called.
  
  double interval;                // Programmed interval (secs)
  double interval_avg;            // Running average of the interval (secs)
  double interval_min;            // Minimum interval measured (secs)
  double interval_max;            // Maximum interval measured (secs)
} task_time_t;

/*
 * Function for initializing a Task's Time Tracking Data structure
 */
void task_time_init(uint32_t interval_ms, task_time_t * p_task_time);

/*
 * Function for updating a Task's Time Tracking Data structure
 */
void task_time_update(task_time_t * p_task_time);

/*
 * Function for logging a Task's Time Tracking Data structure.
 */
void task_time_log(task_time_t * p_task_time);

#endif // TASK_TIME_H__

