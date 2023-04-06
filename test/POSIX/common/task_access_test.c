
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "scheduler.h"
#include "task_access_test.h"

/**
 * Data structure defining what functions are available 
 * for a particular task in a particular state. 
 */ 
typedef struct {
  bool    config;       // Can the task be configured?
  bool    start;        // Can be task be started?
  bool    stop;         // Can be task be stopped?
  bool    interval;     // Can the interval be updated?
  bool    data;         // Can data be added?  
} task_state_access_t;

// Access control definition for the TASK_STATE_UNINIT state.
const task_state_access_t TASK_UNINIT_ACCESS = {
    .config = true,
    .start = false,
    .stop = false,
    .interval = false,
    .data = false};

// Access control definition for the TASK_STATE_STOPPED state.
const task_state_access_t TASK_STOPPED_ACCESS = {
    .config = true,
    .start = true,
    .stop = true,
    .interval = true,
    .data = true};

// Access control definition for the TASK_STATE_ACTIVE state.
const task_state_access_t TASK_ACTIVE_ACCESS = {
    .config = true,
    .start = true,
    .stop = true,
    .interval = true,
    .data = false};

// Access control definition for the TASK_STATE_EXECUTING state.
const task_state_access_t TASK_EXECUTING_ACCESS = {
    .config = false,
    .start = true,
    .stop = true,
    .interval = true,
    .data = false};

// Access control definition for the TASK_STATE_STOPPING state.
const task_state_access_t TASK_STOPPING_ACCESS = {
    .config = false,
    .start = true,
    .stop = true,
    .interval = true,
    .data = false};

/**
 * Function for getting the access control structure for a task
 * based on it's state.
 *
 * @param[in] p_task  Pointer to the task
 * @return            Pointer to the access contorl structure definition.
 */
static const task_state_access_t * task_state_access(sched_task_t *p_task) {
  assert(p_task != NULL);

  switch(p_task->state) {

    case TASK_STATE_UNINIT :
      return &TASK_UNINIT_ACCESS;

    case TASK_STATE_STOPPED :
      return &TASK_STOPPED_ACCESS;

    case TASK_STATE_ACTIVE :
      return &TASK_ACTIVE_ACCESS;

    case TASK_STATE_EXECUTING :
      return &TASK_EXECUTING_ACCESS;

    case TASK_STATE_STOPPING :
      return &TASK_STOPPING_ACCESS;    

  }

  printf("Unknown State");
  return &TASK_UNINIT_ACCESS;
}

/**
 * Function for testing if data can be added to task.
 *
 * @param[in] p_task  Pointer to the task to test.
 *
 * @return            True if data could be added to the task else false.
 *
 */
static bool test_data_add(sched_task_t *p_task) {
  assert(p_task != NULL);

  // Create dummy data to add to the task.
  uint8_t dummy_data[] = {0xFF};

  // Attempt to add the data.
  uint8_t bytes_added = sched_task_data(p_task, dummy_data, sizeof(dummy_data));
  return bytes_added == sizeof(dummy_data);
}

// Task handler for testing purposes.
static void test_task_handler(sched_task_t *p_task, void *p_data, uint8_t data_size) {
  // Empty
}

/**
 * Function for creating a local copy of a task.
 *
 * @param[in] p_task  Pointer to the task to copy.
 *
 * @return            Pointer to the local copy of the task.
 *
 */
static sched_task_t * task_local_copy(sched_task_t *p_task) {

  // Create a local data structure and buffer.
  static sched_task_t task_copy;
  static uint8_t task_copy_buff[UINT8_MAX];

  // Copy the task and task data.
  memcpy(&task_copy, p_task, sizeof(sched_task_t));
  memcpy(task_copy_buff, p_task->p_data, p_task->buff_size);

  // Update local task data pointer to the local buffer.
  task_copy.p_data = task_copy_buff;

  return &task_copy;
}

bool task_access_test(sched_task_t *p_task) {
  // Did the task pass all of the access control tests?
  bool test_pass = true;

  if(p_task == NULL) {
    printf("Fail: Null Task");
    // Can't perfom the tests on a NULL task.
    return false;
  }

  // Get the access control structure based on the task's current state
  task_state_access_t * p_task_access = (task_state_access_t *) task_state_access(p_task);

  /* Create a local copy of the task & data structure to perform
   * the test.  Testing the copy rather than the orginal avoid's 
   * the risk of potentialluy corrupting the orginal task.
   */ 
  sched_task_t * p_task_copy = task_local_copy(p_task);

  /* Test if the task can be configured. Only perform the test on 
   * task's which have been initialized to avoid polluting the scheduler 
   * que with test tasks.
   */
  if(p_task_copy->state != TASK_STATE_UNINIT) {
    bool config_result = sched_task_config(p_task_copy, test_task_handler, 10, true);

    if(config_result != p_task_access->config) {
      printf("Fail: Configuration Test");
      test_pass = false;
    }
  }

  // Atempt to start the task.
  p_task_copy = task_local_copy(p_task);
  bool start_result = sched_task_start(p_task_copy);
  if(start_result != p_task_access->start) {
    printf("Fail: Start Test");
    test_pass = false;
  }

  // Atempt to stop the task.
  p_task_copy = task_local_copy(p_task);
  bool stop_result = sched_task_stop(p_task_copy);
  if(stop_result != p_task_access->stop) {
    printf("Fail: Stop Test");
    test_pass = false;
  }

  // Attempt to update the task interval.
  p_task_copy = task_local_copy(p_task);
  bool interval_result = sched_task_update(p_task_copy, 1000);
  if(interval_result != p_task_access->interval) {
    printf("Fail: Interval Update Test");
    test_pass = false;
  }

  // Attempt to add data to the task if it has a buffer.
  if(p_task_copy->buff_size != 0) {
    p_task_copy = task_local_copy(p_task);
    bool data_add_result = test_data_add(p_task_copy);
    if(data_add_result != p_task_access->data) {
      printf("Fail: Data Add Test");
      test_pass = false;
    }
  }

  return test_pass;
}

