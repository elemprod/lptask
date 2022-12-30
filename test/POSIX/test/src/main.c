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
SCHED_TASK_DEF(task0);

// Test0 Task Scheduler Handler
static void task0_handler(void * p_context) {
  static long call_count = 0;
  call_count ++;
  if (call_count >= 8) {
    printf("Task 0 Complete\n");
    sched_task_stop(&task0);
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
  sched_task_config(&task0, task0_handler, NULL, 2000, true);
  sched_task_start(&task0);

  while(true) {
    sched_task_t * p_next_task = sched_execute();
    if(p_next_task != NULL) {
      // TODO calculate the time to sleep rather than sleeping at a fixed 1 second
      sleep(1);
    } else {
      printf("No Pending Scheduler Events - Program Complete.\n");
      fflush(stdout);
      return 0;
    }
    
  }
}
