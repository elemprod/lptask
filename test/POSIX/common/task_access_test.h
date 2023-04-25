/**
 * task_access_tests.h
 *
 * Module for testing the access control mechanism for scheduler tasks.
 * Each of the access control restrictions is tested and the results are
 * compared against the defined behaviour.
 *
 * A copy of the task is made and the checks are performed on the taks
 * copyto avoid any risk of corrupting the orginal task.
 */

#ifndef TASK_ACCESS_TEST_H__
#define TASK_ACCESS_TEST_H__

#include <stdbool.h>
#include "scheduler.h"

/**
 * Function for testing the task access control mechanism on
 * a task's based on its current state.
 *
 * @param[in] p_task  Pointer to the task to test.
 *
 * @return            True if the task's passed each of
 *                    the access control tests else false.
 */
bool task_access_test(sched_task_t *p_task);

#endif // TASK_ACCESS_TEST_H__