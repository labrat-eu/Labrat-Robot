@page time Time

@note
You may also want to take a look at the [synchronized time](@ref example_time_synchronized) and [stepped time](@ref example_time_stepped) code examples.

# Basics
There exist multiple feasable time sources to synchronize a robotics program.
In order to unify most of them, lbot provides the [lbot::Clock](@ref lbot::Clock) class.
The following time sources are supported by lbot:

| Name          | STL equivalent              | Description |
| ---           | ---                         | ---         |
| system        | [`std::chrono::system_clock`](https://en.cppreference.com/w/cpp/chrono/system_clock) | The clock of your system. This usually is the real world time. It may be adjusted at any time. |
| steady        | [`std::chrono::steady_clock`](https://en.cppreference.com/w/cpp/chrono/steady_clock) | A monotonic clock that cannot decrease as time moves forward. The time between ticks is constnt. However this clock is not related to the real world time. |
| synchronized  | -                           | A clock synchronized to an external time source. The source should roughly match the speed of the steady clock. |
| stepped       | -                           | An incremental user provided clock. This is the preferred option when working with a simulator as this clock source allows for arbitrary slowing, accelerating and stopping of time. |

By default the system time is used by lbot. In order to change the time source you need to specify the `/lbot/clock_mode` parameter accordingly. This needs to be done **before** instanciating the [central manager](@ref lbot::Manager).
```cpp
lbot::Config::Ptr config = lbot::Config::get();
config->setParameter("/lbot/clock_mode", "steady");
lbot::Manager::Ptr manager = lbot::Manager::get();
```

# Usage
The [lbot::Clock](@ref lbot::Clock) class meets the [STL Clock requirements](https://en.cppreference.com/w/cpp/named_req/Clock).
For many use cases like time casting or time formatting you can therefore use existing functions from the [STL chrono library](https://en.cppreference.com/w/cpp/chrono).

## Getting the current time
In order to get the current time you need to call the [Clock::now()](@ref lbot::Clock::now()) function.
```cpp
lbot::Clock::time_point time = lbot::Clock::now();
```

## Sleeping
You can pause execution of the current thread by calling the [Thread::sleepFor()](@ref lbot::Thread::sleepFor) and [Thread::sleepUntil()](@ref lbot::Thread::sleepUntil()) functions.
```cpp
lbot::Thread::sleepFor(std::chrono::seconds(1));
lbot::Thread::sleepUntil(lbot::Clock::now() + std::chrono::seconds(1));
```

## Waiting on a condition variable
You can wait on a [condition variable](@ref lbot::ConditionVariable) by calling the [waitFor()](@ref lbot::ConditionVariable::waitFor()) and [waitUntil()](@ref lbot::ConditionVariable::waitUntil()) member functions. Afterwards the associated mutex will be locked and owned by the calling thread.
```cpp
std::mutex mutex;
std::unique_lock lock(mutex);
...
lbot::ConditionVariable condition;
condition.waitFor(lock, std::chrono::seconds(1))
```

# Custom clocks
## Synchronized
When setting the `/lbot/clock_mode` parameter to `synchronized` you are required to handle synchronization requests.
This is achieved by receiving the `/synchronized_time/request` topic. The type of the topic is `lbot::Timesync`.
It contains a request timestamp that must be returned on the `/synchronized_time/response` topic, alongside a response timestamp.
The response timestamp should contain the current time of another system.

## Stepped
When setting the `/lbot/clock_mode` parameter to `stepped` you are required to update the clock yourself.
This is achieved by writing on the `/stepped_time/input` topic. The type of the topic must be `lbot::Timestamp`.
Successive updates are required to be in order (the clock cannot be decreased).
The custom clock is a purely discrete clock and no interpolation between update is done.
