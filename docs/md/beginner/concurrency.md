@page concurrency Concurrency

@note
You may also want to take a look at the [code example](@ref example_concurrency).

# Threads
Almost any robotics project requires concurrent computation. Concurrency is performed through threads. Lbot provides some helper classes to make working with threads easier.

## Loop Threads
The [lbot::LoopThread()](@ref lbot::LoopThread) class will create a new thread that calls the provided function continuously. You need to specify the function to be called, the name of the thread, the scheduling priority and the function arguments. If the specified function is a non-static member function the `this` pointer must be forwarded. Note that the specified function must be [invocable](https://en.cppreference.com/w/cpp/types/is_invocable).
```cpp
lbot::LoopThread loop_thread(&loopFunction, "loop_thread", 1, ...);
```
The created thread will be scheduled as a [SCHED_RR](https://www.man7.org/linux/man-pages/man7/sched.7.html) real-time thread with the specified [static scheduling priority](https://www.man7.org/linux/man-pages/man7/sched.7.html). The specified name will be visible in programs like top & htop.

The normal usage pattern for threads is to declare them as a member of a node. You can then create them inside the constructor of the node. The thread function is likely a non-static member function of the node.
```cpp
class ExampleNode : public lbot::Node {
public:
  ExampleNode(const lbot::Node::Environment &environment) : lbot::Node(environment) {
    loop_thread = lbot::LoopThread(&ExampleNode::loopFunction, "loop_thread", 1, this);
  }

private:
  void loopFunction() { ... }

  lbot::LoopThread loop_thread;
};
```

When the created [lbot::LoopThread()](@ref lbot::LoopThread) object is deleted (usually when the containing node is destructed), the associated thread will automatically join once the current iteration has been completed. This way you have to worry less about properly exiting your threads.

It is recommended to use [lbot::LoopThread()](@ref lbot::LoopThread) whenever you have to wait on any potentially blocking topics or I/O inside your thread function.

## Timer Threads
The [lbot::TimerThread()](@ref lbot::TimerThread) class behaves exactly like [lbot::LoopThread()](@ref lbot::LoopThread), except that it calls the provided function at a constant time interval. The time interval must be specified at construction.
```cpp
lbot::TimerThread timer_thread(&timerFunction, std::chrono::seconds(1), "timer_thread", 1, ...);
```

@note
If the execution time of the provided function is greater than the specified time interval, the function will not be called twice. Only one call is made at a time. **There is no guarantee, that the specified time interval will be achieved**.

It is recommended to use [lbot::TimerThread()](@ref lbot::TimerThread) for polling of non-blocking topics or I/O as well as low-priority sporadic tasks.

@attention
You should ensure that threads are the very first objects to be deleted upon destruction of an object. This way you can avoid data races between the time a thread is joined and other member fields being destructed. This can be achieved by declaring thread members as the very last members of a class.

# Signals
In your `main()` function you might want to wait on a signal to exit the program whenever the user sees fit. Lbot provides a small helper function [lbot::signalWait()](@ref lbot::signalWait()) that waits until an interrupt has been triggered (SIGINT signal has been received). This way you can control when to shutdown your program via `CTRL+C`.
```cpp
int main() {
  lbot::Logger logger("main");
  int signal = lbot::signalWait();
  logger.logInfo() << "Caught signal (" << signal << "), shutting down.";

  return 0;
}
```