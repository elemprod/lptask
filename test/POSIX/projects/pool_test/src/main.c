/**
 *  main.c
 *
 *  POSIX Task Pool Test
 *
 * The program tests the schedulers Task Pool Mechanism.
 *
 * Tasks are periodically allocated from the pool
 * Each task contains a randomized array of data and CRC
 * value for the data.
 *
 * The CRC is checked at each handler call to validate the stored
 * data.
 *
 * The buffer data is randomized and a new CRC calculated by the
 * handler periodically.
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

// Enable Debugging?
const bool DEBUG = false;

// Informational Logging - only prints if debug is true.
#define log_info(fmt, ...)              \
  do {                                  \
      if (DEBUG) {                      \
        printf(fmt, ## __VA_ARGS__);    \
        fflush(stdout);                 \
      }                                 \
  } while (0)


// Error Logging - Always print
#define log_error(fmt, ...)             \
  do {                                  \
    printf(fmt, ## __VA_ARGS__);        \
    fflush(stdout);                     \
  } while (0)


// The number of Buffered Tasks
#define TASK_COUNT 100

// Test Result
bool test_pass = true;

/* Function for setting the test pass results.  
 * Test fails are sticky, once set to false it stays false.
 */
static void test_pass_set(bool pass) {
  if(test_pass == true) {
    test_pass = pass;
  }
}

// Forward Declaration
static sched_task_t *pool_task_alloc();

// Define a pool of buffered task with storage space for the user data structure.
SCHED_TASK_POOL_DEF(task_pool, sizeof(buff_test_data_t), 100);

// Define a Pool Starter task to allocate and start all of the pool tasks.
SCHED_TASK_DEF(pool_starter_task);

/* Pool Test Task Handler
 *
 * Tests:
 *   Task access inside of the handler.
 *   Integrity of the data stored inside of the task.
 *   Generates new random data and updates the CRC.
 */
static void pool_task_handler(sched_task_t *p_task, void *p_data, uint8_t data_size) {

  assert(p_task != NULL);
  assert(p_data != NULL);

  // The supplied data length should be match the test data structure.
  assert(data_size == sizeof(buff_test_data_t));

  // The task should be in the executing or stopping state here.
  sched_task_state_t task_state = sched_task_state(p_task);
  assert((task_state == SCHED_TASK_EXECUTING) || (task_state == SCHED_TASK_STOPPING));

  // Test the task access inside of the handler.
  bool task_access_result = task_access_test(p_task);
  assert(task_access_result);

  // Cast the supplied pointer to the pool data structure.
  buff_test_data_t *p_buff_data = (buff_test_data_t *)p_data;

  // Increment the handler call counter.
  p_buff_data->handler_count++;

  // Stop the task once it's handler has been called 4 times.
  if(p_buff_data->handler_count >= 4) {
    sched_task_stop(p_task);

    /* Stop the scheduler after the starter task has been stopped and after 
     * all of the pool tasks have been stopped.
     */
    if( !sched_task_active(&pool_starter_task) && 
        (sched_pool_allocated(&task_pool) == 0)) {
        log_info("Test complete, stopping the scheduler.\n");
        sched_stop();
      }

  } else {

#if 0
    // Corrupt the buffer to test the CRC check.
    p_buff_data->buff.data[0] = 0x43;
#endif

    // Perform a CRC check of the previously stored data.
    if (!buff_crc_check(p_buff_data)) { 
      p_buff_data->crc_fail_count++;
      log_error("CRC Fail Cnt: %i\n", p_buff_data->crc_fail_count);
      test_pass_set(false);
    }  

    // Fill the buffer with random data
    buff_randomize(p_buff_data);

    // Update the CRC value.
    buff_crc_calc(p_buff_data);

  }
}

/* Function for attempting to allocate a new buffered test task
 * from the task pool.
 *
 * return The allocated and configured test task or NULL if no
 *        more tasks are available from the pool.
 */
static sched_task_t *pool_task_alloc() {
  // Attempt to allocate a task.
  sched_task_t *p_task = sched_task_alloc(&task_pool);

  // Configure the task if one could be allocated.
  if (p_task != NULL)
  {
    // Test the task access for the unconfigured task.
    bool task_access_result = task_access_test(p_task);
    assert(task_access_result);

    // Generate a random task interval length.
    uint32_t interval = (rand() % 1000) + 50;

    // Configure the allocated task as a repeating task with the random interval.
    bool result = sched_task_config(p_task, pool_task_handler, interval, true);
    assert(result == true);

    // Test the task access for the configured task.
    task_access_result = task_access_test(p_task);
    assert(task_access_result);

    // Create a temporary buff test data structure to add to the the task.
    buff_test_data_t buff_data;
    buff_init(&buff_data);

    // Copy the buff data to the task.
    uint8_t bytes_copied = sched_task_data(p_task, &buff_data, sizeof(buff_data));

    // Check if all of the data was copied to the task
    assert(bytes_copied = sizeof(buff_data));

    // Test the task access for the configured task after the data is added.
    task_access_result = task_access_test(p_task);
    assert(task_access_result);
  }
  return p_task;
}

/* Task handler for allocating and starting pool tak.
 */
static void pool_starter_handler(sched_task_t *p_task, void *p_data, uint8_t data_size) {

  // Count of the number of pool tasks started.
  static uint32_t pool_tasks_started = 0;

  // Attempt to allocate a pool task
  sched_task_t *p_pool_task = pool_task_alloc();

  if (p_pool_task == NULL) {
    // No more pool task available.

    // Check if all of the tasks were allocated.
    if(pool_tasks_started < TASK_COUNT) {
      log_error("Only %u of %u pool tasks could be allocated.\n", pool_tasks_started, TASK_COUNT);
      test_pass_set(false);
    } else {
      log_info("Pool Task's Started: %u.\n", pool_tasks_started);
    }
    
    // Stop the starter task since all of the pool has been allocated.
    bool success = sched_task_stop(p_task);
    assert(success);
    log_info("The task pool has been fully allocated.\n");
  } else {
    // Start the newly allocated task
    bool success =  sched_task_start(p_pool_task);
    assert(success);
    pool_tasks_started ++;
  }

}

int main() {
  log_info("\n*** Pooled Task Buffer Test Start ***\n\n");

  // Initialize the Scheduler
  sched_init();

  // Seed the random number generator.
  srand(time(NULL));

  // Configure the pool starter task.
  bool success = sched_task_config(&pool_starter_task, pool_starter_handler, 10, true);
  assert(success);
  success = sched_task_start(&pool_starter_task);
  assert(success); 

  // Start the Scheduler.
  sched_start();

  if (test_pass) {
    log_info("Scheduler Pool Test: Pass\n");
    return 0;
  } else {
    log_info("Scheduler Pool Test: FAIL\n");
    return 1;
  }

}

// Optional Scheduler Port Init / Deinit (for debugging)
void sched_port_init(void)
{
  log_info("sched_port_init()\n");
}

void sched_port_deinit(void)
{
  log_info("sched_port_deinit()\n");
}
