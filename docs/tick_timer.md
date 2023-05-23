#  Scheduler Tick Timer

The Scheduler mS tick timer is implemented as a unsigned integer counter which 
rolls over to 0 once it it exceeds the UINT32_MAX value.   The tick counter's' 
operation is similar to the minute hand on a analog clock which simply goes 
back 0 after 59.  The difference is that tick counter has no hour hand 
equivalent so there's no method to detect roll overs.  

The modular arithmetic behavior of the counter must therefore be considered 
when calculating the interval until task expiration and also when checking 
whether a task has expired.  Fortunately it is straight forward to do so using 
the following formulas.
  
The counter ticks since the task was started can be calculated as:  
 
elapsed_ticks = (now_ticks - start_ticks)   
  
A task is considered to be expired if:

(now_ticks - start_tick) >= interval ticks  
                           
A couple of examples are presented below using an unsigned byte sized tick 
counter for brevity. A uint8_t follows the same modular arithmetic rules as 
a uint32_t.
             
| Example | Now Tick | Start Tick | Interval | Elapsed (now - start) | Expired?                |
|  :----: | :----:   |  :----:    |  :----:  | :----                 | :----                   |
| A       | 0xDC     | 0xC2       | 0x20     | 0xDC - 0xC2 = 0x1A    | Unexpired (0x1A < 0X20) | 
|         | 220      | 194        | 32       | 26                    | Unexpired (26 < 32)     | 
| B       | 0XF5     | 0xC2       | 0x20     | 0XF5 - 0xC2 = 0x33    | Expired (0x33 >= 0x20)  |
|         | 245      | 194        | 32       | 51                    | Expired (51 >= 32)      |
| C       | 0x04     | 0xC2       | 0x20     | 0X04 - 0xC2 = 0x42    | Expired (0x42 >= 0x20)  |
|         | 4        | 194        | 32       | 66                    | Expired (66 >= 32)      | 
     
To illustrate the necessity of these calculation techniques, consider using 
the naive expiration test of now_ticks >= start_ticks + interval_ticks.  Using 
the values from the Example C where the Now tick has rolled, one can see that 
the naive method fails.  The Now Tick value of 0x04 is less than the Start Tick 
of 0xC2 added Interval 0x20 which equals 0XE2, the task would be have been 
incorrectly detected as unexpired.  

The unfortunate consequence of the roll over behavior is that an expiration tick 
can't be precalculated when the event is started.  This would have allowed the 
scheduler to calculate and store an expiration tick once rather than have to 
perform the calculation at each expiration check.  This would have been a more 
efficient implementation.  The scheduler attempts to mitigate this inefficiency 
by storing a pointer to the next expiring task during each que execution.   
Provided that no additional tasks are added before the next expiration check, 
the cached next expiring task can be checked for expiration rather than checking 
every task for expiration.
