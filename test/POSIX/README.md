# Scheduler Testing

A series of projects have been created to test the scheduler library.  The test
can be ran individually or as a group using the `test_all.sh` script.  

Each of project's is designed to be compiled and ran on a POSIX system (Linux, 
OSX, Raspberry Pi or Windows under Cygwin).  The host must have the GCC 
toolchain installed and console access to monitor the results. 

The test projects are  meant to be used for testing the scheduler library 
since any POSIX system would already have similar functionality to the scheduler 
already included. 

The programs return 0 if all of the test's pass and or 1 or a system level 
error code if any of the tests fail so that the program results can be handled 
by a shell script.  

# Testing Setup

An external CRC library is setup as git submodule.  It should be pulled into the
project directory if it doesn't already exist.

Source: 
https://github.com/gityf/crc.git

Location:
/test/POSIX/external/crc


# Usage

Each of the tests can be ran as a batch with the `test_all.sh` script. 

The script builds and tests the scheduler for each of the supported [Build 
Configurations.](../../docs/build_config.md)

/test/POSIX/test_all.sh

# Test Projects

## Task State Access Test
test/POSIX/projects/access_test/

The project tests the [Task State](../../docs/task_state.md) access protection 
mechanism.  The access protection is verified for each of the possible task 
states.

## Buffered Task Pool Test
test/POSIX/projects/pool_test/

The project tests creating a pool of buffered tasks and passing data to the 
tasks.  

## Interval Math Test
test/POSIX/projects/interval_math/

The project performs unit testing the task interval math functions which are 
used for calculating the task time to expiration.  The program verifies that 
interval is correctly calculated for the possible timer roll over conditions.


## Context Test
test/POSIX/projects/context_test/

The project tests configuring and starting tasks from different contexts.  
Tasks are added from inside a signal handler to simulate task being accessed
from inside an embedded interrupt handler.   The signal is timer triggered at
random intervals.





## Interval Test
test/POSIX/projects/interval_test/

 The project tests the interval accuracy of scheduled tasks.

  - Multiple repeating long running tasks are scheduled.
  - The minimum, average and maximum interval time error is calculated for each task in its handler call.  The interval time error is the difference between the programmed and actual time interval between task handler calls.
  - Any timer roll over or other math errors in the scheduler would detectable by a large interval error.
  - A non-repeating task is included to test errors which might be introduced as a result of restarting the task inside the  handler.  The interval is set to a new random interval during each  handler call and the task restarted.
  - The test stops after ~7 days and the results are reported to the console.

