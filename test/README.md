# Test Project

POSIX Test Project

./POSIX/test/

The test project is designed to be compiled and ran on a POSIX system (Linux, OSX or Windows running Cygwin) which has the GCC toolchain installed and console support.
The project is mainly meant to be used for testing of the scheduler library with long running tasks.  
A POSIX system typically wouldn't the need the small memory footprint benefits offered by the scheduler library.

Build:

Run make from a console inside the project directory to build the project.

Usage:

./build/sched_test

To also save the results to logging file use:

./build/sched_test | tee log.txt

The test runs for 7 days.


