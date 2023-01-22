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

The test project is designed to be compiled and ran on a POSIX system (Linux, OSX, Raspberry Pi or Windows under Cygwin).  The project requires the GCC toolchain.  The host system must have console access to monitor the test results.

The project is mainly meant to be used for testing the scheduler library since any POSIX system would already have the functionality offered by the scheduler.  The project has the follwing features: 

  - Multiple repeating long running tasks are scheduled.
  - The minimum, average and maximum interval time error is calculated for each task in its handler call.  The interval time error is the difference between the programmed and actual time interval between task handler calls.  Timer roll overs or other math errors in the scheduler would detectable by a large interval error.
  - A non-repeating task is included to test errors which might be introduced as a result of restarting the task inside the  handler.  The interval is set to a new random interval during each  handler call and the task restarted.
  - A task is included with an interval set to the maximuim interval supported by the scheduler (6.2 days).
  - The test stops after 7 days and the results are reported to the console.
  - A 2nd random interval task is included for testing stopping and starting the scheduler module.  It stops the scheduler in its handler.  The scheduler and all tasks are restarted once the scheduler stop has completed.

Build:

Run make from a console inside the project directory to build the project.

Usage:

./build/sched_test

To save the results to a logging file use:

./build/sched_test | tee log.txt


