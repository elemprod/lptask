#  Coding Style Guide

## Library Target

Embedded processors can have a wide range of capabilities from the simple 
8-bit 1980's era PICmicro to a 4 GHz 32-core headless edge computer.  The 
LPTASK library targets low to medium power single core embedded 
processors, typically 32-bits with at least 8kB of ROM.  The ARM Cortex 
processor will be a very common target for the library but it can be used with 
any processor for which a C complier is available.  

We define the anticipated target here, at the beginning of the style guide, 
since the target has guided the project convention selections presented below 
despite our desire to keep the project as platform agnostic as possible.  The 
choice of a simple boolean return values is one example of this influence.

## Naming Conventions

The `Snake Case` naming convention is used with lower cases letters. Spaces are 
replaced with underscores "_."  Constant value definitions and macros use 
upper case letters and the same underscore separator. 

All publicly accessible functions, macros and scheduler specific type names 
have "sched_" prepended to provide module level naming scope.

Functions names which operate on a scheduler task have 
"sched_task_" prepended.

Platform specific functions should have "sched_port_" prepended to indicate that
they are specific to the particular processor or processor family and need to 
implemented by the end user.

## Type Naming Conventions

| Scope               | Naming Convention         | Example                 |
| :----               | :----                     | :----                   |
| Functions           | Lower Case                | sched_task_config()     |
| Pointers            | Lower Case, Prepend "p_"  | sched_task_t *p_task    |
| Custom Types        | Lower Case, Append "_t"   | sched_task_state_t      |
| Enum Values         | Upper Case                | SCHED_TASK_ACTIVE       |
| Macros              | Upper Case                | SCHED_MS_MAX            |

## General Conventions

* Static memory is exclusively used rather than dynamic memory to ensure 
predictable runtime program execution.  A technique rarely exists which enables
an embedded program to recover from `malloc()` returning less memory than was
requested at run-time.

* Where possible, prefer static inline functions versus function like 
macros.  Although this approach requires trusting the compiler to generate 
efficient code, it significantly improves the source code readability and 
provides for simpler debugging in most cases.

* Prefer enumerations with typedefs versus #defines for multi-valued 
constants.  When properly named, a typedef is self-documenting even though 
some compliers may generate slightly more code for an enumeration vs a #define.

* Include the units for function names or function parameters which represent 
values.  I.E. `sched_task_remaining_ms()`.

* Prefer returning a simple bool to indicate public function success rather than 
saddling the end user with error codes.  While it is understood that error codes 
can provide more detailed debugging information, a simple bool was deemed to be 
adequate for the small collection of potential error types.  A scheduler 
function failure most often represents a user level programming error.  For 
example, starting the scheduler without initializing it first represents a bug 
in the end users program which needs be fixed rather than an intermittent issue 
for which a logged error code might be useful.

* All publicly accessible functions should have their pointer parameters NULL 
and range checked and should return a reasonable and documented value if the 
parameter is invalid.  For example, a function which checks if a NULL task has 
expired should return false since a NULL task can never expire.

* All private functions should be statically declared and have their parameters 
NULL or range checked for cases which would result in undefined operation with 
the assert() macro.  This approach both guard against library level bugs and 
also documents the assumed parameter values.  The assert macro was chosen over a 
return value for private functions to improve efficiency.  It can optimized 
away for release builds.

* Bit fields should only be used for the purpose of packing short-length data 
and flags inside a structure to economize storage space.  Bit fields should not 
be used to access individual bits in larger data types since their layout is 
implementation defined.

* Prefer exact-width integer data types for consistent operation on different 
platforms.  For example, the `int32_t` type should be used versus the `int` 
type.

## Project Goals

Power reduction and ease of use are the highest priority goals which guide all 
library design and architecture decisions.  In general, power reduction is
achieved by performing the scheduler's work in the fewest possible instructions 
and putting the processor back to sleep as quickly as possible.  Any scheduler 
work is overhead, albeit necessary overhead, and does not directly contribute 
to the end users application.  Library developers carry the responsibility of 
minimizing this overhead.  In pursuit of these goals, we take the following 
approach:

* Utilize concise function and modules naming while still maintaining project 
level name space and readability.

* Prefer power optimizations over RAM or ROM optimizations within reason. 

* Provide hooks, in the form of platform specific port functions, to enable 
the end user to optimize the timer and sleep technique for their platform.  

* Make as many of the port functions optional as possible to reduce the effort 
required to implement the scheduler on a new platform.  This approach enables 
the end user to quickly experiment with partial port implementation but also 
optimize the port where needed.
