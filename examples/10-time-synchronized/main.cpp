#include <labrat/lbot/config.hpp>
#include <labrat/lbot/manager.hpp>
#include <labrat/lbot/msg/timesync.hpp>
#include <labrat/lbot/node.hpp>
#include <labrat/lbot/plugins/foxglove-ws/server.hpp>
#include <labrat/lbot/plugins/mcap/recorder.hpp>
#include <labrat/lbot/utils/signal.hpp>
#include <labrat/lbot/utils/thread.hpp>

#include <chrono>

// Time (synchronized)
//
// There exist multiple feasable time sources to synchronize a robotics program.
//
// In this example we will showcase the synchronized time mode.

class TimeNode : public lbot::Node
{
public:
  TimeNode()
  {
    sender_response = addSender<lbot::Timesync>("/synchronized_time/response");

    receiver_request = addReceiver<const lbot::Timesync>("/synchronized_time/request");
    receiver_request->setCallback(&TimeNode::callback, this);
  }

private:
  static void callback(const lbot::Message<lbot::Timesync> &message_in, TimeNode *self)
  {
    const auto now = std::chrono::system_clock::now().time_since_epoch();

    lbot::Message<lbot::Timesync> message_out;

    message_out.request = std::make_unique<foxglove::Time>(*message_in.request);
    message_out.response = std::make_unique<foxglove::Time>(
      std::chrono::duration_cast<std::chrono::seconds>(now).count(),
      (std::chrono::duration_cast<std::chrono::nanoseconds>(now) % std::chrono::seconds(1)).count()
    );

    self->sender_response->put(message_out);
  }

  Sender<lbot::Timesync>::Ptr sender_response;
  Receiver<const lbot::Timesync>::Ptr receiver_request;
};

class ExampleNode : public lbot::Node
{
public:
  ExampleNode()
  {
    thread = lbot::TimerThread(&ExampleNode::loopFunction, std::chrono::seconds(1), "sender_thread", 1, this);
  }

private:
  void loopFunction()
  {
    getLogger().logInfo() << "Test message (" << i++ << ")";
  }

  lbot::TimerThread thread;

  uint64_t i = 0;
};

int main(int argc, char **argv)
{
  // Specify that we use an external time source.
  lbot::Config::Ptr config = lbot::Config::get();
  config->setParameter("/lbot/clock_mode", "synchronized");

  lbot::Logger logger("main");
  lbot::Manager::Ptr manager = lbot::Manager::get();

  // Register custom time source node.
  manager->addNode<TimeNode>("time");

  manager->addPlugin<lbot::plugins::McapRecorder>("mcap");
  manager->addPlugin<lbot::plugins::FoxgloveServer>("foxglove-ws");

  manager->addNode<ExampleNode>("example");

  logger.logInfo() << "Press CTRL+C to exit the program.";

  int signal = lbot::signalWait();
  logger.logInfo() << "Caught signal (" << signal << "), shutting down.";

  return 0;
}
