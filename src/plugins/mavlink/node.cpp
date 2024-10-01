/**
 * @file node.cpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v2.1 or later (LGPL-2.1-or-later)
 *
 */

#include <labrat/lbot/message.hpp>
#include <labrat/lbot/node.hpp>
#include <labrat/lbot/plugins/mavlink/connection.hpp>
#include <labrat/lbot/plugins/mavlink/msg/actuator_control_target.hpp>
#include <labrat/lbot/plugins/mavlink/msg/altitude.hpp>
#include <labrat/lbot/plugins/mavlink/msg/attitude.hpp>
#include <labrat/lbot/plugins/mavlink/msg/attitude_quaternion.hpp>
#include <labrat/lbot/plugins/mavlink/msg/attitude_target.hpp>
#include <labrat/lbot/plugins/mavlink/msg/autopilot_version.hpp>
#include <labrat/lbot/plugins/mavlink/msg/battery_status.hpp>
#include <labrat/lbot/plugins/mavlink/msg/command_ack.hpp>
#include <labrat/lbot/plugins/mavlink/msg/command_int.hpp>
#include <labrat/lbot/plugins/mavlink/msg/command_long.hpp>
#include <labrat/lbot/plugins/mavlink/msg/current_event_sequence.hpp>
#include <labrat/lbot/plugins/mavlink/msg/distance_sensor.hpp>
#include <labrat/lbot/plugins/mavlink/msg/esc_info.hpp>
#include <labrat/lbot/plugins/mavlink/msg/esc_status.hpp>
#include <labrat/lbot/plugins/mavlink/msg/estimator_status.hpp>
#include <labrat/lbot/plugins/mavlink/msg/event.hpp>
#include <labrat/lbot/plugins/mavlink/msg/extended_sys_state.hpp>
#include <labrat/lbot/plugins/mavlink/msg/global_position_int.hpp>
#include <labrat/lbot/plugins/mavlink/msg/gps_global_origin.hpp>
#include <labrat/lbot/plugins/mavlink/msg/gps_raw_int.hpp>
#include <labrat/lbot/plugins/mavlink/msg/gps_status.hpp>
#include <labrat/lbot/plugins/mavlink/msg/heartbeat.hpp>
#include <labrat/lbot/plugins/mavlink/msg/highres_imu.hpp>
#include <labrat/lbot/plugins/mavlink/msg/home_position.hpp>
#include <labrat/lbot/plugins/mavlink/msg/link_node_status.hpp>
#include <labrat/lbot/plugins/mavlink/msg/local_position_ned.hpp>
#include <labrat/lbot/plugins/mavlink/msg/local_position_ned_system_global_offset.hpp>
#include <labrat/lbot/plugins/mavlink/msg/mission_ack.hpp>
#include <labrat/lbot/plugins/mavlink/msg/mission_clear_all.hpp>
#include <labrat/lbot/plugins/mavlink/msg/mission_count.hpp>
#include <labrat/lbot/plugins/mavlink/msg/mission_current.hpp>
#include <labrat/lbot/plugins/mavlink/msg/mission_item_int.hpp>
#include <labrat/lbot/plugins/mavlink/msg/mission_item_reached.hpp>
#include <labrat/lbot/plugins/mavlink/msg/mission_request_int.hpp>
#include <labrat/lbot/plugins/mavlink/msg/mission_request_list.hpp>
#include <labrat/lbot/plugins/mavlink/msg/nav_controller_output.hpp>
#include <labrat/lbot/plugins/mavlink/msg/odometry.hpp>
#include <labrat/lbot/plugins/mavlink/msg/open_drone_id_location.hpp>
#include <labrat/lbot/plugins/mavlink/msg/open_drone_id_system.hpp>
#include <labrat/lbot/plugins/mavlink/msg/param_request_read.hpp>
#include <labrat/lbot/plugins/mavlink/msg/param_set.hpp>
#include <labrat/lbot/plugins/mavlink/msg/param_value.hpp>
#include <labrat/lbot/plugins/mavlink/msg/ping.hpp>
#include <labrat/lbot/plugins/mavlink/msg/position_target_global_int.hpp>
#include <labrat/lbot/plugins/mavlink/msg/position_target_local_ned.hpp>
#include <labrat/lbot/plugins/mavlink/msg/raw_imu.hpp>
#include <labrat/lbot/plugins/mavlink/msg/rc_channels.hpp>
#include <labrat/lbot/plugins/mavlink/msg/rc_channels_override.hpp>
#include <labrat/lbot/plugins/mavlink/msg/rc_channels_raw.hpp>
#include <labrat/lbot/plugins/mavlink/msg/scaled_imu.hpp>
#include <labrat/lbot/plugins/mavlink/msg/scaled_pressure.hpp>
#include <labrat/lbot/plugins/mavlink/msg/servo_output_raw.hpp>
#include <labrat/lbot/plugins/mavlink/msg/set_position_target_local_ned.hpp>
#include <labrat/lbot/plugins/mavlink/msg/sys_status.hpp>
#include <labrat/lbot/plugins/mavlink/msg/system_time.hpp>
#include <labrat/lbot/plugins/mavlink/msg/time_estimate_to_target.hpp>
#include <labrat/lbot/plugins/mavlink/msg/timesync.hpp>
#include <labrat/lbot/plugins/mavlink/msg/utm_global_position.hpp>
#include <labrat/lbot/plugins/mavlink/msg/vfr_hud.hpp>
#include <labrat/lbot/plugins/mavlink/msg/vibration.hpp>
#include <labrat/lbot/plugins/mavlink/msg/wind_cov.hpp>
#include <labrat/lbot/plugins/mavlink/node.hpp>
#include <labrat/lbot/utils/cleanup.hpp>
#include <labrat/lbot/utils/thread.hpp>

#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include <mavlink/common/mavlink.h>

inline namespace labrat {
namespace lbot::plugins {

class Mavlink::NodePrivate
{
public:
  std::unordered_map<u16, Node::GenericSender<mavlink_message_t>::Ptr> sender_map;
  std::vector<Node::GenericReceiver<mavlink_message_t>::Ptr> receiver_vector;

  struct MavlinkServer
  {
    template <typename T, typename U>
    struct ServerInfo;

    template <typename T, typename U>
    static Message<U> handle(const Message<T> &request, ServerInfo<T, U> *info);

    template <typename T, typename U>
    struct ServerInfo
    {
      using Ptr = std::unique_ptr<ServerInfo<T, U>>;

      Mavlink::Node *const node;

      typename Node::Sender<T>::Ptr sender;
      typename Node::Receiver<U>::Ptr receiver;
      typename Node::Server<T, U>::Ptr server;

      ServerInfo(Mavlink::Node *node, const std::string &service, const std::string &sender_topic, const std::string &receiver_topic) :
        node(node)
      {
        sender = node->addSender<T>(sender_topic);
        receiver = node->addReceiver<U>(receiver_topic);

        server = node->addServer<T, U>(service);
        server->setHandler(MavlinkServer::handle<T, U>, this);
      }
    };

    ServerInfo<mavlink::common::ParamRequestRead, mavlink::common::ParamValue>::Ptr param_request_read;
    ServerInfo<mavlink::common::CommandInt, mavlink::common::CommandAck>::Ptr command_int;
    ServerInfo<mavlink::common::CommandLong, mavlink::common::CommandAck>::Ptr command_long;
    ServerInfo<mavlink::common::MissionRequestList, mavlink::common::MissionCount>::Ptr command_mission_request_list;
    ServerInfo<mavlink::common::MissionRequestInt, mavlink::common::MissionItemInt>::Ptr command_mission_request_int;
  } server;

  NodePrivate(MavlinkConnection::Ptr &&connection, Mavlink::Node &node);
  ~NodePrivate();

  void writeMessage(const mavlink_message_t &message);

  Mavlink::SystemInfo system_info;

private:
  template <typename FlatbufferType>
  void addSender(std::string &&topic_name, u16 id)
  {
    node.registerSender<Mavlink::Node::MavlinkMessage<FlatbufferType>>(std::forward<std::string>(topic_name), id);
  }

  template <typename FlatbufferType>
  void addReceiver(std::string &&topic_name)
  {
    node.registerReceiver<Mavlink::Node::MavlinkMessage<FlatbufferType>>(std::forward<std::string>(topic_name));
  }

  template <typename RequestType, typename ResponseType>
  typename MavlinkServer::ServerInfo<RequestType, ResponseType>::Ptr
  addServer(const std::string &service, const std::string &sender_topic, const std::string &receiver_topic)
  {
    typename MavlinkServer::ServerInfo<RequestType, ResponseType>::Ptr result =
      std::make_unique<MavlinkServer::ServerInfo<RequestType, ResponseType>>(&node, service, sender_topic, receiver_topic);

    return result;
  }

  void readLoop();
  void heartbeatLoop();

  void readMessage(const mavlink_message_t &message);

  MavlinkConnection::Ptr connection;
  Mavlink::Node &node;

  static constexpr std::size_t buffer_size = 1024;

  std::mutex mutex;

  LoopThread read_thread;
  TimerThread heartbeat_thread;

public:
  CleanupLock lock;
};

template <>
Message<mavlink::common::ParamValue>
Mavlink::NodePrivate::MavlinkServer::handle<mavlink::common::ParamRequestRead, mavlink::common::ParamValue>(
  const Message<mavlink::common::ParamRequestRead> &request,
  ServerInfo<mavlink::common::ParamRequestRead, mavlink::common::ParamValue> *info
)
{
  Message<mavlink::common::ParamValue> result;

  do {
    info->sender->put(request);

    try {
      result = info->receiver->next(std::chrono::seconds(1));
    } catch (TopicNoDataAvailableException &) {
      throw ServiceUnavailableException("MAVLink parameter request failed due to flushed topic.", info->node->getLogger());
    } catch (TopicTimeoutException &) {
      throw ServiceTimeoutException("MAVLink  parameter request failed due to timeout.", info->node->getLogger());
      ;
    }
  } while (result.param_id != request.param_id);

  return result;
}

template <>
Message<mavlink::common::CommandAck> Mavlink::NodePrivate::MavlinkServer::handle<mavlink::common::CommandInt, mavlink::common::CommandAck>(
  const Message<mavlink::common::CommandInt> &request,
  ServerInfo<mavlink::common::CommandInt, mavlink::common::CommandAck> *info
)
{
  Message<mavlink::common::CommandAck> result;

  do {
    info->sender->put(request);

    try {
      result = info->receiver->next(std::chrono::seconds(1));
    } catch (TopicNoDataAvailableException &) {
      throw ServiceUnavailableException("MAVLink command failed due to flushed topic.", info->node->getLogger());
    } catch (TopicTimeoutException &) {
      throw ServiceTimeoutException("MAVLink command failed due to timeout.", info->node->getLogger());
      ;
    }
  } while (result.command != request.command);

  return result;
}

template <>
Message<mavlink::common::CommandAck> Mavlink::NodePrivate::MavlinkServer::handle<mavlink::common::CommandLong, mavlink::common::CommandAck>(
  const Message<mavlink::common::CommandLong> &request,
  ServerInfo<mavlink::common::CommandLong, mavlink::common::CommandAck> *info
)
{
  Message<mavlink::common::CommandAck> result;

  do {
    info->sender->put(request);

    try {
      result = info->receiver->next(std::chrono::seconds(1));
    } catch (TopicNoDataAvailableException &) {
      throw ServiceUnavailableException("MAVLink command failed due to flushed topic.", info->node->getLogger());
    } catch (TopicTimeoutException &) {
      throw ServiceTimeoutException("MAVLink command failed due to timeout.", info->node->getLogger());
      ;
    }
  } while (result.command != request.command);

  return result;
}

template <>
Message<mavlink::common::MissionCount>
Mavlink::NodePrivate::MavlinkServer::handle<mavlink::common::MissionRequestList, mavlink::common::MissionCount>(
  const Message<mavlink::common::MissionRequestList> &request,
  ServerInfo<mavlink::common::MissionRequestList, mavlink::common::MissionCount> *info
)
{
  Message<mavlink::common::MissionCount> result;

  do {
    info->sender->put(request);

    try {
      result = info->receiver->next(std::chrono::seconds(1));
    } catch (TopicNoDataAvailableException &) {
      throw ServiceUnavailableException("MAVLink mission list request failed due to flushed topic.", info->node->getLogger());
    } catch (TopicTimeoutException &) {
      throw ServiceTimeoutException("MAVLink mission list request failed due to timeout.", info->node->getLogger());
      ;
    }
  } while (result.mission_type != request.mission_type);

  return result;
}

template <>
Message<mavlink::common::MissionItemInt>
Mavlink::NodePrivate::MavlinkServer::handle<mavlink::common::MissionRequestInt, mavlink::common::MissionItemInt>(
  const Message<mavlink::common::MissionRequestInt> &request,
  ServerInfo<mavlink::common::MissionRequestInt, mavlink::common::MissionItemInt> *info
)
{
  Message<mavlink::common::MissionItemInt> result;

  do {
    info->sender->put(request);

    try {
      result = info->receiver->next(std::chrono::seconds(1));
    } catch (TopicNoDataAvailableException &) {
      throw ServiceUnavailableException("MAVLink mission item request failed due to flushed topic.", info->node->getLogger());
    } catch (TopicTimeoutException &) {
      throw ServiceTimeoutException("MAVLink mission item request failed due to timeout.", info->node->getLogger());
      ;
    }
  } while (result.seq != request.seq);

  return result;
}

Mavlink::Mavlink(MavlinkConnection::Ptr &&connection) :
  Plugin()
{
  node = addNode<Node>(getName(), std::forward<MavlinkConnection::Ptr>(connection));
}

Mavlink::~Mavlink() = default;

Mavlink::Node::Node(MavlinkConnection::Ptr &&connection)
{
  std::allocator<Mavlink::NodePrivate> allocator;
  priv = allocator.allocate(1);

  priv = std::construct_at(priv, std::forward<MavlinkConnection::Ptr>(connection), *this);
}

Mavlink::Node::~Node()
{
  delete priv;
}

Mavlink::SystemInfo &Mavlink::Node::getSystemInfo() const
{
  return priv->system_info;
}

void Mavlink::Node::registerGenericSender(Node::GenericSender<mavlink_message_t>::Ptr &&sender, u16 id)
{
  if (!priv->sender_map.try_emplace(id, std::forward<Node::GenericSender<mavlink_message_t>::Ptr>(sender)).second) {
    throw ManagementException("A sender has already been registered for the MAVLink message ID '" + std::to_string(id) + "'.");
  }
}

void Mavlink::Node::registerGenericReceiver(Node::GenericReceiver<mavlink_message_t>::Ptr &&receiver)
{
  priv->receiver_vector.emplace_back(std::forward<Node::GenericReceiver<mavlink_message_t>::Ptr>(receiver));
}

void Mavlink::Node::receiverCallback(const mavlink_message_t &message, SystemInfo *system_info)
{
  Mavlink::NodePrivate *node = system_info->node;
  CleanupLock::Guard guard = node->lock.lock();

  if (!guard.valid()) {
    return;
  }

  node->writeMessage(message);
}

Mavlink::NodePrivate::NodePrivate(MavlinkConnection::Ptr &&connection, Mavlink::Node &node) :
  connection(std::forward<MavlinkConnection::Ptr>(connection)),
  node(node)
{
  system_info.node = this;
  system_info.channel_id = MAVLINK_COMM_0;
  system_info.system_id = 0xf0;
  system_info.component_id = MAV_COMP_ID_ONBOARD_COMPUTER;

  addSender<mavlink::common::Heartbeat>("/" + node.getName() + "/in/heartbeat", MAVLINK_MSG_ID_HEARTBEAT);
  addSender<mavlink::common::SysStatus>("/" + node.getName() + "/in/sys_status", MAVLINK_MSG_ID_SYS_STATUS);
  addSender<mavlink::common::SystemTime>("/" + node.getName() + "/in/system_time", MAVLINK_MSG_ID_SYSTEM_TIME);
  addSender<mavlink::common::Ping>("/" + node.getName() + "/in/ping", MAVLINK_MSG_ID_PING);
  addSender<mavlink::common::LinkNodeStatus>("/" + node.getName() + "/in/link_node_status", MAVLINK_MSG_ID_LINK_NODE_STATUS);
  addSender<mavlink::common::ParamValue>("/" + node.getName() + "/in/param_value", MAVLINK_MSG_ID_PARAM_VALUE);
  addSender<mavlink::common::ParamSet>("/" + node.getName() + "/in/param_set", MAVLINK_MSG_ID_PARAM_SET);
  addSender<mavlink::common::GpsRawInt>("/" + node.getName() + "/in/gps_raw_int", MAVLINK_MSG_ID_GPS_RAW_INT);
  addSender<mavlink::common::GpsStatus>("/" + node.getName() + "/in/gps_status", MAVLINK_MSG_ID_GPS_STATUS);
  addSender<mavlink::common::ScaledImu>("/" + node.getName() + "/in/scaled_imu", MAVLINK_MSG_ID_SCALED_IMU);
  addSender<mavlink::common::RawImu>("/" + node.getName() + "/in/raw_imu", MAVLINK_MSG_ID_RAW_IMU);
  addSender<mavlink::common::ScaledPressure>("/" + node.getName() + "/in/scaled_pressure", MAVLINK_MSG_ID_SCALED_PRESSURE);
  addSender<mavlink::common::Attitude>("/" + node.getName() + "/in/attitude", MAVLINK_MSG_ID_ATTITUDE);
  addSender<mavlink::common::AttitudeQuaternion>("/" + node.getName() + "/in/attitude_quaternion", MAVLINK_MSG_ID_ATTITUDE_QUATERNION);
  addSender<mavlink::common::LocalPositionNed>("/" + node.getName() + "/in/local_position_ned", MAVLINK_MSG_ID_LOCAL_POSITION_NED);
  addSender<mavlink::common::GlobalPositionInt>("/" + node.getName() + "/in/global_position_int", MAVLINK_MSG_ID_GLOBAL_POSITION_INT);
  addSender<mavlink::common::GpsGlobalOrigin>("/" + node.getName() + "/in/gps_global_origin", MAVLINK_MSG_ID_GPS_GLOBAL_ORIGIN);
  addSender<mavlink::common::NavControllerOutput>("/" + node.getName() + "/in/nav_controller_output", MAVLINK_MSG_ID_NAV_CONTROLLER_OUTPUT);
  addSender<mavlink::common::RcChannels>("/" + node.getName() + "/in/rc_channels", MAVLINK_MSG_ID_RC_CHANNELS);
  addSender<mavlink::common::RcChannelsRaw>("/" + node.getName() + "/in/rc_channels_raw", MAVLINK_MSG_ID_RC_CHANNELS_RAW);
  addSender<mavlink::common::RcChannelsOverride>("/" + node.getName() + "/in/rc_channels_override", MAVLINK_MSG_ID_RC_CHANNELS_OVERRIDE);
  addSender<mavlink::common::ServoOutputRaw>("/" + node.getName() + "/in/servo_output_raw", MAVLINK_MSG_ID_SERVO_OUTPUT_RAW);
  addSender<mavlink::common::VfrHud>("/" + node.getName() + "/in/vfr_hud", MAVLINK_MSG_ID_VFR_HUD);
  addSender<mavlink::common::CommandInt>("/" + node.getName() + "/in/command_int", MAVLINK_MSG_ID_COMMAND_INT);
  addSender<mavlink::common::CommandLong>("/" + node.getName() + "/in/command_long", MAVLINK_MSG_ID_COMMAND_LONG);
  addSender<mavlink::common::CommandAck>("/" + node.getName() + "/in/command_ack", MAVLINK_MSG_ID_COMMAND_ACK);
  addSender<mavlink::common::PositionTargetLocalNed>(
    "/" + node.getName() + "/in/position_target_local_ned", MAVLINK_MSG_ID_POSITION_TARGET_LOCAL_NED
  );
  addSender<mavlink::common::AttitudeTarget>("/" + node.getName() + "/in/attitude_target", MAVLINK_MSG_ID_ATTITUDE_TARGET);
  addSender<mavlink::common::PositionTargetGlobalInt>(
    "/" + node.getName() + "/in/position_target_global_int", MAVLINK_MSG_ID_POSITION_TARGET_GLOBAL_INT
  );
  addSender<mavlink::common::LocalPositionNedSystemGlobalOffset>(
    "/" + node.getName() + "/in/local_position_ned_system_global_offset", MAVLINK_MSG_ID_LOCAL_POSITION_NED_SYSTEM_GLOBAL_OFFSET
  );
  addSender<mavlink::common::HighresImu>("/" + node.getName() + "/in/highres_imu", MAVLINK_MSG_ID_HIGHRES_IMU);
  addSender<mavlink::common::Timesync>("/" + node.getName() + "/in/timesync", MAVLINK_MSG_ID_TIMESYNC);
  addSender<mavlink::common::ActuatorControlTarget>(
    "/" + node.getName() + "/in/actuator_control_target", MAVLINK_MSG_ID_ACTUATOR_CONTROL_TARGET
  );
  addSender<mavlink::common::Altitude>("/" + node.getName() + "/in/altitude", MAVLINK_MSG_ID_ALTITUDE);
  addSender<mavlink::common::BatteryStatus>("/" + node.getName() + "/in/battery_status", MAVLINK_MSG_ID_BATTERY_STATUS);
  addSender<mavlink::common::AutopilotVersion>("/" + node.getName() + "/in/autopilot_version", MAVLINK_MSG_ID_AUTOPILOT_VERSION);
  addSender<mavlink::common::EstimatorStatus>("/" + node.getName() + "/in/estimator_status", MAVLINK_MSG_ID_ESTIMATOR_STATUS);
  addSender<mavlink::common::Vibration>("/" + node.getName() + "/in/vibration", MAVLINK_MSG_ID_VIBRATION);
  addSender<mavlink::common::HomePosition>("/" + node.getName() + "/in/home_position", MAVLINK_MSG_ID_HOME_POSITION);
  addSender<mavlink::common::ExtendedSysState>("/" + node.getName() + "/in/extended_sys_state", MAVLINK_MSG_ID_EXTENDED_SYS_STATE);
  addSender<mavlink::common::EscInfo>("/" + node.getName() + "/in/esc_info", MAVLINK_MSG_ID_ESC_INFO);
  addSender<mavlink::common::EscStatus>("/" + node.getName() + "/in/esc_status", MAVLINK_MSG_ID_ESC_STATUS);
  addSender<mavlink::common::MissionCurrent>("/" + node.getName() + "/in/mission_current", MAVLINK_MSG_ID_MISSION_CURRENT);
  addSender<mavlink::common::MissionRequestList>("/" + node.getName() + "/in/mission_request_list", MAVLINK_MSG_ID_MISSION_REQUEST_LIST);
  addSender<mavlink::common::MissionRequestInt>("/" + node.getName() + "/in/mission_request_int", MAVLINK_MSG_ID_MISSION_REQUEST_INT);
  addSender<mavlink::common::MissionCount>("/" + node.getName() + "/in/mission_count", MAVLINK_MSG_ID_MISSION_COUNT);
  addSender<mavlink::common::MissionClearAll>("/" + node.getName() + "/in/mission_clear_all", MAVLINK_MSG_ID_MISSION_CLEAR_ALL);
  addSender<mavlink::common::MissionItemReached>("/" + node.getName() + "/in/mission_item_reached", MAVLINK_MSG_ID_MISSION_ITEM_REACHED);
  addSender<mavlink::common::MissionItemInt>("/" + node.getName() + "/in/mission_item_int", MAVLINK_MSG_ID_MISSION_ITEM_INT);
  addSender<mavlink::common::MissionAck>("/" + node.getName() + "/in/mission_ack", MAVLINK_MSG_ID_MISSION_ACK);
  addSender<mavlink::common::Odometry>("/" + node.getName() + "/in/odometry", MAVLINK_MSG_ID_ODOMETRY);
  addSender<mavlink::common::UtmGlobalPosition>("/" + node.getName() + "/in/utm_global_position", MAVLINK_MSG_ID_UTM_GLOBAL_POSITION);
  addSender<mavlink::common::TimeEstimateToTarget>(
    "/" + node.getName() + "/in/time_estimate_to_target", MAVLINK_MSG_ID_TIME_ESTIMATE_TO_TARGET
  );
  addSender<mavlink::common::CurrentEventSequence>(
    "/" + node.getName() + "/in/current_event_sequence", MAVLINK_MSG_ID_CURRENT_EVENT_SEQUENCE
  );
  addSender<mavlink::common::DistanceSensor>("/" + node.getName() + "/in/distance_sensor", MAVLINK_MSG_ID_DISTANCE_SENSOR);
  addSender<mavlink::common::OpenDroneIdLocation>(
    "/" + node.getName() + "/in/open_drone_id_location", MAVLINK_MSG_ID_OPEN_DRONE_ID_LOCATION
  );
  addSender<mavlink::common::OpenDroneIdSystem>("/" + node.getName() + "/in/open_drone_id_system", MAVLINK_MSG_ID_OPEN_DRONE_ID_SYSTEM);
  addSender<mavlink::common::WindCov>("/" + node.getName() + "/in/wind_cov", MAVLINK_MSG_ID_WIND_COV);
  addSender<mavlink::common::Event>("/" + node.getName() + "/in/event", MAVLINK_MSG_ID_EVENT);

  addReceiver<mavlink::common::ParamRequestRead>("/" + node.getName() + "/out/param_request_read");
  addReceiver<mavlink::common::SetPositionTargetLocalNed>("/" + node.getName() + "/out/set_position_target_local_ned");
  addReceiver<mavlink::common::CommandInt>("/" + node.getName() + "/out/command_int");
  addReceiver<mavlink::common::CommandLong>("/" + node.getName() + "/out/command_long");
  addReceiver<mavlink::common::Timesync>("/" + node.getName() + "/out/timesync");
  addReceiver<mavlink::common::SystemTime>("/" + node.getName() + "/out/system_time");
  addReceiver<mavlink::common::RcChannelsOverride>("/" + node.getName() + "/out/rc_channels_override");
  addReceiver<mavlink::common::MissionRequestList>("/" + node.getName() + "/out/mission_request_list");
  addReceiver<mavlink::common::MissionRequestInt>("/" + node.getName() + "/out/mission_request_int");
  addReceiver<mavlink::common::MissionAck>("/" + node.getName() + "/out/mission_ack");

  server.param_request_read = addServer<mavlink::common::ParamRequestRead, mavlink::common::ParamValue>(
    "/" + node.getName() + "/srv/param_request_read",
    "/" + node.getName() + "/out/param_request_read",
    "/" + node.getName() + "/in/param_value"
  );
  server.command_int = addServer<mavlink::common::CommandInt, mavlink::common::CommandAck>(
    "/" + node.getName() + "/srv/command_int", "/" + node.getName() + "/out/command_int", "/" + node.getName() + "/in/command_ack"
  );
  server.command_long = addServer<mavlink::common::CommandLong, mavlink::common::CommandAck>(
    "/" + node.getName() + "/srv/command_long", "/" + node.getName() + "/out/command_long", "/" + node.getName() + "/in/command_ack"
  );
  server.command_mission_request_list = addServer<mavlink::common::MissionRequestList, mavlink::common::MissionCount>(
    "/" + node.getName() + "/srv/mission_request_list",
    "/" + node.getName() + "/out/mission_request_list",
    "/" + node.getName() + "/in/mission_count"
  );
  server.command_mission_request_int = addServer<mavlink::common::MissionRequestInt, mavlink::common::MissionItemInt>(
    "/" + node.getName() + "/srv/mission_request_int",
    "/" + node.getName() + "/out/mission_request_int",
    "/" + node.getName() + "/in/mission_item_int"
  );

  read_thread = LoopThread(&Mavlink::NodePrivate::readLoop, "mavlink read", 1, this);
  heartbeat_thread = TimerThread(&Mavlink::NodePrivate::heartbeatLoop, std::chrono::milliseconds(500), "mavlink hb", 1, this);
}

Mavlink::NodePrivate::~NodePrivate() = default;

void Mavlink::NodePrivate::readLoop()
{
  std::array<u8, buffer_size> buffer;
  const std::size_t number_bytes = connection->read(buffer.data(), buffer_size);

  for (std::size_t i = 0; i < number_bytes; ++i) {
    mavlink_message_t message;
    mavlink_status_t status;

    if (mavlink_parse_char(system_info.channel_id, buffer[i], &message, &status) == MAVLINK_FRAMING_OK) {
      readMessage(message);
    }
  }
}

void Mavlink::NodePrivate::heartbeatLoop()
{
  mavlink_message_t message;
  mavlink_msg_heartbeat_pack_chan(
    system_info.system_id,
    system_info.component_id,
    system_info.channel_id,
    &message,
    MAV_TYPE_ONBOARD_CONTROLLER,
    MAV_AUTOPILOT_INVALID,
    0,
    0,
    MAV_STATE_ACTIVE
  );

  writeMessage(message);
}

void Mavlink::NodePrivate::readMessage(const mavlink_message_t &message)
{
  auto iterator = sender_map.find(message.msgid);

  if (iterator == sender_map.end()) {
    node.getLogger().logDebug() << "Received MAVLink message without handling implementation (ID: " << message.msgid << ").";
    return;
  }

  iterator->second->put(message);
}

void Mavlink::NodePrivate::writeMessage(const mavlink_message_t &message)
{
  std::array<u8, MAVLINK_MAX_PACKET_LEN> buffer;
  const std::size_t number_bytes = mavlink_msg_to_send_buffer(buffer.data(), &message);

  std::lock_guard guard(mutex);

  connection->write(buffer.data(), number_bytes);
}

}  // namespace lbot::plugins
}  // namespace labrat
