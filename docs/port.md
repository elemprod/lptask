# Mandatory Platform Port Functions

A handful of platform specific functions must be implemented in order to utilize the scheduler with a new processor type.   These functions are all declared in the `sched_port.h` header.  Several examples implementations are provided in the project's 
`.\examples\` folder which can serve as starting point for porting to a new platform.

## Que Lock Functions

`void sched_port_lock(void)`
`void sched_port_free(void)`

These function are used for acquiring exclusive access to the scheduler's linked list task que.  The function is called by the scheduler whenever it needs to modify the task que.  The lock prevents different sections of code from modifying the task que pointers at the same time which could lead to corruption of the que.  One example of this would be a higher priority interrupt preempting a lower priority interrupt with both attempting to add tasks. 

Exclusive access can typically be achieved by either temporarily disabling global interrupts on simpler platforms or by utilizing a mutex lock.  The `sched_port_que_unlock()` function will always be called after the `sched_port_lock()` function once the que modifications have completed.

## Millisecond Timer Function

`uint32_t sched_port_ms(void)`

The scheduler requires a relatively accurate mS timer to accurately time tasks. The `sched_port_ms()` returns the current value of the timer in units of mS. The timer must be monatomic, it must increment one time for each mS of real time after initialization with no discontinuities or jumps.  It is expected to roll back to 0 after the UINT32_MAX value.  Once the scheduler has been started, the counter should only be modified by to the normal mS increment behavior.  More details on how the scheduler handles the mS timer and task expiration can be found in the [Tick Timer](tick_timer.md) section.

Certain sleep strategies may stop the counter during the platform sleep function call.  This is acceptable provided the timer is corrected for the actual duration it was asleep and restarted before the sleep timer returns.  See the sleep function section for a more detailed explanation.

# Optional Platform Port Functions

Several additional functions can be implemented by the user to further customize scheduler operation and handle an platform specific requirements.  These functions are defined as weak inside of the scheduler module and therefore do not require user definition if they are not utilized.

## Port Initializer Function

`void sched_port_init(void)`

The function is called by the scheduler module during startup to initialize any platform specific resources required by the scheduler.  The Millisecond Timer would typically be started here if not previously started during boot.

## Port Deinitializer Function
`void sched_port_deinit(void)`

The function is called by the scheduler during its stop sequence to perform in any platform specific deinitialization and tear down.  Any resources initialized by 'sched_port_init()' should be deinitialized here.  

## Port Sleep Function

`void sched_port_sleep(uint32_t interval_ms)`

The sleep function can be used to configure the platform for its lowest power consumption mode.  A variety of different sleep techniques can be implemented depending on the sleep hardware supported by the platform.   When porting to a new platform, it may be helpful to delay  implementation of the sleep function until the rest of the port has been completed since it is optional. 



Several simple sleep implementations are presented below but they not represent the lowest power option available.

* The simplest sleep implementation for sched_port_sleep() is no implementation since the function is optional.  If no user implementation is supplied, the scheduler will busy wait between tasks.  This implementation offers no power savings but this may be an entirely acceptable solution for initial testing or for systems which aren't concerned with power consumption.


### Fixed Duration Sleep

<img src="./img/port_sleep_fixed.svg" align="right" hspace="15" vspace="15" alt="Fixed Sleep Time"> 

A simple sleep implementation is too always sleep for a short fixed duration regardless of the interval supplied at function call. For example, the sleep function could always sleep for 1 mS before returning.  The fixed sleep duration could be increased to 5 or 10 mS if the application can tolerate a larger jitter in the task interval execution.  This may not be the lowest power solution for a application which uses longer task intervals but it is simple.

<br clear="right"/>

```
void sched_port_sleep(uint32_t interval_ms) {
    sleep_1ms();       // Sleep for 1 mS
}
```

### Systick Timer Sleep Wake

For a platform's which are configured to generate interrupt's every 1 ms,  in order to increment the Systick timer, the sleep function may be able to utilize a WFI (Wait for Interrupt) instruction.   This would be a typical solution for an ARM processor. 

TODO diagram showing WFI implementation.   Show the mS timer to the side.

```
void sched_port_sleep(uint32_t interval_ms) {
    WFI();       // Sleep until the next interrupt occurs
}
```

### Complex Sleep Implementations

In many cases, power consumption can be significantly reduced by implementing a more complex sleep technique which utilize the the platform's sleep hardware.  All sleep implantation's must take care to meet the following requirements:

1. The platform must wake itself and return from the sched_port_sleep()function call at or before the supplied interval.  Since the scheduler cache's the next expiring task, there is minimal overhead introduced by returning early.  This could be the case for a system with an 8-bit sleep timer as an example.  If the requested interval exceeds the maximum sleep interval supported by the platform's sleep timer, the platform can simply sleep for as long as it can and then return.

TODO - diagram showing sleeping for less than the requested interval.

2. The mS timer, as returned by the sched_port_ms() function, must be be valid on return from the function.  If the mS timer is stopped during the sleep interval, it should should be corrected and restarted before the sleep function returns.

TODO - diagram showing stopping and correcting and restarting timer.



<img src="./img/port_sleep_int.svg" align="right" hspace="15" vspace="15" alt="Sleep Interrupt"> 

3. Support for adding or modifying tasks from an interrupt context imposes an additional requirement. If the platform is woken from sleep due to an interrupt, the function must return once the interrupt has been serviced even if the sleep interval hasn't expired.   A new task may have been started inside of the ISR and it could expire before the previously commanded sleep interval does.  The sleep function needs to return to ensure that the new task is serviced.  This is the typical behavior for most platform with hardware timers which are configured to generate an interrupt after sleep interval expiration.  
<br clear="right"/>

  **Failing Sleep Example**

  The example sleep function fails to meet the requirement #3.  An interrupt inside of the while loop does not result in the sleep function returning in a reasonable time frame.

  ```
  void sched_port_sleep(uint32_t interval_ms) {
    uint32_t sleep_cnt;
    while(sleep_cnt < interval_ms) {
      sleep_5ms();       // Sleep for 5 mS
      sleep_cnt += 5;
    }
  }
  ```


