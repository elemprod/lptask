
# Scheduler Build Configuration

The scheduler supports several different build configurations which allows the 
end user to customize its operation and optimize it for their application.

## SCHED_MS_MAX

 `SCHED_MS_MAX` defines the maximum task interval length in mS.  The default 
 value of `UINT32_MAX` will be suitable for most applications but the end user 
 can define a lower value should they desire to limit interval length. 
 
## SCHED_TASK_BUFF_CLEAR_EN

Defining `SCHED_TASK_BUFF_CLEAR_EN` to be != 0 enables clearing task data 
buffers during task configuration for buffered tasks.  The entire buffer will 
have the value of 0x00 after the `sched_task_config()` function returns.  The 
default implementation is to not clear the buffers but the end user can 
override this behavior by defining `SCHED_TASK_BUFF_CLEAR_EN` to be 1 if 
needed.  Clearing large task data buffer can be costly and is unnecessary for 
most applications since the portion of the buffer which is utilized is 
overwritten when the task data is added with the `sched_task_data()` function.  
Clearing the buffer can be useful for debugging purposes and is therefore 
supported.  Note that the definition does not apply to unbuffered tasks.

## SCHED_TASK_POOL_EN

Scheduler task pools can optionally be disabled with the SCHED_TASK_POOL_EN
#define.  If `SCHED_TASK_POOL_EN` is set to be 0, scheduler task pools are  
disabled.   Task pools are enabled by default but the end user can disable them 
to save ROM space and to slightly improve scheduler performance  if they are 
not needed.

## SCHED_TASK_CACHE_EN

<img src="./img/task_loop_cache.svg" align="right" 
  hspace="10" vspace="0" alt="Task Service Loop"> 
The scheduler caches the next expiring task during the task service loop by 
default.  Saving the next expiring task enables the scheduler to quickly 
determine if it can sleep in some instances without having to test each task 
in the que for expiration.  If the cached task is unexpired, the scheduler can 
immediately put the processor back to sleep.  The task caching improves the 
overall efficiency of the scheduler in cases where the processor is routinely 
woken due an interrupt or other exception.  
<br>
<br clear="right"/>

<img src="./img/task_loop_no_cache.svg" align="right" 
  hspace="15" vspace="0" alt="Task Service Loop with Caching Disabled"> 

Task caching can be disabled by by defining SCHED_TASK_CACHE_EN to be 0.  

<br clear="right"/>

A flow chart representing the task search algorithm is presented below 
for completeness.  The same task search technique is utilized whether or not 
caching is enabled.
<center>
<img src="./img/task_search.svg" align="center" hspace="5" 
  vspace="10" alt="Task Search"> 
</center>