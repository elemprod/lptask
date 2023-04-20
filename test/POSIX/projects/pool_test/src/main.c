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

// The number of Buffered Tasks
#define TASK_COUNT 100

// Test Result
bool test_pass = true;

// Forward Delcaration
static sched_task_t *pool_task_alloc();

// Define a pool of buffered task with storage space for the user data structure.
SCHED_TASK_POOL_DEF(task_pool, sizeof(buff_test_data_t), 100);

/* Pool Test Task Handler
 *
 * Test's task access inside of the handler.
 * Test the integrity of the data stored inside of the task.
 * Generates new random data and updates the CRC.
 */
static void pool_task_handler(sched_task_t *p_task, void *p_data, uint8_t data_size)
{
  printf("Pool Task Handler\n");
  fflush(stdout);

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
  printf("Handler Call Cnt: %i\n", p_buff_data->handler_count);

  // Stop the task once the handler has been called 10 times.
  if(p_buff_data->handler_count >= 10) {
    sched_task_stop(p_task);
    return;
  }
  // CRC check of the previously stored data.
  if (!buff_crc_check(p_buff_data))
  { 
    p_buff_data->crc_fail_count++;
    printf("CRC Fail Cnt: %i\n", p_buff_data->crc_fail_count);
    fflush(stdout);
    test_pass = false;
  }

  // Fill the buffer with random data
  buff_randomize(p_buff_data);

  // Update the CRC value.
  buff_crc_calc(p_buff_data);

  fflush(stdout);
}

/* Function for attempting to allocate a new buffered test task
 * from the task pool.
 *
 * return The allocated and configured test task or NULL if no
 *        more tasks are available from the pool.
 */
static sched_task_t *pool_task_alloc()
{
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

    // Test the task access for the configured task after the data was added.
    task_access_result = task_access_test(p_task);
    assert(task_access_result);
  }
  return p_task;
}


int main()
{
  printf("\n*** Pooled Task Buffer Test Start ***\n\n");
  fflush(stdout);

  // Initialize the Scheduler
  sched_init();

  // Seed the random number generator.
  srand(time(NULL));

  sched_task_t *p_task = pool_task_alloc();
  if (p_task == NULL)
  {
    printf("Task Could Not Allocated.\n");
  }
  else
  {
    // Start the new task
    sched_task_start(p_task);
  }

  // Start the Scheduler.
  sched_start();

  // Test Complete
  printf("Scheduler Pool Test Complete.\n\n");

  if (test_pass)
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

// Optional Scheduler Port Init / Deinit (for debugging)
void sched_port_init(void)
{
  printf("sched_port_init()\n");
  fflush(stdout);
}

void sched_port_deinit(void)
{
  printf("sched_port_deinit()\n");
  fflush(stdout);
}


// TODO  Create a repeating task to allocate and start new tasks until the pool is fully allocated
//      Once the buffer is fully allocated, start another repeating tasks which checks if the buffer has no allocations and then stop the scheduler.