#include <labrat/robot/logger.hpp>
#include <labrat/robot/manager.hpp>
#include <labrat/robot/node.hpp>
#include <labrat/robot/plugins/foxglove-ws/server.hpp>
#include <labrat/robot/plugins/mavlink/msg/command_ack.pb.h>
#include <labrat/robot/plugins/mavlink/msg/command_long.pb.h>
#include <labrat/robot/plugins/mavlink/msg/param_request_read.pb.h>
#include <labrat/robot/plugins/mavlink/msg/param_value.pb.h>
#include <labrat/robot/plugins/mavlink/msg/set_position_target_local_ned.pb.h>
#include <labrat/robot/plugins/mavlink/node.hpp>
#include <labrat/robot/plugins/mavlink/serial_connection.hpp>
#include <labrat/robot/plugins/mavlink/udp_connection.hpp>
#include <labrat/robot/plugins/mcap/recorder.hpp>
#include <labrat/robot/utils/thread.hpp>

#include <atomic>
#include <thread>

#include <signal.h>

class OffboardNode : public labrat::robot::Node {
public:
  OffboardNode(const Node::Environment &environment) : Node(environment) {
    sender =
      addSender<labrat::robot::Message<mavlink::msg::common::SetPositionTargetLocalNed>>("/mavlink/out/set_position_target_local_ned");

    client_param =
      addClient<labrat::robot::Message<mavlink::msg::common::ParamRequestRead>, labrat::robot::Message<mavlink::msg::common::ParamValue>>(
        "/mavlink/srv/param_request_read");
    client_cmd =
      addClient<labrat::robot::Message<mavlink::msg::common::CommandLong>, labrat::robot::Message<mavlink::msg::common::CommandAck>>(
        "/mavlink/srv/command_long");

    for (u8 i = 0; i < 5; ++i) {
      labrat::robot::Message<mavlink::msg::common::ParamRequestRead> request;

      request().set_target_system(1);
      request().set_target_component(1);
      request().set_param_id("CBRK_FLIGHTTERM");
      request().set_param_index(-1);

      try {
        labrat::robot::Message<mavlink::msg::common::ParamValue> response = client_param->callSync(request, std::chrono::minutes(1));

        if (response().param_id() == "CBRK_FLIGHTTERM") {
          const float param_value = response().param_value();
          getLogger()() << "Parameter 'CBRK_FLIGHTTERM' = " << *reinterpret_cast<const u32 *>(&param_value) << ".";
          break;
        } else {
          getLogger().warning() << "Parameter request failed.";
        }
      } catch (labrat::robot::ServiceUnavailableException &) {
      } catch (labrat::robot::ServiceTimeoutException &) {
        getLogger().warning() << "Parameter request did not respond.";
      }

      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    run_thread = utils::TimerThread(
      [this]() {
      labrat::robot::Message<mavlink::msg::common::SetPositionTargetLocalNed> message;

      message().set_target_system(1);
      message().set_target_component(1);
      message().set_coordinate_frame(8);
      message().set_type_mask(~0x838);
      message().set_vx(1.5);
      message().set_vy(1.5);
      message().set_vz(-1);
      message().set_yaw_rate(0.1);

      sender->put(message);
      },
      std::chrono::milliseconds(100));

    arm_thread = utils::TimerThread(
      [this]() {
      labrat::robot::Message<mavlink::msg::common::CommandLong> request;

      request().set_target_system(1);
      request().set_target_component(1);
      request().set_command(400);
      request().set_param1(1);

      try {
        labrat::robot::Message<mavlink::msg::common::CommandAck> response = client_cmd->callSync(request, std::chrono::seconds(1));

        if (response().result() == 0) {
          getLogger()() << "Vehicle armed.";
        } else {
          getLogger().warning() << "Command failed, trying again.";
        }
      } catch (labrat::robot::ServiceUnavailableException &) {
      } catch (labrat::robot::ServiceTimeoutException &) {
        getLogger().warning() << "Command did not respond, trying again.";
      }
      },
      std::chrono::seconds(1));
  }

private:
  Node::Sender<labrat::robot::Message<mavlink::msg::common::SetPositionTargetLocalNed>>::Ptr sender;
  Node::Client<labrat::robot::Message<mavlink::msg::common::ParamRequestRead>,
    labrat::robot::Message<mavlink::msg::common::ParamValue>>::Ptr client_param;
  Node::Client<labrat::robot::Message<mavlink::msg::common::CommandLong>, labrat::robot::Message<mavlink::msg::common::CommandAck>>::Ptr
    client_cmd;

  utils::TimerThread run_thread;
  utils::TimerThread arm_thread;
};

void usage() {
  std::cerr << "Usage: ./example serial|udp" << std::endl;
  exit(1);
}

int main(int argc, char **argv) {
  if (argc < 2) {
    usage();
  }

  bool use_serial = false;

  if (std::string(argv[1]) == "serial") {
    use_serial = true;
  } else if (std::string(argv[1]) == "udp") {
    use_serial = false;
  } else {
    usage();
  }

  labrat::robot::Logger logger("main");
  labrat::robot::Logger::setLogLevel(labrat::robot::Logger::Verbosity::debug);
  logger() << "Starting application.";

  labrat::robot::plugins::McapRecorder recorder("example_mavlink.mcap");
  logger() << "Recording started.";

  labrat::robot::plugins::FoxgloveServer server("Test Server");
  logger() << "Server started.";

  labrat::robot::plugins::MavlinkConnection::Ptr connection;
  if (use_serial) {
    connection = std::make_unique<labrat::robot::plugins::MavlinkSerialConnection>();
  } else {
    connection = std::make_unique<labrat::robot::plugins::MavlinkUdpConnection>();
  }

  labrat::robot::Manager::get().addNode<labrat::robot::plugins::MavlinkNode>("mavlink", std::move(connection));
  logger() << "Mavlink node connected.";

  labrat::robot::Manager::get().addNode<OffboardNode>("offboard");
  logger() << "Offboard node connected.";

  sigset_t signal_mask;
  sigemptyset(&signal_mask);
  sigaddset(&signal_mask, SIGINT);
  sigprocmask(SIG_BLOCK, &signal_mask, NULL);

  int signal;
  sigwait(&signal_mask, &signal);
  logger() << "Caught signal (" << signal << "), shutting down.";

  return 0;
}
