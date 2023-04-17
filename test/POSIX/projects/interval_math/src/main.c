/**
  *  main.c
  *
  * @brief Interval Function Test
  * 
  * Simple test of the the task interval calculation functions.
  * 
  * 
  */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "scheduler.h"

/* Include the scheduler module source in order to get access to the
 * statically declared functions.  This is just done for testing purposes.
 */
#include "scheduler.c"

static void task_log(sched_task_t *p_task, uint32_t now_ms) {
  assert(p_task != NULL);

  printf("Task Start: %u mS, Interval: %u mS\n", p_task->start_ms, p_task->interval_ms);
  printf("Time Now: %u mS\n", now_ms);  

  uint32_t elapsed = task_time_elapsed_ms(p_task, now_ms);
  printf("Elapsed: %u mS\n", elapsed);  

  uint32_t remaining = task_time_remaining_ms(p_task, now_ms);

  printf("Remaining: %u mS\n", remaining); 

  if(task_time_expired(p_task, now_ms)) {
    printf("Expired\n");  
  } else {
    printf("Unexpired\n");     
  }
}

int main()
{
  printf("\n*** Interval Function Test ***\n\n");
  fflush(stdout);

  SCHED_TASK_DEF(test_task);

  printf("\n* Unexpired Task\n");
  test_task.interval_ms = 1000;
  test_task.start_ms = 10000;
  task_log(&test_task, 10100);
  
  printf("\n* Just Expired Task\n");
  task_log(&test_task, 11000);

  printf("\n* Very Expired Task\n");
  task_log(&test_task, 12000);

  printf("\n* Unexpired Task with Timer Roll.\n");
  test_task.interval_ms = 1000;
  // Start the task 100 mS before roll
  test_task.start_ms = UINT32_MAX - 100;
  // Now time set to 100 mS after roll
  // Elapsed should be 200 mS
  // Remaining should be 1000 - 100 - 100 = 800 mS
  task_log(&test_task, UINT32_MAX + 100);

  printf("\n* Expired Task with Timer Roll. \n");
  test_task.interval_ms = 100;
  // Start the task 2000 mS before roll
  test_task.start_ms = UINT32_MAX - 2000;
  // Now time set to 100 mS after roll
  // Elapsed should be 2100 mS
  // Remaining should be 0 mS
  task_log(&test_task, UINT32_MAX + 100);


  fflush(stdout);
  return 0;
}
