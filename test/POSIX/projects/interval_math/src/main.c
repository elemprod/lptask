/**
  *  main.c
  *
  * @brief Interval Function Test
  * 
  * Simple test of the the task interval math functions.
  * Verifies that the unsigned integer calculations on the interval 
  *  handle timer roll overs correctly.
  *
  */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "scheduler.h"

// Enable Debugging?
const bool DEBUG = false;

// Informational Logging - Only print if debug is true.
#define log_info(fmt, ...)              \
  do {                                  \
      if (DEBUG) {                      \
        printf(fmt, ## __VA_ARGS__);  \
        fflush(stdout);                 \
      }                                 \
  } while (0)


// Error Logging - Always Print
#define log_error(fmt, ...)             \
  do {                                  \
    printf(fmt, ## __VA_ARGS__);      \
    fflush(stdout);                     \
  } while (0)


/* Include the scheduler module source in order to get access to the
 * statically declared functions.  (Just for Testing)
 */
#include "scheduler.c"

/* Function for checking the interval math functions.
 *
 * @param[in] p_task        Pointer to the task.
 * @param[in] now_ms        Current timer value.
 * @param[in] remaining_ms  Anticipated remaining.
 * @param[in] elapsed_ms    Anticipated elapsed. 
 * @param[in] expired       Anticipated expired.  
 */

static bool task_check(sched_task_t *p_task,
                       uint32_t now_ms,
                       uint32_t remaining_ms,
                       uint32_t elapsed_ms,
                       bool expired)
{
  bool test_result = true;
  uint32_t remaining_calc = task_time_remaining_ms(p_task, now_ms);
  uint32_t elapsed_calc = task_time_elapsed_ms(p_task, now_ms);
  bool expired_calc = task_time_expired(p_task, now_ms);

  log_info("Start: %u mS, Interval: %u mS\n", p_task->start_ms, p_task->interval_ms);
  log_info("Now: %u mS, Elapsed: %u mS, Remaining: %u mS\n", now_ms, elapsed_ms, remaining_ms);

  if (remaining_calc != remaining_ms) {
    test_result = false;
    log_error("Remaining: %u mS does not match check: %u mS.\n", remaining_calc, remaining_ms);
  }

  if (elapsed_calc != elapsed_ms) {
    test_result = false;
    log_error("Elapsed: %u mS does not match check: %u mS.\n", elapsed_calc, elapsed_ms);
  }

  if (expired_calc != expired) {
    test_result = false;
    log_error("Expired does not match check.\n");
  }
  return test_result;
}

// Unexpired Task, No Timer Roll
static bool unexpired_test() {
  SCHED_TASK_DEF(test_task);
  log_info("\n* Unexpired Task, No Timer Roll\n");
  test_task.interval_ms = 1000;
  test_task.start_ms = 10000;
  return task_check(&test_task, 10100, 900, 100, false);
}

// Just expired Task, No Timer Roll
static bool expired_0_test() {
  SCHED_TASK_DEF(test_task);
  log_info("\n* Just Expired Task, No Timer Roll\n");
  test_task.interval_ms = 1000;
  test_task.start_ms = 10000;
  return task_check(&test_task, 11000, 0, 1000, true);
}

// Very expired Task, No Timer Roll
static bool expired_1_test() {
  SCHED_TASK_DEF(test_task);
  log_info("\n* Very Expired Task, No Timer Roll\n");
  test_task.interval_ms = 1000;
  test_task.start_ms = 10000;
  return task_check(&test_task, 20000, 0, 10000, true);
}

// Unexpired Task with Timer Roll
static bool unexpired_roll_test() {
  SCHED_TASK_DEF(test_task);
  log_info("\n* Unexpired Task with Timer Roll.\n");
  test_task.interval_ms = 1000;
  // Start time = 100 mS before timer roll
  test_task.start_ms = UINT32_MAX - 100;
  // Now Time = 100 mS after roll.
  // Elapsed should be 200 mS, remaining should be 800 mS
  return task_check(&test_task, UINT32_MAX + 100, 800, 200, false);
}

// Expired Task with Timer Roll
static bool expired_roll_test() {
  SCHED_TASK_DEF(test_task);
  log_info("\n* Expired Task with Timer Roll.\n");
  test_task.interval_ms = 1000;
  // Start time = 20000 mS before timer roll
  test_task.start_ms = UINT32_MAX - 2000;
  // Now Time = 100 mS after roll.
  // The elapsed times should be 2100 mS, remaining time should be 0 mS
  return task_check(&test_task, UINT32_MAX + 100, 0, 2100, true);
}

int main() {
  log_info("\n*** Interval Math Function Test's ***\n\n");

  bool test_result = 
    unexpired_test() &&
    expired_0_test() &&
    expired_1_test() &&
    unexpired_roll_test() &&
    expired_roll_test();

  if (test_result) {
    log_info("\nInterval Math Test: Pass\n");
    return 0;
  } else {
    log_info("\nInterval Math Test: FAIL\n");
    return 1; 
  }
}
