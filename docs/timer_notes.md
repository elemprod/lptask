#  Scheduler Tick Timer Notes

The Scheduler mS tick timer is implemented as a unsigned integer which rolls over to 0 once it it exceeds the UINT32_MAX value.   The tick counter's' operation is similar to the mintue hand on a analog clock which simply goes back 0 after 59.  The diference betweeen is that tick counter provides no hour hand so there's no method to detect roll overs.  

The modular arithmetic behaviour of the counter must therefore be considered when calcuating the number of ticks until task expiration and determining whether a task has expried.  Fortunately it is staright forward to do so using the following following formulas.
  
Calculate the Ticks since the Task was Started:  
 
elapsed_ticks = (now_ticks - start_ticks)   
  
Check if a Task has Expired:

(now_ticks - start_tick) >= interval ticks  
                           
A couple of examples are presented below using an unsigned byte sized tick counter for brevity.
             
| Example | Now    | Start | Interval | Elapsed           | Result                |
|         | Tick   | Tick  | Ticks    | Ticks             |                       |
| :----:  | :----: | :----:|   :----: |      :----:       | ----:                 |
| A:      | 0xDC   | 0xC2  | 0x20     | 0xDC - 0xC2 = 0x1A| Unexpired(0x1A < 0X20)| 
|         | 220    | 194   | 32       | 26                | Unexpired(26 < 32)    | 
| B:      | 0XF5   | 0xC2  | 0x20     | 0XF5 - 0xC2 = 0x33| Expired (0x33 >= 0x20)|
|         | 245    | 194   | 32       | 51                | Expired (51 >= 32)    |
| C:      | 0x04   | 0xC2  | 0x20     | 0X04 - 0xC2 = 0x42| Expired (0x42 >= 0x20)|
|         | 4      | 194   | 32       | 66                | Expired (66 >= 32)    | 
     
To illustrate why these calculation techniques are required, consider using the naive expiration test of now_ticks >= start_ticks + interval_ticks.  Using the values from Timer Rolled Example C, one can see that this method fails.  The start_tick of 0xC2 added interval_tick of 0x20 is 0xE2 which is greater than now_ticks value of 0x04.  The task would be have been incorrectly detected as unexpired.  

The unfortunate consequence of the roll over behvaiour is that an expiration tick can't be precalculated when a event is started.  This would have allowed us to calcuate and cache an expiration tick one time rather than have to perform the calcuation at each expiration check which would have been more effiecent.    
