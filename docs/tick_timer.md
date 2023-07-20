#  Scheduler Tick Timer

The Scheduler's mS tick timer is implemented as a 32-bit unsigned integer 
value which rolls over to 0 once it it exceeds the UINT32_MAX value.   The tick 
counter's operation is similar to the minute hand on a analog clock which goes 
back 0 after 59 minutes.  The difference is that tick counter has no hour hand 
equivalent so there's no inherent method to detect roll overs.  

The modular arithmetic behavior of the counter must therefore be considered 
when calculating the time interval until task expiration and also when checking 
if a task has expired.  Fortunately it is straight forward to do so using 
the following formulas.
  
The ticks elapsed since the task was started can be calculated as:  
 
elapsed_ticks = (now_ticks - start_ticks)   
  
A task is considered to be expired if:

(now_ticks - start_tick) >= interval ticks  
                           
The unfortunate consequence of the roll over behavior is that a tasks expiration
time can't be precalculated during event configuration.  This would have allowed 
the scheduler to store an expiration timer value rather than having to 
perform the expiration calculation at each check. This potentially could have 
provided a more efficient implementation. 