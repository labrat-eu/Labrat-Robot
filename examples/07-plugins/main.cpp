#include <labrat/lbot/manager.hpp>
#include <labrat/lbot/node.hpp>
#include <labrat/lbot/plugins/foxglove-ws/server.hpp>
#include <labrat/lbot/plugins/linux/stats.hpp>
#include <labrat/lbot/plugins/mcap/recorder.hpp>
#include <labrat/lbot/utils/signal.hpp>
#include <labrat/lbot/utils/thread.hpp>

// Plugins
//
// Plugins allow you to globally access messages.
// This is useful if you want to record messages for later analysis or if you want to debug you program while it is running.
//
// In this example we will use some existing plugins that might help you in your projects.

int main(int argc, char **argv)
{
  lbot::Logger logger("main");
  lbot::Manager::Ptr manager = lbot::Manager::get();

  // Register some existing plugins.

  // The LinuxStats plugin records statistics about your system.
  // This includes disk usage, CPU load and more.
  manager->addPlugin<lbot::plugins::LinuxStats>("stats");

  // The McapRecorder plugin allows you to trace messages to a mcap file.
  // If you want to see what this program has been doing, you might:
  //  1. Install and open Foxglove Studio (https://foxglove.dev/)
  //  2. Open a local file with the path trace.mcap
  manager->addPlugin<lbot::plugins::McapRecorder>("mcap");

  // The FoxgloveServer plugin allows you to trace messages live via Foxglove Studio.
  // If you want to see what this program is doing, you might:
  //  1. Install and open Foxglove Studio (https://foxglove.dev/)
  //  2. Open a connection via Foxglove WebSocket with the URL ws://localhost:8765
  manager->addPlugin<lbot::plugins::FoxgloveServer>("foxglove-ws");

  logger.logInfo() << "Press CTRL+C to exit the program.";

  int signal = lbot::signalWait();
  logger.logInfo() << "Caught signal (" << signal << "), shutting down.";

  return 0;
}
