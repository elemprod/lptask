# Mandatory Platform Port Functions

A handful of platform specific functions must be implemented in order to utilize the scheduler with a new processor type.   These functions are all declared in the `scheduler_port.h` header.  Several examples implementations are provided in the project's 
`.\examples\` folder which can serve as starting point for porting to a new platform.

## Que Lock Functions

`void scheduler_port_que_lock(void)`
`void scheduler_port_que_free(void)`

These function are used for acquiring exclusive access to the scheduler's linked list task que.  The function is called by the scheduler whenever it needs to modify the task que.  The lock prevents different sections of code from modifying the task que pointers at the same time which could lead to corruption of the que.  One example of this would be a higher priority interrupt preempting a lower priority interrupt with both attempting to add tasks. 

Exclusive access can typically be achieved by either temporarily disabling global interrupts on simpler platforms or by utilizing a mutex lock.  The `scheduler_port_que_unlock()` function will always be called after the `scheduler_port_que_lock()` function once the que modifications have completed.

## Millisecond Timer Function

`uint32_t scheduler_port_ms(void)`

The scheduler requires a relatively accurate mS timer to accurately time tasks. The `scheduler_port_ms()` returns the current value of the timer in units of mS. The timer must be monatomic, it must increment one time for each mS of real time after initialization with no discontinuities or jumps.  It is expected to roll back to 0 after the UINT32_MAX value.  Once the scheduler has been started, the counter should only be modified by to the normal mS increment behavior.  More details on how the scheduler handles the mS timer and task expiration can be found in the [Tick Timer](tick_timer.md) section.

Certain sleep strategies may stop the counter during the platform sleep function call.  This is acceptable provided the timer is corrected for the actual duration it was asleep and restarted before the sleep timer returns.  See the sleep function section for a more detailed explanation.

# Optional Platform Port Functions

Several additional functions can be implemented by the user to further customize scheduler operation and handle an platform specific requirements.  These functions are defined as weak inside of the scheduler module and therefore do not require user definition if they are not utilized.

## Initializer Function

`void scheduler_port_init(void)`

The function is called by the scheduler module during startup to initialize any platform specific resources required by the scheduler.  The Millisecond Timer would typically be started here if not previously started during boot.

## Deinitializer Function
`void scheduler_port_deinit(void)`

The function is called by the scheduler during its stop sequence to perform in any platform specific deinitialization and tear down.  Any resources initialized by 'scheduler_port_init()' should be deinitialized here.  

## Sleep Function

`void scheduler_port_sleep(uint32_t interval_ms)`

The sleep function can be used to configure the platform for its lowest power consumption mode.  A variety of different sleep techniques can be implemented depending on the sleep hardware supported by the platform.   When porting to a new platform, it may be helpful to delay  implementation of the sleep function until the rest of the port has been completed since it is optional. 

### Simple Sleep Implementation

Several simple sleep implementations are presented below but they not represent the lowest power option available.

* The simplest sleep implementation for scheduler_port_sleep() is no implementation since the function is optional.  If no user implementation is supplied, the scheduler will busy wait between tasks.  This implementation offers no power savings but this may be an entirely acceptable solution for initial testing or for systems which aren't concerned with power consumption.

* For a platform which are configured to generate an interrupt every mS in order to increment a Systick mS timer, the sleep function can utilize a WFI (Wait for Interrupt) instruction if supported.   This would be a typical solution for an ARM processor.

### Complex Sleep Implementations

In many cases, power consumption can be significantly reduced by implementing a more complex sleep technique which utilize the the platform's sleep hardware.  All sleep implantation's must take care to meet the following requirements:

1. The platform must wake itself and return from the sleep function call at or before the supplied interval.  Since the scheduler cache's the next expiring task, there is minimal overhead introduced by returning early.  This could be the case for a system with an 8-bit sleep timer as an example.  If the requested interval exceeds the maximum sleep interval supported by the platform's sleep timer, the platform can simply sleep for as long as it can and then return.

2. The mS timer, as returned by the scheduler_port_ms() function, must be be valid on return from the function.  If the mS timer is stopped during the sleep interval, it should should be corrected and restarted before the sleep function returns.

3. Support for adding or modifying tasks from an interrupt context imposes an additional requirement.  If a task is started from within an ISR, the new task might have an interval which expires before the interval the sleep function is currently sleeping for.   In many cases, a newly started task will have a 0 mS interval.  This could be the case if the developer is using the scheduler to move work out of the ISR.   The sleep function must return within a reasonable time after an interrupt event or risk delaying the execution of the new task.  Addressing this requirement can be solved with either of the following methods.

  * If the platform is woken from sleep for a reason other than the sleep interval expiring, the function must return within a reasonable time even if the interval it is waiting on hasn't expired yet.  This will be the default behavior for most platform hardware timers which are configured to generate an interrupt after sleep interval expiration.  

  * Another option is to always sleep for a short duration regardless of the interval supplied at function call. For example, the sleep function could always sleep for 1 mS before returning.  The fixed sleep duration could be increased to 5 or 10 mS if the application can tolerate a larger potential jitter in the task interval execution.  

  **Failing Sleep Example**

  Below is an example sleep function implementations which fails to meet the requirement #3.  Th. An interrupt inside of the while loop does not cause the function to return in a reasonable time frame.

  ```
  void scheduler_port_sleep(uint32_t interval_ms) {
    uint32_t sleep_cnt;
    while(sleep_cnt < interval_ms) {
      sleep_5ms();       // Sleep for 5 mS
      sleep_cnt += 5;
    }
  }
  ```

  The sleep function could instead be implemented with a fixed delay with the understanding that this may introduce up to 5 mS of error in task execution interval time.

  ```
  void scheduler_port_sleep(uint32_t interval_ms) {
      sleep_5ms();       // Sleep for 5 mS
  }
  ```
