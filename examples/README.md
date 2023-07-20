# STM32L0 Example Project 

examples/STM32L0/power/

The project demonstrates 3 different sleep methods for the STM32L0XX 
processor. Each of the progressively more sophisticated techniques 
offers additional power consumption improvements.  

The project has a single scheduler task which is configured to toggle an LED 
at 5 Hz.  Although the simple project doesn't represent a typical real world
application, it does serve to demonstrate the potential power savings the
more complex sleep techniques offer.
    
### Sleep Mode Selection

One of the three supported modes can be selected with a #define SLEEP_METHOD.

SLEEP_NONE:  No sleep method is implemented. The processor simply busy waits 
during periods of inactivity.

SLEEP_SYSTICK:  The processor is stopped between expired tasks using the WFI 
instruction.  The SYSTICK timer is configured to generate an interrupt every 
1 mS and is enabled during sleep. This results in the processor waking every 
1mS, checking if has expired tasks and then going back to sleep if not.   The 
next expiring task is cached by the scheduler preventing it from having to 
check every task in the scheduler's que for expiration.

SLEEP_LPTIMER: The processor is stopped between expiring task using the WFI 
instruction with the SYSTICK timer disabled.   The LPTIM (Low Power Timer) is 
used to wake the processor once the next task expires.  The LPTIM is configured 
to generate an interrupt at the next tasks expiration interval.    While this 
technique increases the task to task jitter, it offers a 60 times improvement
in power consumption over the SLEEP_SYSTICK method for this example.

### Hardware Test Setup

A PCB with the following hardware configuration was utilized for the test 
measurements.

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

The average processor current was measured for each sleep method using a 
7-1/2 Digit Keithley DMM7510.  Note that the current measurement is for the 
processor only, the LED current is not included.  The task execution time 
interval and interval jitter (standard deviation of the interval) were 
measured by probing the LED output with an oscilloscope. 



### Project Setup

The STM32L0 Project requires that the STMicroelectronics supplied 
STM32CubeL0 SDK be cloned to the following folder:

/examples/STM32L0/SDK

The STM32CubeL0 SDK is available at:

https://github.com/STMicroelectronics/STM32CubeL0.git

### Build

The STM32LO example projects are provided for Segger Embedded Studio 
(SES) software.  Segger offers a free non-commercial license for SES. It 
would be relatively straight forward to setup the project with other 
compliers if desired.
