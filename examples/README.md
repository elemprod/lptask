# STM32L0 Example Project 

STM32L0 Power Example

examples/STM32L0/power/

The project demostrates 3 different sleep method for the STM32L0XX processor. The active sleep method can be selected with a #define.  Each of the progressively more sosphisicated sleep technique provide reduced power consumption.



    - A Basic Scheduler example designed to illustrate the most simple scheduler implementation.
    - An LED is repeatably toggled on and off.
    
Current Measurements:

STM2L053C8 Processor
Run Mode Clock : 4.194 MHz Medium Speed Internal Clock   
Stop Mode Clock: 32,768 Hz External Crystal Clock
Power: 3.0V CR2032 Lithuim Coin Cell Battery

Sleep Technique     Measured Current

SLEEP_NONE
SLEEP_SYSTICK    
SLEEP_LPTIMER

Setup:

The STM32L0 Project requires that the STMicroelectronics supplied STM32CubeL0 SDK be clonde to the following folder:

examples/STM32L0/SDK

The STM32CubeL0 SDK is available at:

https://github.com/STMicroelectronics/STM32CubeL0.git

Build:

The STM32LO example projects are provided for Segger Embedded Studio (SES).  Segger offers a free non-commerical license for SES. It should be relatively straight forward to setup the project with other compliers if desired.

# POSIX Example Project

examples/POSIX/test/

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


