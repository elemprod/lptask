# Example Projects


Generic Test Example
./PC/test/

Designed to be compiled and ran on a PC which has the GCC toolchain installed.
The example is mainly used for testing the scheduler library since the small memory footprint benefits offered by the scheduler library are rarely required on a PC.
Requires printf (console) and time.h support.


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
