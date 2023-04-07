
# POSIX Test Project's

Each of project's are designed to be compiled and ran on a POSIX system (Linux, OSX, Raspberry Pi or Windows under Cygwin) and require the GCC toolchain to be installed.  The host system must have console access to monitor the results.  The projects meant to be used for testing the scheduler library since the similar functionality would already be built into any POSIX system. 

# Setup

Clone the CRC library from:

https://github.com/gityf/crc.git

to the POSIX folder:

/test/POSIX/external/crc

# Projects
## Task State Access Test
test/POSIX/projects/access_test/

The project tests the [Task State](../../docs/task_state.md) access protection mechanism implemented by the scheduler.  
The access protection is verified for each of the possible task states.

### Usage

Build the program by running make inside the project directory.

Run the test program:

./build/access_test

## Interval Test
test/POSIX/projects/interval_test/

 The project tests the interval accuracy of scheduled tasks.

  - Multiple repeating long running tasks are scheduled.
  - The minimum, average and maximum interval time error is calculated for each task in its handler call.  The interval time error is the difference between the programmed and actual time interval between task handler calls.
  - Any timer roll over or other math errors in the scheduler would detectable by a large interval error.
  - A non-repeating task is included to test errors which might be introduced as a result of restarting the task inside the  handler.  The interval is set to a new random interval during each  handler call and the task restarted.
  
  ** TODO the max interval changed **

  - A task is included with an interval set to the maximum interval supported by the scheduler (6.2 days).
  - The test stops after ~7 days and the results are reported to the console.

### Build

Run the make command from a console inside the project directory to build the project.

### Usage

./build/interval_test

To run and save the results to a log file use:

./build/interval_test | tee log.txt

## Buffered Task Pool Test
test/POSIX/projects/pool_test/

The project tests creating pool of buffered tasks.
Passing data to the allocated tasks.
Task data integrity.

### Usage

Build the program by running make inside the project directory.

./build/pool_test

To save the results to a logging file use:

./build/pool_test | tee log.txt


