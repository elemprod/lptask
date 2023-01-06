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
#include "task_time.h"

// Task Interval Definations - mS between task calls
#define SEC_INTERVAL_MS 1000
#define MIN_INTERVAL_MS (1000 * 60)
#define HOUR_INTERVAL_MS (1000 * 60 * 60)
#define DAY_INTERVAL_MS (1000 * 60 * 60 * 24)

/*
 * Random Interval Task
 * This task test updating the task interval inside of the handler.
 * The interval is set to a random each time the task expires.
 */
SCHED_TASK_DEF(rand_task);

// Task Called Once per Second
SCHED_TASK_DEF(sec_task);
task_time_t sec_task_time;

// Task Called Once per Minute
SCHED_TASK_DEF(min_task);
task_time_t min_task_time;

// Task Called Once per Hour
SCHED_TASK_DEF(hour_task);
task_time_t hour_task_time;

// Task Called Once per Day
SCHED_TASK_DEF(day_task);
task_time_t day_task_time;

// Function for logging all of the task time stats.
static void log_task_stats() {
  
  // Log the Task Time Stats
  printf("Second Task:  ");
  task_time_log(&sec_task_time);
  printf("Minute Task:  ");
  task_time_log(&min_task_time);
  printf("Hour Task:  ");
  task_time_log(&hour_task_time);
  printf("Day Task:  ");
  task_time_log(&day_task_time);
  fflush(stdout);
}

// Random Interval Task Schedule Handler.
static void rand_task_handler(void * p_context) {
  static uint32_t call_count = 0;
  call_count ++;
  
  // Generate a random interval between 50 mS and 10 seconds
  uint32_t interval = (rand() % 9950) + 50;
  
  // Update the interval and restart the task.
  sched_task_update(&rand_task, interval);
  sched_task_start(&rand_task);

  if((call_count > 0) && (call_count % 1000 == 0)) {
    printf("Random Task Executed %d times.\n", call_count);
    fflush(stdout);
  }
}

static void sec_task_handler(void * p_context) {
  
  // Update the task statistics
  task_time_update(&sec_task_time);
  
}

static void min_task_handler(void * p_context) {
  
  // Update the task statistics
  task_time_update(&min_task_time);
  
  // Number of Running Minutes
  static uint32_t run_mins = 0;
  
  time_t mytime = time(NULL);
  char * time_str = ctime(&mytime);
  time_str[strlen(time_str)-1] = '\0';
  
  if((run_mins > 0) && (run_mins % 15 == 0)) {
    printf("Running for %d Minutes, Time : %s\n", run_mins, time_str);
  }
  run_mins ++;
}

static void hour_task_handler(void * p_context) {
  
  // Update the task statistics
  task_time_update(&hour_task_time);
  
  // Number of Running Hours
  static uint32_t run_hours = 0;
  
  time_t mytime = time(NULL);
  char * time_str = ctime(&mytime);
  time_str[strlen(time_str)-1] = '\0';
  
  printf("Running for %d Hours, Time : %s\n", run_hours, time_str);
  run_hours ++;
  
  log_task_stats();
}

static void day_task_handler(void * p_context) {
  
  // Update the task statistics
  task_time_update(&day_task_time);
  
  // Number of Running Hours
  static uint32_t run_days = 0;
  
  time_t mytime = time(NULL);
  char * time_str = ctime(&mytime);
  time_str[strlen(time_str)-1] = '\0';
  
  printf("Run %d Days, Time : %s\n", run_days, time_str);
  fflush(stdout);
  run_days ++;
  
  // Stop all tasks after 7 days
  if(run_days >= 7) {
    sched_task_stop(&rand_task);
    sched_task_stop(&sec_task);
    sched_task_stop(&min_task);
    sched_task_stop(&hour_task);
    sched_task_stop(&day_task);
  }
}

// Function for Sleeping
void sleep_ms(int milliseconds){

#if _POSIX_C_SOURCE >= 199309L
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
#else
    if (milliseconds >= 1000)
      sleep(milliseconds / 1000);
    usleep((milliseconds % 1000) * 1000);
#endif
}


int main()
{
  printf("*** Scheduler Test Started ***\n");
  fflush(stdout);
  
  // Initialize the Scheduler
  sched_init();

  // Configure the random interval task as non-repeating.
  sched_task_config(&rand_task, rand_task_handler, NULL, 4000, false);
  sched_task_start(&rand_task);

  // Seed the random number generator.
  srand(time(NULL));
  
  // Configure the second task to be called once per second
  sched_task_config(&sec_task, sec_task_handler, NULL, SEC_INTERVAL_MS, true);
  sched_task_start(&sec_task);

  // Configure the task to be called once per minute
  sched_task_config(&min_task, min_task_handler, NULL, MIN_INTERVAL_MS, true);
  sched_task_start(&min_task);
  
  // Configure the task to be called once per hour
  sched_task_config(&hour_task, hour_task_handler, NULL, HOUR_INTERVAL_MS, true);
  sched_task_start(&hour_task);

  // Configure the task to be called once per day
  sched_task_config(&day_task, day_task_handler, NULL, DAY_INTERVAL_MS, true);
  sched_task_start(&day_task);
  
  // Initialize the Task Time Structures for Tracking the Task Call Time Statistics
  task_time_init(SEC_INTERVAL_MS, &sec_task_time);
  task_time_init(MIN_INTERVAL_MS, &min_task_time);
  task_time_init(HOUR_INTERVAL_MS, &hour_task_time);
  task_time_init(DAY_INTERVAL_MS, &day_task_time);
  
  while(true) {
    // Execute the scheduler Que
    sched_task_t * p_next_task = sched_execute();
    // Check if there are any tasks left in the que.
    if(p_next_task != NULL) {
      // Sleep until the next task expires.
      sleep_ms(sched_task_remaining_ms(p_next_task));
    } else {
      printf("No Events in Que - Scheduler Test Complete.\n");
      // Log the Final Task Stats
      log_task_stats();
      fflush(stdout);
      return 0;
    }
  }
}
