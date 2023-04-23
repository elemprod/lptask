# STM32L0 Example Project 

examples/STM32L0/power/

The project demonstrates 3 different sleep methods for the STM32L0XX processor. Each of the progressively more sophisticated sleep techniques offers additional power consumption improvements.  A single scheduler task is configured to toggle an LED 5 Hz.
    
### Sleep Mode Selection

One of the three supported modes can be selected with a #define SLEEP_METHOD.

SLEEP_NONE:  No sleep method is implemented. The processor simply busy waits during periods of inactivity.

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

The average processor current was measured for each sleep method using a 7-1/2 Digit Keithley DMM7510 DMM.  Note that the current measurement is for the processor only, the LED current was not included.  The task execution time interval and interval jitter (standard deviation interval) was also measured by probing the LED output with an oscilloscope. 

Utilizing the the Low Power Timer offers a 50 times reduction in power for this particular example!

### Project Setup

The STM32L0 Project requires that the STMicroelectronics supplied STM32CubeL0 SDK be cloned to the following folder:

/examples/STM32L0/SDK

The STM32CubeL0 SDK is available at:

https://github.com/STMicroelectronics/STM32CubeL0.git

### Build

The STM32LO example projects are provided for Segger Embedded Studio (SES).  Segger offers a free non-commercial license for SES. It should be relatively straight forward to setup the project with other compliers if desired.
