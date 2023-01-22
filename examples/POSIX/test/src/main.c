//
//  main.c
//
//  Scheduler Module Test Program
//

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "scheduler.h"
#include "scheduler_port.h"
#include "task_time.h"


/**
 *  Test Tasks:
 *
 *  Random Interval Task
 *    A one-shot task which updates its interval new random value each time the task's
 *    handler is called.
 *
 *  Second, Minute, Hour & Day Interval Task's
 *    Long running repeating tasks.
 *
 *  Stop Task
 *    Stops the scheduler.
 *
 *
 */


// Task Interval Definations - mS between task calls
#define SEC_INTERVAL_MS 1000
#define MIN_INTERVAL_MS (1000 * 60)
#define HOUR_INTERVAL_MS (1000 * 60 * 60)
#define DAY_INTERVAL_MS (1000 * 60 * 60 * 24)

//
#define STOP_INTERVAL_MS (16232)

// Number of Hours & Days the Test has been Running for
static uint32_t run_hours = 0;
static uint32_t run_days = 0;

// Random interval task
SCHED_TASK_DEF(rand_task);
task_time_t rand_task_time;

// Task called once per second
SCHED_TASK_DEF(sec_task);
task_time_t sec_task_time;

// Task called once per minute
SCHED_TASK_DEF(min_task);
task_time_t min_task_time;

// Task called once per hour
SCHED_TASK_DEF(hour_task);
task_time_t hour_task_time;

// Task called once per day
SCHED_TASK_DEF(day_task);
task_time_t day_task_time;

// Task called to test stopping and then restarting the scheduler.
SCHED_TASK_DEF(stop_task);
// Count of the number of times stopped.  A pointer to this variable is
// passed with the stop handler which increments it during each call.
uint32_t stop_count = 0;

// Function for logging all of the task time stats.
static void log_task_stats() {
  
  printf("** Interval Report ***\n");
  // Log each of the Task Time Stats
  printf("Random Task ");
  task_time_log(&rand_task_time);
  printf("Seconds Task ");
  task_time_log(&sec_task_time);
  printf("Minutes Task ");
  task_time_log(&min_task_time);
  printf("Hours Task ");
  task_time_log(&hour_task_time);
  if(run_days > 0) {
    printf("Day Task ");
    task_time_log(&day_task_time);
  }
  fflush(stdout);
}

// Random Interval Task Schedule Handler.
static void rand_task_handler(void * p_context) {
  
#if 0
  // Store the Last Time the Handler was called.
  static uint32_t time_ms_last = 0;
  if(time_ms_last != 0) {
    printf("Interval %ld mS\n", (long) sched_get_ms() - time_ms_last);
  }
  time_ms_last = sched_get_ms();
#endif
  
  // Update the task statistics
  task_time_update(&rand_task_time);

  // Generate a random interval between 10 mS and 10 seconds
  uint32_t interval = (rand() % 9990) + 10;
  
  // Update time stats with the new interval
  task_time_set_interval(&rand_task_time, interval);
  
  // Update the task's interval & restart the task.
  sched_task_update(&rand_task, interval);

#if 0
  task_time_log(&rand_task_time);
  fflush(stdout);
#endif
}

static void sec_task_handler(void * p_context) {
  
  // Update the task statistics
  task_time_update(&sec_task_time);
  
}
static void min_task_handler(void * p_context) {
  
  // Update the task statistics
  task_time_update(&min_task_time);

#if 0
  // Number of Running Minutes
  static uint32_t run_mins = 0;
  run_mins ++;
  

  time_t mytime = time(NULL);
  char * time_str = ctime(&mytime);
  time_str[strlen(time_str)-1] = '\0';
  
  if((run_mins > 0) && (run_mins % 15 == 0)) {
    printf("Running for %d Minutes, Time : %s\n", run_mins, time_str);
  }
#endif
  
  // Update the Console Once per Minute
  fflush(stdout);
}

static void hour_task_handler(void * p_context) {
  
  // Update the task statistics
  task_time_update(&hour_task_time);

  run_hours ++;
  
  time_t mytime = time(NULL);
  char * time_str = ctime(&mytime);
  time_str[strlen(time_str)-1] = '\0';
  
  printf("Run %d Hours, Time : %s\n", run_hours, time_str);
  log_task_stats();
}

static void day_task_handler(void * p_context) {
  
  // Update the task statistics
  task_time_update(&day_task_time);
  
  run_days ++;
  
  time_t mytime = time(NULL);
  char * time_str = ctime(&mytime);
  time_str[strlen(time_str)-1] = '\0';
  
  printf("Run %d Days, Time : %s\n", run_days, time_str);
  
  // Stop all tasks after 7 days / exit the program.
  if(run_days >= 7) {
    sched_task_stop(&rand_task);
    sched_task_stop(&sec_task);
    sched_task_stop(&min_task);
    sched_task_stop(&hour_task);
    sched_task_stop(&day_task);
    sched_task_stop(&stop_task);
  }
}

static void stop_task_handler(void * p_context) {
  
  // Cast the context as a pointer to stop count variable.
  uint32_t * p_stop_count = (uint32_t *) p_context;
  
  // Increment the stop count variable.
  (* p_stop_count) ++;
  
  // Stop the Scheduler
  bool stopped = sched_stop();
  if(stopped) {
    printf("Scheduler Stopped, Count: %u\n", * p_stop_count);
  } else {
    printf("Scheduler Stop Qued, Count: %u\n", * p_stop_count);
  }
  fflush(stdout);
}

// Function for sleeping for a time interval in mS.
void sleep_ms(int milliseconds){
  struct timespec ts;
  ts.tv_sec = milliseconds / 1000;
  ts.tv_nsec = (milliseconds % 1000) * 1000000;
  nanosleep(&ts, NULL);
}

#define TEST_INTERVAL_MS 853
// Simple function for testing the scheduler sched_get_ms() funcition.
static void sched_get_ms_test() {
  
  // Store the Start time.
  uint32_t time_ms_start = scheduler_port_ms();
  // Sleep for the Test Time
  sleep_ms(TEST_INTERVAL_MS);
  // Get the End time
  uint32_t time_ms_end = scheduler_port_ms();
  // Calculate the interval slept and  the error.
  int32_t time_ms_elapsed = time_ms_end - time_ms_start;
  int32_t time_ms_error = time_ms_elapsed - TEST_INTERVAL_MS;
  printf("Start: %u mS, End: %u mS\n", time_ms_start, time_ms_end);
  printf("Interval: %d mS, Error: %d mS\n", time_ms_elapsed, time_ms_error);
  fflush(stdout);
}


// Function for configuring all of the test tasks
static void test_tasks_config() {
  
  // Configure the random interval task as non-repeating with an intial interval
  // of 1 second.
  sched_task_config(&rand_task, rand_task_handler, NULL, SEC_INTERVAL_MS, false);

  // Configure the second's task to be called once per second
  sched_task_config(&sec_task, sec_task_handler, NULL, SEC_INTERVAL_MS, true);

  // Configure the minute task to be called once per minute
  sched_task_config(&min_task, min_task_handler, NULL, MIN_INTERVAL_MS, true);
  
  // Configure the hour task to be called once per hour
  sched_task_config(&hour_task, hour_task_handler, NULL, HOUR_INTERVAL_MS, true);

  // Configure the day task to be called once per day
  sched_task_config(&day_task, day_task_handler, NULL, DAY_INTERVAL_MS, true);
 
  // Configure the stop task with a random interval between 1 and 200 minutes
  // Pass a pointer to the stop count.
  uint32_t interval = (rand() % (MIN_INTERVAL_MS * 199)) + MIN_INTERVAL_MS;
  sched_task_config(&stop_task, stop_task_handler, &stop_count, interval, false);
}

// Function for starting all of the test tasks
static void test_tasks_start() {
  sched_task_start(&rand_task);
  sched_task_start(&sec_task);
  sched_task_start(&min_task);
  sched_task_start(&hour_task);
  sched_task_start(&day_task);
  sched_task_start(&stop_task);
}

// Function for restarting the scheduler and all tasks after it
// has been stopped.
static void scheduler_restart() {
  
  // Initialize the scheduller.
  assert(sched_init());
  
  // Configure and Start the Tasks
  test_tasks_config();
  test_tasks_start();
  
  // Update task interval tracking stats.
  task_time_set_interval(&rand_task_time, SEC_INTERVAL_MS);
  task_time_set_interval(&sec_task_time, SEC_INTERVAL_MS);
  task_time_set_interval(&min_task_time, MIN_INTERVAL_MS);
  task_time_set_interval(&hour_task_time, HOUR_INTERVAL_MS);
  task_time_set_interval(&day_task_time, DAY_INTERVAL_MS);

}

int main()
{
  printf("*** Scheduler Test Start ***\n");
  fflush(stdout);
  
  // Initialize the Scheduler
  sched_init();
  
  // Seed the random number generator.
  srand(time(NULL));
  
  // Configure the tasks
  test_tasks_config();
  
  // Initialize the task's interval tracking structure for
  // each of the tasks.
  task_time_init(&rand_task_time, SEC_INTERVAL_MS);
  task_time_init(&sec_task_time, SEC_INTERVAL_MS);
  task_time_init(&min_task_time, MIN_INTERVAL_MS);
  task_time_init(&hour_task_time, HOUR_INTERVAL_MS);
  task_time_init(&day_task_time, DAY_INTERVAL_MS);
  
  // Start each of the test tasks
  test_tasks_start();

  //sched_get_ms_test();
  
  while(true) {
    // Execute the scheduler Que
    sched_task_t * p_next_task = sched_execute();
    // Check if there are any tasks left in the que.
    if(p_next_task != NULL) {
      // Sleep until the next task expires.
      sleep_ms(sched_task_remaining_ms(p_next_task));
    } else {
      // No more tasks in the scheduler que.
      if(run_days < 7) {
        // Restart the scheduler if the test is still running
        printf("Scheduler Stopped - Restarting.\n");
        fflush(stdout);
        scheduler_restart();
      } else {
        printf("Scheduler Test Complete.\n");
        // Log the final task stats
        log_task_stats();
        fflush(stdout);
        return 0;
      }
    }
  }
}

// Optional Scheduler Port Init / Deinit (debugging)
void scheduler_port_init(void) {
  printf("scheduler_port_init()\n");
  fflush(stdout);
}

void scheduler_port_deinit(void) {
  printf("scheduler_port_deinit()\n");
  fflush(stdout);
}
