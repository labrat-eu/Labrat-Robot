#include <labrat/robot/logger.hpp>
#include <labrat/robot/manager.hpp>
#include <labrat/robot/plugins/mavlink/node.hpp>
#include <labrat/robot/plugins/mavlink/udp_connection.hpp>
#include <labrat/robot/plugins/mcap/recorder.hpp>

#include <signal.h>

int main(/*int argc, char **argv*/) {
  labrat::robot::Logger logger("main");
  labrat::robot::Logger::setLogLevel(labrat::robot::Logger::Verbosity::debug);
  logger() << "Starting application.";

  labrat::robot::plugins::McapRecorder recorder("example_mavlink.mcap");
  logger() << "Recording started.";

  labrat::robot::Manager::get().addNode<labrat::robot::plugins::MavlinkNode>("mavlink",
    std::make_unique<labrat::robot::plugins::MavlinkUdpConnection>());
  logger() << "Mavlink node connected.";

  sigset_t signal_mask;
  sigemptyset(&signal_mask);
  sigaddset(&signal_mask, SIGINT);
  sigprocmask(SIG_BLOCK, &signal_mask, NULL);

  int signal;
  sigwait(&signal_mask, &signal);
  logger() << "Caught signal (" << signal << "), shutting down.";

  return 0;
}
