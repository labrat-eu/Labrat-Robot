#include <labrat/robot/logger.hpp>
#include <labrat/robot/manager.hpp>
#include <labrat/robot/node.hpp>
#include <labrat/robot/plugins/foxglove-ws/server.hpp>
#include <labrat/robot/plugins/mavlink/msg/command_ack.pb.h>
#include <labrat/robot/plugins/mavlink/msg/command_long.pb.h>
#include <labrat/robot/plugins/mavlink/msg/set_position_target_local_ned.pb.h>
#include <labrat/robot/plugins/mavlink/node.hpp>
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

    client = addClient<labrat::robot::Message<mavlink::msg::common::CommandLong>, labrat::robot::Message<mavlink::msg::common::CommandAck>>(
      "/mavlink/cmd/command_long");

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
        labrat::robot::Message<mavlink::msg::common::CommandAck> response = client->callSync(request, std::chrono::seconds(1));

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

    mode_thread = utils::TimerThread(
      [this]() {
      labrat::robot::Message<mavlink::msg::common::CommandLong> request;

      union Param {
        float raw;
        u32 data;
      };

      Param param1;
      param1.data = 0x81;  // MAV_MODE_FLAG_SAFETY_ARMED ^ MAV_MODE_FLAG_CUSTOM_MODE_ENABLED

      Param param2;
      param2.data = 6;

      Param param3;
      param3.data = 0;

      request().set_target_system(1);
      request().set_target_component(1);
      request().set_command(176);
      request().set_param1(param1.raw);
      request().set_param2(param2.raw);
      request().set_param3(param3.raw);

      try {
        labrat::robot::Message<mavlink::msg::common::CommandAck> response = client->callSync(request, std::chrono::seconds(1));

        if (response().result() == 0) {
          getLogger()() << "Offboard mode engaged.";
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
  Node::Client<labrat::robot::Message<mavlink::msg::common::CommandLong>, labrat::robot::Message<mavlink::msg::common::CommandAck>>::Ptr
    client;

  utils::TimerThread run_thread;
  utils::TimerThread arm_thread;
  utils::TimerThread mode_thread;
};

int main(/*int argc, char **argv*/) {
  labrat::robot::Logger logger("main");
  labrat::robot::Logger::setLogLevel(labrat::robot::Logger::Verbosity::debug);
  logger() << "Starting application.";

  labrat::robot::plugins::McapRecorder recorder("example_mavlink.mcap");
  logger() << "Recording started.";

  labrat::robot::plugins::FoxgloveServer server("Test Server");
  logger() << "Server started.";

  labrat::robot::Manager::get().addNode<labrat::robot::plugins::MavlinkNode>("mavlink",
    std::make_unique<labrat::robot::plugins::MavlinkUdpConnection>());
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
