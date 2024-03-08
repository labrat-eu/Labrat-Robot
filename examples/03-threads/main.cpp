#include <labrat/robot/node.hpp>
#include <labrat/robot/manager.hpp>
#include <labrat/robot/utils/thread.hpp>
#include <labrat/robot/utils/signal.hpp>

#include <cmath>

using namespace labrat;

// Threads
//
// Threads provide a way to compute concurrently.
//
// This example showcases helper classes to make working with threads easier.

class PrimeNode : public robot::Node {
public:
  PrimeNode(const robot::Node::Environment &environment) : robot::Node(environment) {
    // Create two threads, one to continuously calculate prime numbers, and one to output the status every second.

    // The utils::LoopThread class will create a new thread that calls the provided function continuously.
    // You need to specify the function to be called, the name of the thread, the scheduling priority and the function arguments.
    // In this case only the 'this' pointer must be forwarded, as loopFunction() is a non-static function.
    loop_thread = utils::LoopThread(&PrimeNode::loopFunction, "loop_thread", 1, this);

    // The utils::TimerThread class will create a new thread that calls the provided function at a constant time interval of 1 second.
    // You need to specify the function to be called, the time interval, the name of the thread, the scheduling priority and the function arguments.
    // In this case only the 'this' pointer must be forwarded, as timerFunction() is a non-static function.
    timer_thread = utils::TimerThread(&PrimeNode::timerFunction, std::chrono::seconds(1), "timer_thread", 1, this);
  }

private:
  // Will be called continuously.
  void loopFunction() {
    ++i;

    for (uint64_t j = 2; j < std::sqrt(i); ++j) {
      if (i % j == 0) {
        return;
      }
    }

    max_prime = i;
  }

  // Will be called every second.
  void timerFunction() {
    getLogger().logInfo() << "The highest prime so far is: " << max_prime;
  }

  // Threads will automatically join upon destruction of the node.
  utils::LoopThread loop_thread;
  utils::TimerThread timer_thread;

  volatile uint64_t max_prime = 2;
  uint64_t i = 2;
};

int main(int argc, char **argv) {
  robot::Logger logger("main");
  robot::Manager::Ptr manager = robot::Manager::get();

  manager->addNode<PrimeNode>("primes");

  logger.logInfo() << "Press CTRL+C to exit the program.";

  // A helper function utils::signalWait() is provided to simplify waiting on process signals.
  // It is recommended to use utils::signalWait() at the end of your main() function.
  // This way you can control when to shutdown your program via CTRL+C.
  int signal = utils::signalWait();
  logger.logInfo() << "Caught signal (" << signal << "), shutting down.";

  return 0;
}
