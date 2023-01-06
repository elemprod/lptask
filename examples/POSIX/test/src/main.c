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


// Test Task 0
SCHED_TASK_DEF(task0);

// Task for Logging the Time every 60 Seconds
SCHED_TASK_DEF(sec_task);

// Task for Logging the Time Once per Minute
SCHED_TASK_DEF(min_task);

// Task for the Logging the Time Once per Hour
SCHED_TASK_DEF(hour_task);

// Task for the Logging the Time Once per Day
SCHED_TASK_DEF(day_task);

// Test0 Task Scheduler Handler
static void task0_handler(void * p_context) {
  static uint32_t call_count = 0;
  call_count ++;
  if (call_count >= 8) {
    printf("Task 0 Complete\n");
    sched_task_stop(&task0);
  } else {
    printf("Test Task 0, Call Cnt: %d\n", call_count);
    fflush(stdout);
  }
}

static void sec_task_handler(void * p_context) {
  
  // Number of Running Seconds
  static uint64_t run_secs = 0;
  
  time_t mytime = time(NULL);
  char * time_str = ctime(&mytime);
  time_str[strlen(time_str)-1] = '\0';
  
  // Print the seconds log every minute
  if(run_secs % (60) == 0) {
    printf("Run %llu Seconds, Time : %s\n", run_secs, time_str);
    fflush(stdout);
  }
  run_secs ++;
}

static void min_task_handler(void * p_context) {
  
  // Number of Running Minutes
  static uint32_t run_mins = 0;
  
  time_t mytime = time(NULL);
  char * time_str = ctime(&mytime);
  time_str[strlen(time_str)-1] = '\0';
  

  printf("Run %d Minutes, Time : %s\n", run_mins, time_str);
  run_mins ++;
}

static void hour_task_handler(void * p_context) {
  
  // Number of Running Hours
  static uint32_t run_hours = 0;
  
  time_t mytime = time(NULL);
  char * time_str = ctime(&mytime);
  time_str[strlen(time_str)-1] = '\0';
  
  printf("Run %d Hours, Time : %s\n", run_hours, time_str);
  fflush(stdout);
  run_hours ++;
  
}

static void day_task_handler(void * p_context) {
  
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
    sched_task_stop(&sec_task);
    sched_task_stop(&min_task);
    sched_task_stop(&hour_task);
    sched_task_stop(&day_task);
  }
}

// Sleep Function
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
  printf("Scheduler Test Start\n");
  
  // Initialize the Scheduler
  sched_init();

  // Configure the test task but don't start it.
  sched_task_config(&task0, task0_handler, NULL, 2000, true);
  //sched_task_start(&task0);

  // Configure the second task to be called once per second
  sched_task_config(&sec_task, sec_task_handler, NULL, 1000, true);
  sched_task_start(&sec_task);

  // Configure the task to be called once per minute
  sched_task_config(&min_task, min_task_handler, NULL, (1000 * 60), true);
  sched_task_start(&min_task);
  
  // Configure the task to be called once per hour
  sched_task_config(&hour_task, hour_task_handler, NULL, (1000 * 60 * 60), true);
  sched_task_start(&hour_task);

  // Configure the task to be called once per day
  sched_task_config(&day_task, day_task_handler, NULL, (1000 * 60 * 60 * 24), true);
  sched_task_start(&day_task);
  
  // Call the handler functions to print the initial times.
  sec_task_handler(NULL);
  min_task_handler(NULL);
  hour_task_handler(NULL);
  day_task_handler(NULL);
  
  while(true) {
    // Execute the scheduler Que
    sched_task_t * p_next_task = sched_execute();
    // Check if there are any tasks left in the que.
    if(p_next_task != NULL) {
      // Sleep until the next task expires.
      sleep_ms(sched_task_remaining_ms(p_next_task));
    } else {
      printf("No Events in Que - Scheduler Test Complete.\n");
      fflush(stdout);
      return 0;
    }
  }
}
