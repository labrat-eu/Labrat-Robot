#include <labrat/lbot/manager.hpp>
#include <labrat/lbot/logger.hpp>
#include <labrat/lbot/plugins/gazebo-time/time.hpp>
#include <labrat/lbot/plugins/foxglove-ws/server.hpp>
#include <labrat/lbot/plugins/mcap/recorder.hpp>
#include <labrat/lbot/utils/signal.hpp>

int main(int argc, char **argv) {
  lbot::Logger::setLogLevel(lbot::Logger::Verbosity::debug);

  lbot::Logger logger("main");
  lbot::Manager::Ptr manager = lbot::Manager::get();

  manager->addPlugin<lbot::plugins::GazeboTimeSource>("gazebo-time");
  manager->addPlugin<lbot::plugins::McapRecorder>("mcap");
  manager->addPlugin<lbot::plugins::FoxgloveServer>("foxglove-ws");

  logger.logInfo() << "Press CTRL+C to exit the program.";

  int signal = lbot::signalWait();
  logger.logInfo() << "Caught signal (" << signal << "), shutting down.";

  return 0;
}
