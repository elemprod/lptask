# Example Projects

Simple Scheduler Example

./(TARGET)/simple/

    - A Basic Scheduler example designed to illustrate the mostsimple scheduler implementation.
    - The example does not utilize any power reduction techniques.
    - The example simply busy waits between executing scheduler tasks rather than sleeping.

Low Power Repeating Task Example

./(TARGET)/lp_repeating/
    
    - Utilizes the Targets LPTIMER module to stop and wake the processor as needed to 
    processs scheduler events.
    - The examples toggles GPIO's at various rates using repeating tasks over a long duration.
    
User Push Switch Press, Debounce and Hold Detection Example
   
./(TARGET)/switch/
    
    - Utilizes a one shot scheduler events to filter switch events.
    - Switch presses are debounced for a configurable time period.
    - Switch holds are also detected.
    - A switch event callback is made on switch status change detectionn.
