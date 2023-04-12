#  Coding Style Guide

## Module Naming Conventions

All publicly accessible functions, macros and scheduler specific types should have "sched_" prepended to provide module naming scope.

Functions which take a scheduler task as a parameter should be have "sched_task_" prepended.

## Type Naming Conventions

| Scope               | Rule                        | Example                 |
| :----               | :----                       | :----                   |
| Macros              | Upper Case                  | SCHED_MS_MAX            |
| Functions           | Lower Case                  | sched_task_config()     |
| Pointer Parameters  | Lower Case, Prepend "p_"    | sched_task_t *p_task    |
| Custom Types        | Lower Case, Append "_t"     | sched_task_state_t      |
| Enum Values         | Upper Case                  | TASK_STATE_ACTIVE       |

## General Conventions

* Perfer static inline functions over function like macro's, where possible, for simpler debugging.

* Perfer enumerations and typedefs versus #defines for multi-valued constants.  A typedef provides better
self-documenting code than a #define even though older compliers may generate slightly more code for enumerations.

* Use static declaration for functions which are only used inside of a module.

* Include the units in any function names or function parameters which represent values with units.  I.E. sched_task_remaining_ms()

* All publicy accessible functions should have pointer parameters NULL checked and return a reasonable and documented value when the parameter is NULL.  For example, checking if NULL task has expired should return false since a NULL task can never expire.

* Perfer returning a simple bool to indicate function suceess rather than saddling the user with error codes.  It is understood that error codes are generally more useful for debugging but a bool was demmed to be adequate.  A scheduler function failure most often represents a a user level programming error for which the end user can simply assert.  For example, starting the scheduler without initializing it would return false and indicate a bug in the users program. 