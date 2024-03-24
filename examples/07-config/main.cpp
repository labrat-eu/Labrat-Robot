#include <labrat/lbot/config.hpp>
#include <labrat/lbot/manager.hpp>
#include <labrat/lbot/plugins/foxglove-ws/server.hpp>
#include <labrat/lbot/utils/signal.hpp>

int main(int argc, char **argv) {
  lbot::Logger logger("main");
  lbot::Manager::Ptr manager = lbot::Manager::get();

  lbot::Config::Ptr config = lbot::Config::get();
  config->load("07-config/config.yaml");

  std::unique_ptr<lbot::plugins::FoxgloveServer> foxglove_plugin = std::make_unique<lbot::plugins::FoxgloveServer>("Example Server", 8765);

  logger.logInfo() << "Press CTRL+C to exit the program.";

  int signal = utils::signalWait();
  logger.logInfo() << "Caught signal (" << signal << "), shutting down.";

  return 0;
}
