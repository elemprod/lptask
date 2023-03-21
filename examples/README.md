# STM32L0 Example Project 

examples/STM32L0/power/

The project demonstrates 3 different sleep methods for the STM32L0XX processor. Each of the progressively more sophisticated sleep technique offers additional power consumption improvements.  A single scheduler task is configured to toggle an LED 5 Hz.
    
### Sleep Mode Selection

One of the three supported modes can be selected with a #define SLEEP_METHOD.

SLEEP_NONE:  No sleep method is implemented. The processor simply busy waits while waiting for the next task to expire.

SLEEP_SYSTICK:  The processor is stopped between expired tasks using the WFI instruction.  The SYSTICK timer is configured to generate an interrupt every 1 mS and is enabled during sleep. The results in the processor waking every 1mS, checking if has expired tasks and then going back to sleep if not.   The next expiring task by the scheduler preventing it from having to check each task for expiration on wake.

SLEEP_LPTIMER: The processor is stopped between expiring task using the WFI instruction but the SYSTICK timer is disabled with this sleep mode.   The LPTIM (Low Power Timer) is instead used to wake the processor once the next task expires.  The LPTIM is configured to generate an interrupt at the next tasks expiration interval.   

### Hardware Test Setup

A PCB with the following hardware configuration was utilized for the test current measurements.

| Hardware        | Configuration                   |
| :----           | :----                           |
| Processor       | STM2L053C8                      |
| Run Mode Clock  | 4.194 MHz Medium Speed Internal |
| Stop Mode Clock | 32.768 kHz External Crystal     |
| Power           | 3.0V CR2032 Battery             |
| LED             | Port B, GPIO 8                  |

### Test Results

| Sleep Method   | Current | Interval | Jitter |
| :----          | ----:   | ----:    | ----:  |  
| SLEEP_NONE     | 715 uA  | 501.9 mS | 57 uS  |
| SLEEP_SYSTICK  | 146 uA  | 500.1 mS | 38 uS  | 
| SLEEP_LPTIMER  | 2.1 uA  | 500.7 mS | 209 uS |

The average processor current was measured for each sleep method using a 7-1/2 Digit Keithley DMM7510 DMM.  Note that the current measurement is for the processor only, the LED current is not included.  The task execution time interval and interval jitter (standard deviation interval) was also measured by probing the LED output with an oscilloscope. 

Utilizing the the Low Power Timer offers over a 50 times reduction in current for this particular example at the expense of an increase in interval jitter. 

### Project Setup

The STM32L0 Project requires that the STMicroelectronics supplied STM32CubeL0 SDK be cloned to the following folder:

/examples/STM32L0/SDK

The STM32CubeL0 SDK is available at:

https://github.com/STMicroelectronics/STM32CubeL0.git

### Build

The STM32LO example projects are provided for Segger Embedded Studio (SES).  Segger offers a free non-commercial license for SES. It should be relatively straight forward to setup the project with other compliers if desired.

# POSIX Example Project's

The project's are designed to be compiled and ran on a POSIX system (Linux, OSX, Raspberry Pi or Windows under Cygwin) and require the GCC toolchain to be installed.  The host system must have console access to monitor the results.  The projectsare mainly meant to be used for testing the scheduler library since any POSIX system would already have the functionality offered by the scheduler. 

## POSIX Interval Test
examples/POSIX/interval_test/

 The project tests the interval accuracy of scheduled tasks.

  - Multiple repeating long running tasks are scheduled.
  - The minimum, average and maximum interval time error is calculated for each task in its handler call.  The interval time error is the difference between the programmed and actual time interval between task handler calls.
  - Any T=timer roll over or other math errors in the scheduler would detectable by a large interval error.
  - A non-repeating task is included to test errors which might be introduced as a result of restarting the task inside the  handler.  The interval is set to a new random interval during each  handler call and the task restarted.
  - A task is included with an interval set to the maximum interval supported by the scheduler (6.2 days).
  - The test stops after 7 days and the results are reported to the console.

### Build

Run the make command from a console inside the project directory to build the project.

### Usage

./build/interval_test

To save the results to a logging file use:

./build/interval_test | tee log.txt


