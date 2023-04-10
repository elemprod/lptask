#  Coding Style Guide


## Naming Conventions

All publicly accessible functions, macros and scheduler specific types should have "sched_" prepended.


| Scope               | Rule                        | Example         |
| :----               | :----                       |   :----        |
| Macros              | Upper Case                  | SCHED_MS_MAX    |
| Functions           | Lower Case                  | sched_task_config()    |
| Pointer Parameters  | Lower Case, Prepend "p_"    | sched_task_t *p_task    |
| Custom Types        | Lower Case, Append "_t"     | sched_task_state_t   |
| Enum Values         | Upper Case                  | TASK_STATE_ACTIVE    |

## General Conventions

* Perfer static inline functions over function like macro's where possible for easier debugging.

* Perfer enumerations with typedef versus #defines for multi-valued constants because they are better self-documenting even though older compliers may generate slightly more code.

* Use static declaration for functions which are only used inside of a module.

* Include the units in any function names which a value with units or function parameters which represent units.


