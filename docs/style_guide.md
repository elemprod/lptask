#  Coding Style Guide

## Module Naming Conventions

All publicly accessible functions, macros and scheduler specific type names should have "sched_" prepended which provides module level naming scope.

Functions names which take a scheduler task as a parameter should have "sched_task_" prepended.

## Type Naming Conventions

| Scope               | Naming Convention           | Example                 |
| :----               | :----                       | :----                   |
| Functions           | Lower Case                  | sched_task_config()     |
| Pointers            | Lower Case, Prepend "p_"    | sched_task_t *p_task    |
| Custom Types        | Lower Case, Append "_t"     | sched_task_state_t      |
| Enum Values         | Upper Case                  | SCHED_TASK_ACTIVE       |
| Macros              | Upper Case                  | SCHED_MS_MAX            |

## General Conventions

* Prefer static inline functions versus function like macros, where possible. This approach requires trusting the compiler to generate efficient code but significantly improves the readability and provides simpler debugging.

* Prefer enumerations with typedefs versus #defines for multi-valued constants.  When properly named, a typedef is self-documenting even though older compliers may generate slightly more code for an enumeration vs a #defines.

* Use static declaration for functions which are only meant to be used inside of a module.  This is standard C practice but easily forgetten.

* Include the units for function names or function parameters which represent values.  I.E. sched_task_remaining_ms()

* All publicly accessible functions should have pointer parameters NULL checked and should return a reasonable and documented value if the parameter is NULL.  For example, a function which checks if a NULL task has expired should return false since a NULL task can never expire.

* Prefer returning a simple bool to indicate function success rather than saddling the user with error codes.  While it is understood that error codes can provide more debugging information, a simple bool was deemed to be adequate in this case.  A scheduler function failure most often represents a user level programming error.  The end user should simply assert on the return value in most cases.  For example, starting the scheduler without initializing it first would return false.  This represents a bug in the users program which needs be fixed.  There's no value in returning an error code.

* Bit fields should only be used for the purpose of packing short-length data and flags inside a structure to economize storage space.  Bit fields should not be used to access individual bits in larger data types since the bit field layout is implementation defined.

* Specific length data types should be used for consistent operation on different platforms.  For example, the int32_t type should be used rather than the int type.