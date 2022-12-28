//
//  main.c
//  
//
//  Scheduler Module Test Program
//

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include "scheduler.h"

// Test Task 0
SCHED_EVT_DEF(test0_task);

// Test0 Task Scheduler Handler
static void test0_task_handler(void * p_context) {
  static long call_count = 0;
  call_count ++;
  if (call_count >= 8) {
    printf("Task 0 Complete\n");
    sched_evt_stop(&test0_task);
  } else {
    printf("Test Task 0, Call Cnt: %ld\n", call_count);
  }
}


int main()
{
  printf("Scheduler Test Start\n");
  
  // Initialize the Scheduler
  sched_init();

  // Configure the test task to be called every 10 seconds.
  sched_evt_config(&test0_task, test0_task_handler, NULL, 2000, true);
  sched_evt_start(&test0_task);

  while(true) {
    sched_evt_t * p_next_event = sched_execute();
    if(p_next_event != NULL) {
      // TODO calculate the time to sleep rather than sleeping at a fixed 1 second
      sleep(1);
    } else {
      printf("No Pending Scheduler Events - Program Complete.\n");
      fflush(stdout);
      return 0;
    }
    
  }
}
