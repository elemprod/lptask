/**
 * @file task_access_tests.h
 * @author Ben Wirz
 * @brief Module for testing the scheduler task access control mechanism.
 *
 * @note A copy of the task is made and the checks are performed on the task
 * copy to avoid any risk of corrupting the original task.
 */

#ifndef TASK_ACCESS_TEST_H__
#define TASK_ACCESS_TEST_H__

#include <stdbool.h>
#include "scheduler.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Function for testing the task access control mechanism on
 * a task's based on its current state.
 *
 * @param[in] p_task  Pointer to the task to test.
 *
 * @return            True if the task's passed each of
 *                    the access control tests else false.
 */
bool task_access_test(const sched_task_t * const p_task);

#ifdef __cplusplus
}
#endif

#endif // TASK_ACCESS_TEST_H__