# STM32L0 Example Projects

Three different STM32L0XX examples projects are provided.  Each project implements a progressively more sosphisicated power reduction technique.


STM32L0 Simple Example

./STM32L0/simple/

    - A Basic Scheduler example designed to illustrate the most simple scheduler implementation.
    - The example utilizes the most basic power reduction techniques of simply simply sleeping
    between systick interrupts while not excecuting scheduler tasks.

STM32L0 Low Power Example

./STM32L0/low_power/
    
    - Utilizes the Targets LPTIMER module to stop and wake the processor as needed to 
    processs scheduler events.
    - The examples toggles GPIO's at various rates using repeating tasks over a long duration.

STM32L0 Low Power Switch Debounce Example
   
./(TARGET)/switch/
    
    - Utilizes a one shot scheduler events to filter switch events.
    - Switch presses are debounced for a configurable time period.
    - Switch holds are also detected.
    - A switch event callback is made on switch status change detectionn.


# POSIX Example Project

./POSIX/test/

The test example project is designed to be compiled and ran on a POSIX system (Linux, OSX, Raspberry PI or Windows using Cygwin).  The project requires the GCC toolchain and Make.  The host system must have console access to monitor the test results.

The project is mainly useful for testing the scheduler library.  It offers the following features:

  - Multiple repeating long running tasks are scheduled.
  - The minimum, average and maximum interval time error is tracked for each task.  The interval time error is the difference between the programmed and actual time interval between task handler calls.  Roll over or other math errors in the scheduler would result in a large interval error.
  - A non-repeating task is included to test errors which might be introduced as a result of restarting the task inside the  handler.  The interval is set to a new random interval during each the handler call and the task restarted.
  - A task is included with an interval of set to the maximuim interval of 6.2 days required that the test by ran 7 days to complete.

Build:

Run make from a console inside the project directory to build the project.

Usage:

./build/sched_test

To save the results to a logging file use:

./build/sched_test | tee log.txt


