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

The test project is designed to be compiled and ran on a POSIX system (Linux, OSX or Windows using Cygwin) whihc has the GCC toolchain available.
The project is mainly meant to be used for testing of the scheduler library with long running tasks.  
A POSIX system typically wouldn't the need the small memory footprint benefits offered by the scheduler library.

Build:

Run make from a console inside the project directory to build the project.

Usage:

./build/sched_test

To save the results to logging file use:

./build/sched_test | tee log.txt

The test runs for 7 days before stopping.
