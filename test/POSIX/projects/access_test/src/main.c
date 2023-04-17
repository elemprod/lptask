/**
 *  main.c
 *
 * POSIX Task Access Control Test
 *
 * The program tests the scheduler's task access control protection.
 * The task is accessed in each of the possible task states to verify the
 * operation follows the access control specification.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>

#include "scheduler.h"
#include "buff_test_data.h"
#include "task_access_test.h"

/*
 * Access Control Test State
 */
typedef enum
{
  TEST_STATE_UNTESTED = 0, // The test has been performed yet.
  TEST_STATE_PASS,         // The test passed.
  TEST_STATE_FAIL,         // The test failed.
} test_state_t;

/*
 * Task data structure for storing the task access test results for
 * each of the task states and the number of handler calls.
 */
typedef struct
{
  test_state_t uninit;    // TASK_STATE_UNINIT
  test_state_t stopped;   // TASK_STATE_STOPPED
  test_state_t active;    // TASK_STATE_ACTIVE
  test_state_t executing; // TASK_STATE_EXECUTING
  test_state_t stopping;  // TASK_STATE_STOPPING
  uint32_t handler_cnt;   // Count of the number of handler calls.
} test_result_data_t;

// Define a pair of buffered test tasks.
SCHED_TASK_BUFF_DEF(test_task_a, UINT8_MAX);
SCHED_TASK_BUFF_DEF(test_task_b, UINT8_MAX);

// Task A Result Data.
test_result_data_t task_results_a = {
    .uninit = TEST_STATE_UNTESTED,
    .stopped = TEST_STATE_UNTESTED,
    .active = TEST_STATE_UNTESTED,
    .executing = TEST_STATE_UNTESTED,
    .stopping = TEST_STATE_UNTESTED,
    .handler_cnt = 0};

// Task B Result Data.
test_result_data_t task_results_b = {
    .uninit = TEST_STATE_UNTESTED,
    .stopped = TEST_STATE_UNTESTED,
    .active = TEST_STATE_UNTESTED,
    .executing = TEST_STATE_UNTESTED,
    .stopping = TEST_STATE_UNTESTED,
    .handler_cnt = 0};

/**
 * Function for combining the test results for each state into
 * a single result value.
 */
static test_state_t test_result_combined(test_result_data_t *p_result)
{

  assert(p_result != NULL);

  if (p_result->handler_cnt == 0)
  {
    return TEST_STATE_UNTESTED;
  }

  if ((p_result->uninit == TEST_STATE_UNTESTED) ||
      (p_result->stopped == TEST_STATE_UNTESTED) ||
      (p_result->active == TEST_STATE_UNTESTED) ||
      (p_result->executing == TEST_STATE_UNTESTED) ||
      (p_result->stopping == TEST_STATE_UNTESTED))
  {
    return TEST_STATE_UNTESTED;
  }

  if ((p_result->uninit == TEST_STATE_FAIL) ||
      (p_result->stopped == TEST_STATE_FAIL) ||
      (p_result->active == TEST_STATE_FAIL) ||
      (p_result->executing == TEST_STATE_FAIL) ||
      (p_result->stopping == TEST_STATE_FAIL))
  {
    return TEST_STATE_FAIL;
  }

  return TEST_STATE_PASS;
}

/**
 * Function for logging one test state.
 */
static void log_test_state(test_state_t state)
{

  if (state == TEST_STATE_UNTESTED)
  {
    printf("Untested\n");
  }
  else if (state == TEST_STATE_FAIL)
  {
    printf("Fail\n");
  }
  else if (state == TEST_STATE_PASS)
  {
    printf("Pass\n");
  }
  else
  {
    printf("ERROR:  Uknown State\n");
  }
}
/**
 * Function for logging a test result for each tested state.
 */
static void log_result(test_result_data_t *p_result)
{

  printf("State Uninit    : ");
  log_test_state(p_result->uninit);
  printf("State Stopped   : ");
  log_test_state(p_result->stopped);
  printf("State Active    : ");
  log_test_state(p_result->active);
  printf("State Executing : ");
  log_test_state(p_result->executing);
  printf("State Stopping  : ");
  log_test_state(p_result->stopping);
}

/**
 * Function for performming the task acess control test and saving the results
 * to the task data structure.
 */
static void task_test(sched_task_t *p_task, test_result_data_t *p_result_data)
{

  test_state_t test_state;

  // Test the task access control and save the test state.
  if (task_access_test(p_task))
  {
    test_state = TEST_STATE_PASS;
  }
  else
  {
    test_state = TEST_STATE_FAIL;
  }

  // Save the test results to the data structure.
  switch (p_task->state)
  {
  case TASK_STATE_UNINIT:
    if (p_result_data->uninit != TEST_STATE_FAIL)
    {
      p_result_data->uninit = test_state;
    }
    break;

  case TASK_STATE_STOPPED:
    if (p_result_data->stopped != TEST_STATE_FAIL)
    {
      p_result_data->stopped = test_state;
    }
    break;

  case TASK_STATE_ACTIVE:
    if (p_result_data->active != TEST_STATE_FAIL)
    {
      p_result_data->active = test_state;
    }
    break;

  case TASK_STATE_EXECUTING:
    if (p_result_data->executing != TEST_STATE_FAIL)
    {
      p_result_data->executing = test_state;
    }
    break;

  case TASK_STATE_STOPPING:
    if (p_result_data->stopping != TEST_STATE_FAIL)
    {
      p_result_data->stopping = test_state;
    }
    break;

  default:
    printf("ERROR: Unknown State");
  }
}

/*
 * Test Task Handler
 */
static void task_handler(sched_task_t *p_task, void *p_data, uint8_t data_size)
{

  assert(p_task != NULL);

  // The task should be in the executing or stopping state here.
  sched_task_state_t task_state = sched_task_state(p_task);
  assert((task_state == TASK_STATE_EXECUTING) || (task_state == TASK_STATE_STOPPING));

  // Test the task access control of both tasks inside of the handler.
  task_test(&test_task_a, &task_results_a);
  task_test(&test_task_a, &task_results_b);

  if (p_task == &test_task_a)
  {
    task_results_a.handler_cnt++;
    if (task_results_a.handler_cnt >= 8)
    {
      sched_task_stop(&test_task_a);
    }
  }
  else if (p_task == &test_task_b)
  {
    task_results_b.handler_cnt++;
    if (task_results_b.handler_cnt >= 8)
    {
      sched_task_stop(&test_task_b);
    }
  }
  else
  {
    printf("Unknown Task\n");
    fflush(stdout);
    return;
  }

  // Retest the task access control since the task may have been stopped.
  task_test(&test_task_a, &task_results_a);
  task_test(&test_task_a, &task_results_b);

  sched_task_state_t task_state_a = sched_task_state(&test_task_a);
  sched_task_state_t task_state_b = sched_task_state(&test_task_a);

  if ((task_state_a == TASK_STATE_STOPPED) || (task_state_a == TASK_STATE_STOPPING))
  {
    if ((task_state_b == TASK_STATE_STOPPED) || (task_state_b == TASK_STATE_STOPPING))
    {
      // Stop the scheduler if both tasks are stopping or stopped.
      sched_stop();
    }
  }
  fflush(stdout);
}

int main()
{
  printf("\n*** Scheduler Access Control Test Started ***\n\n");
  fflush(stdout);

  // Initialize the Scheduler
  sched_init();

  // Test the access control before configuring the tasks
  task_test(&test_task_a, &task_results_a);
  task_test(&test_task_a, &task_results_b);

  // Configure the tasks as repeating.
  bool config_result = sched_task_config(&test_task_a, task_handler, 100, true);
  assert(config_result == true);
  config_result = sched_task_config(&test_task_b, task_handler, 125, true);
  assert(config_result == true);

  // Test the access control after configuring the tasks
  task_test(&test_task_a, &task_results_a);
  task_test(&test_task_a, &task_results_b);

  // Store some data in the structure.
  uint8_t dummy_data[] = {0x00, 0x01, 0x02, 0x03};
  uint8_t bytes_stored = sched_task_data(&test_task_a, dummy_data, sizeof(dummy_data));
  assert(bytes_stored == sizeof(dummy_data));
  bytes_stored = sched_task_data(&test_task_b, dummy_data, sizeof(dummy_data));
  assert(bytes_stored == sizeof(dummy_data));

  // Test the access control again after setting the task data.
  task_test(&test_task_a, &task_results_a);
  task_test(&test_task_b, &task_results_b);

  // Start the tasks
  bool start_result = sched_task_start(&test_task_a);
  assert(start_result == true);
  start_result = sched_task_start(&test_task_b);
  assert(start_result == true);

  // Start the Scheduler.
  sched_start();

  // Test Complete
  printf("Scheduler Access Control Test Complete.\n");

  printf("\nTask A Results:\n");
  log_result(&task_results_a);

  printf("\nTask B Results:\n");
  log_result(&task_results_b);
  printf("\n");

  if ((test_result_combined(&task_results_a) == TEST_STATE_PASS) &
      (test_result_combined(&task_results_b) == TEST_STATE_PASS))
  {
    printf("** TEST PASS **\n\n");
  }
  else
  {
    printf("** TEST FAIL **\n\n");
  }
  fflush(stdout);
  return 0;
}