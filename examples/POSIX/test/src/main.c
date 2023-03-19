//
//  main.c
//
//  POSIX Scheduler Test Program
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
 *    A one-shot task which uses a new random inveral time for every task
 *    handler call.
 *
 *  Second, Minute, Hour & Day Interval Task's
 *    Long running repeating tasks.
 *
 *  Stop Task
 *    Stops the scheduler.  The scheduler is restarted if it has ran for less
 *    than 7 days.
 *
 */


// Task Interval Definations - mS between task calls
#define SEC_INTERVAL_MS 1000
#define MIN_INTERVAL_MS (1000 * 60)
#define HOUR_INTERVAL_MS (1000 * 60 * 60)
#define DAY_INTERVAL_MS (1000 * 60 * 60 * 24)

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

// Task to test stopping and then restarting the scheduler.
SCHED_TASK_DEF(stop_task);

// Buffered Test Task.
SCHED_TASK_DEF_BUFF(buffered_task, sizeof(int64_t));

// Scheduler Task Pool
SCHED_TASK_POOL_DEF(task_pool, 16, 8);


// Count of the number of times stopped.  A pointer to this variable is
// passed with the stop handler.  It is incremented during the handler call.
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
static void rand_task_handler(void *p_data, uint8_t data_size) {
  
  // Update the task statistics
  task_time_update(&rand_task_time);

  // Generate a random interval between 10 mS and 10 seconds
  uint32_t interval = (rand() % 9990) + 10;
  
  // Update time stats with the new interval
  task_time_set_interval(&rand_task_time, interval);
  
  // Update the task's interval & restart the task.
  sched_task_update(&rand_task, interval);
}

static void sec_task_handler(void *p_data, uint8_t data_size) {
  
  // Update the task statistics
  task_time_update(&sec_task_time);
  
}
static void min_task_handler(void *p_data, uint8_t data_size) {
  
  // Update the task statistics
  task_time_update(&min_task_time);

  // Update the Console with an Log Messages Once per Minute
  fflush(stdout);
}

static void hour_task_handler(void *p_data, uint8_t data_size) {
  
  // Update the task statistics
  task_time_update(&hour_task_time);

  run_hours ++;
  
  time_t mytime = time(NULL);
  char * time_str = ctime(&mytime);
  time_str[strlen(time_str)-1] = '\0';
  
  printf("Run %d Hours, Time : %s\n", run_hours, time_str);
  log_task_stats();
}

static void day_task_handler(void *p_data, uint8_t data_size) {
  
  // Update the task statistics
  task_time_update(&day_task_time);
  
  run_days ++;
  
  time_t mytime = time(NULL);
  char * time_str = ctime(&mytime);
  time_str[strlen(time_str)-1] = '\0';
  
  printf("Run %d Days, Time : %s\n", run_days, time_str);
  
  // Stop the the test after 7 days.
  if(run_days >= 7) {
    sched_task_stop(&rand_task);
    sched_task_stop(&sec_task);
    sched_task_stop(&min_task);
    sched_task_stop(&hour_task);
    sched_task_stop(&day_task);
    sched_task_stop(&stop_task);
    sched_stop();
  }
}

static void stop_task_handler(void *p_data, uint8_t data_size) {
  
  assert(data_size = sizeof(uint32_t));

  // Cast the data pointer to a pointer to the stop count variable.
  uint32_t * p_stop_count = (uint32_t *) p_data;
  
  // Increment the stop count variable.
  (* p_stop_count) ++;

  printf("Scheduler Stop Count: %u\n", (* p_stop_count)); 

  // Stop the Scheduler - will be restarted.
  sched_stop();

}

// The buffered task handler adds random data to the task on each call.

static void buffered_task_handler(void *p_data, uint8_t data_size) {


}

// Function for configuring all of the test tasks
static void test_tasks_config() {
  
  // Configure the random interval task as non-repeating with an initial interval
  // of 1 second.
  sched_task_config(&rand_task, rand_task_handler, SEC_INTERVAL_MS, false);

  // Configure the second's task to be called once per second
  sched_task_config(&sec_task, sec_task_handler, SEC_INTERVAL_MS, true);

  // Configure the minute task to be called once per minute
  sched_task_config(&min_task, min_task_handler, MIN_INTERVAL_MS, true);
  
  // Configure the hour task to be called once per hour
  sched_task_config(&hour_task, hour_task_handler, HOUR_INTERVAL_MS, true);

  // Configure the day task to be called once per day
  sched_task_config(&day_task, day_task_handler, DAY_INTERVAL_MS, true);
 
  // Configure the stop task with a random interval between 1 and 200 minutes
  uint32_t interval = (rand() % (MIN_INTERVAL_MS * 199)) + MIN_INTERVAL_MS;
  sched_task_config(&stop_task, stop_task_handler, interval, false);
  /* A pointer to the stop count is added to the unbuffered task to 
   * test the pointer passsing functionality.
   */
  uint8_t bytes_added = sched_task_data(&stop_task, &stop_count, sizeof(stop_count));
  assert(bytes_added == sizeof(stop_count));
}

// Function for starting all of the task except the stop task.
static void test_tasks_start() {
  sched_task_start(&rand_task);
  sched_task_start(&sec_task);
  sched_task_start(&min_task);
  sched_task_start(&hour_task);
  sched_task_start(&day_task);
}

// Function for restarting the scheduler and all tasks after it
// has been stopped.
static void scheduler_restart() {
  
  // Initialize the scheduler.
  sched_init();
  
  // Configure and Start all of the tasks except for the stop task.
  // The stop task is only runs one time.
  test_tasks_config();
  test_tasks_start();
  
  // Update task interval tracking stats if stopped and restarted to 
  // avoid introducing error into the into the interval stats.
  task_time_set_interval(&rand_task_time, SEC_INTERVAL_MS);
  task_time_set_interval(&sec_task_time, SEC_INTERVAL_MS);
  task_time_set_interval(&min_task_time, MIN_INTERVAL_MS);
  task_time_set_interval(&hour_task_time, HOUR_INTERVAL_MS);
  task_time_set_interval(&day_task_time, DAY_INTERVAL_MS);

  // Start the Scheduler.
  sched_start();
}

int main()
{
  printf("\n*** Scheduler Test Started ***\n\n");
  printf("Platform Pointer Size: %lu bits.\n", sizeof(uint8_t *) * 8);
  printf("Scheduler Task Size: %lu bytes.\n\n", sizeof(sched_task_t));
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

  // Start the stop task one time.
  sched_task_start(&stop_task);
  
  // Start the Scheduler.
  sched_start();

  // Restart the scheduler if the test is still running.
  while(run_days < 7) {
    printf("Scheduler Stopped - Restarting.\n");
    fflush(stdout);
    scheduler_restart();
  }
  
  // Test Complete
  printf("Scheduler Test Complete.\n");

  // Log the final task stats
  log_task_stats();
  fflush(stdout);
  return 0;

}

// Optional Scheduler Port Init / Deinit (for debugging)
void scheduler_port_init(void) {
  printf("scheduler_port_init()\n");
  fflush(stdout);
}

void scheduler_port_deinit(void) {
  printf("scheduler_port_deinit()\n");
  fflush(stdout);
}
