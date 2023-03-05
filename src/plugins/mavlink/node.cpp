/**
 * @file node.cpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#include <labrat/robot/message.hpp>
#include <labrat/robot/node.hpp>
#include <labrat/robot/plugins/mavlink/connection.hpp>
#include <labrat/robot/plugins/mavlink/node.hpp>
#include <labrat/robot/utils/string.hpp>
#include <labrat/robot/utils/thread.hpp>
#include <labrat/robot/utils/cleanup.hpp>
#include <mavlink/msg/common/actuator_control_target.fb.hpp>
#include <mavlink/msg/common/altitude.fb.hpp>
#include <mavlink/msg/common/attitude.fb.hpp>
#include <mavlink/msg/common/attitude_quaternion.fb.hpp>
#include <mavlink/msg/common/attitude_target.fb.hpp>
#include <mavlink/msg/common/autopilot_version.fb.hpp>
#include <mavlink/msg/common/battery_status.fb.hpp>
#include <mavlink/msg/common/command_ack.fb.hpp>
#include <mavlink/msg/common/command_int.fb.hpp>
#include <mavlink/msg/common/command_long.fb.hpp>
#include <mavlink/msg/common/current_event_sequence.fb.hpp>
#include <mavlink/msg/common/esc_info.fb.hpp>
#include <mavlink/msg/common/esc_status.fb.hpp>
#include <mavlink/msg/common/estimator_status.fb.hpp>
#include <mavlink/msg/common/extended_sys_state.fb.hpp>
#include <mavlink/msg/common/global_position_int.fb.hpp>
#include <mavlink/msg/common/gps_raw_int.fb.hpp>
#include <mavlink/msg/common/gps_status.fb.hpp>
#include <mavlink/msg/common/heartbeat.fb.hpp>
#include <mavlink/msg/common/highres_imu.fb.hpp>
#include <mavlink/msg/common/home_position.fb.hpp>
#include <mavlink/msg/common/link_node_status.fb.hpp>
#include <mavlink/msg/common/local_position_ned.fb.hpp>
#include <mavlink/msg/common/local_position_ned_system_global_offset.fb.hpp>
#include <mavlink/msg/common/odometry.fb.hpp>
#include <mavlink/msg/common/open_drone_id_location.fb.hpp>
#include <mavlink/msg/common/open_drone_id_system.fb.hpp>
#include <mavlink/msg/common/param_request_read.fb.hpp>
#include <mavlink/msg/common/param_set.fb.hpp>
#include <mavlink/msg/common/param_value.fb.hpp>
#include <mavlink/msg/common/ping.fb.hpp>
#include <mavlink/msg/common/position_target_global_int.fb.hpp>
#include <mavlink/msg/common/position_target_local_ned.fb.hpp>
#include <mavlink/msg/common/raw_imu.fb.hpp>
#include <mavlink/msg/common/rc_channels.fb.hpp>
#include <mavlink/msg/common/rc_channels_raw.fb.hpp>
#include <mavlink/msg/common/scaled_imu.fb.hpp>
#include <mavlink/msg/common/scaled_pressure.fb.hpp>
#include <mavlink/msg/common/servo_output_raw.fb.hpp>
#include <mavlink/msg/common/set_position_target_local_ned.fb.hpp>
#include <mavlink/msg/common/sys_status.fb.hpp>
#include <mavlink/msg/common/system_time.fb.hpp>
#include <mavlink/msg/common/time_estimate_to_target.fb.hpp>
#include <mavlink/msg/common/timesync.fb.hpp>
#include <mavlink/msg/common/utm_global_position.fb.hpp>
#include <mavlink/msg/common/vfr_hud.fb.hpp>
#include <mavlink/msg/common/vibration.fb.hpp>

#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include <mavlink/common/mavlink.h>

namespace labrat::robot::plugins {

class MavlinkNodePrivate {
public:
  struct MavlinkSender {
    template <typename T>
    static void convert(const mavlink_message_t &source, Message<T> &destination, const void *);

    std::unordered_map<u16, Node::GenericSender<mavlink_message_t>::Ptr> map;
  } sender;

  struct MavlinkReceiver {
    template <typename T>
    static void convert(const Message<T> &source, mavlink_message_t &destination, const MavlinkNode::SystemInfo *info);

    std::vector<Node::GenericReceiver<mavlink_message_t>::Ptr> vector;
  } receiver;

  struct MavlinkServer {
    template <typename T, typename U>
    struct ServerInfo;

    template <typename T, typename U>
    static Message<U> handle(const Message<T> &request, ServerInfo<T, U> *info);

    template <typename T, typename U>
    struct ServerInfo {
      using Ptr = std::unique_ptr<ServerInfo<T, U>>;

      MavlinkNode *const node;

      typename Node::Sender<Message<T>>::Ptr sender;
      typename Node::Receiver<Message<U>>::Ptr receiver;
      typename Node::Server<Message<T>, Message<U>>::Ptr server;

      ServerInfo(MavlinkNode *node, const std::string &service, const std::string &sender_topic, const std::string &receiver_topic) :
        node(node) {
        sender = node->addSender<Message<T>>(sender_topic);
        receiver = node->addReceiver<Message<U>>(receiver_topic);

        Message<U> (*ptr)(const Message<T> &, ServerInfo<T, U> *) = handle<T, U>;
        server = node->addServer<Message<T>, Message<U>>(service, ptr, this);
      }
    };

    ServerInfo<mavlink::msg::common::ParamRequestRead, mavlink::msg::common::ParamValue>::Ptr param_request_read;
    ServerInfo<mavlink::msg::common::CommandInt, mavlink::msg::common::CommandAck>::Ptr command_int;
    ServerInfo<mavlink::msg::common::CommandLong, mavlink::msg::common::CommandAck>::Ptr command_long;
  } server;

  MavlinkNodePrivate(MavlinkConnection::Ptr &&connection, MavlinkNode &node);
  ~MavlinkNodePrivate();

  void writeMessage(const mavlink_message_t &message);

  MavlinkNode::SystemInfo system_info;

private:
  template <typename MessageType>
  void addSender(const std::string &topic_name, u16 id) {
    node.registerSender<MessageType>(topic_name, MavlinkSender::convert<MessageType>, id, &system_info);
  }

  template <typename MessageType>
  void addReceiver(const std::string &topic_name) {
    node.registerReceiver<MessageType>(topic_name, MavlinkReceiver::convert<MessageType>, &system_info);
  }

  template <typename RequestType, typename ResponseType>
  typename MavlinkServer::ServerInfo<RequestType, ResponseType>::Ptr addServer(const std::string &service, const std::string &sender_topic,
    const std::string &receiver_topic) {
    typename MavlinkServer::ServerInfo<RequestType, ResponseType>::Ptr result =
      std::make_unique<MavlinkServer::ServerInfo<RequestType, ResponseType>>(&node, service, sender_topic, receiver_topic);

    return result;
  }

  void readLoop();
  void heartbeatLoop();

  void readMessage(const mavlink_message_t &message);

  MavlinkConnection::Ptr connection;
  MavlinkNode &node;

  static constexpr std::size_t buffer_size = 1024;

  utils::LoopThread read_thread;
  utils::TimerThread heartbeat_thread;

  std::mutex mutex;

public:
  utils::CleanupLock lock;
};

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::Heartbeat>(const mavlink_message_t &source,
  Message<mavlink::msg::common::Heartbeat> &destination, const void *) {
  destination().type = mavlink_msg_heartbeat_get_type(&source);
  destination().autopilot = mavlink_msg_heartbeat_get_autopilot(&source);
  destination().base_mode = mavlink_msg_heartbeat_get_base_mode(&source);
  destination().custom_mode = mavlink_msg_heartbeat_get_custom_mode(&source);
  destination().system_status = mavlink_msg_heartbeat_get_system_status(&source);
  destination().mavlink_version = mavlink_msg_heartbeat_get_mavlink_version(&source);
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::SysStatus>(const mavlink_message_t &source,
  Message<mavlink::msg::common::SysStatus> &destination, const void *) {
  destination().onboard_control_sensors_present = mavlink_msg_sys_status_get_onboard_control_sensors_present(&source);
  destination().onboard_control_sensors_enabled = mavlink_msg_sys_status_get_onboard_control_sensors_enabled(&source);
  destination().onboard_control_sensors_health = mavlink_msg_sys_status_get_onboard_control_sensors_health(&source);
  destination().load = mavlink_msg_sys_status_get_load(&source);
  destination().voltage_battery = mavlink_msg_sys_status_get_voltage_battery(&source);
  destination().current_battery = mavlink_msg_sys_status_get_current_battery(&source);
  destination().drop_rate_comm = mavlink_msg_sys_status_get_drop_rate_comm(&source);
  destination().errors_comm = mavlink_msg_sys_status_get_errors_comm(&source);
  destination().errors_count1 = mavlink_msg_sys_status_get_errors_count1(&source);
  destination().errors_count2 = mavlink_msg_sys_status_get_errors_count2(&source);
  destination().errors_count3 = mavlink_msg_sys_status_get_errors_count3(&source);
  destination().errors_count4 = mavlink_msg_sys_status_get_errors_count4(&source);
  destination().battery_remaining = mavlink_msg_sys_status_get_battery_remaining(&source);
  destination().onboard_control_sensors_present_extended = mavlink_msg_sys_status_get_onboard_control_sensors_present_extended(&source);
  destination().onboard_control_sensors_enabled_extended = mavlink_msg_sys_status_get_onboard_control_sensors_enabled_extended(&source);
  destination().onboard_control_sensors_health_extended = mavlink_msg_sys_status_get_onboard_control_sensors_health_extended(&source);
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::SystemTime>(const mavlink_message_t &source,
  Message<mavlink::msg::common::SystemTime> &destination, const void *) {
  destination().time_unix_usec = mavlink_msg_system_time_get_time_unix_usec(&source);
  destination().time_boot_ms = mavlink_msg_system_time_get_time_boot_ms(&source);
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::Ping>(const mavlink_message_t &source,
  Message<mavlink::msg::common::Ping> &destination, const void *) {
  destination().time_usec = mavlink_msg_ping_get_time_usec(&source);
  destination().seq = mavlink_msg_ping_get_seq(&source);
  destination().target_system = mavlink_msg_ping_get_target_system(&source);
  destination().target_component = mavlink_msg_ping_get_target_component(&source);
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::LinkNodeStatus>(const mavlink_message_t &source,
  Message<mavlink::msg::common::LinkNodeStatus> &destination, const void *) {
  destination().timestamp = mavlink_msg_link_node_status_get_timestamp(&source);
  destination().tx_rate = mavlink_msg_link_node_status_get_tx_rate(&source);
  destination().rx_rate = mavlink_msg_link_node_status_get_rx_rate(&source);
  destination().messages_sent = mavlink_msg_link_node_status_get_messages_sent(&source);
  destination().messages_received = mavlink_msg_link_node_status_get_messages_received(&source);
  destination().messages_lost = mavlink_msg_link_node_status_get_messages_lost(&source);
  destination().rx_parse_err = mavlink_msg_link_node_status_get_rx_parse_err(&source);
  destination().tx_overflows = mavlink_msg_link_node_status_get_tx_overflows(&source);
  destination().rx_overflows = mavlink_msg_link_node_status_get_rx_overflows(&source);
  destination().tx_buf = mavlink_msg_link_node_status_get_tx_buf(&source);
  destination().rx_buf = mavlink_msg_link_node_status_get_rx_buf(&source);
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::ParamValue>(const mavlink_message_t &source,
  Message<mavlink::msg::common::ParamValue> &destination, const void *) {
  destination().param_value = mavlink_msg_param_value_get_param_value(&source);
  destination().param_count = mavlink_msg_param_value_get_param_count(&source);
  destination().param_index = mavlink_msg_param_value_get_param_index(&source);

  destination().param_id.resize(17);
  mavlink_msg_param_value_get_param_id(&source, destination().param_id.data());
  utils::shrinkString(destination().param_id);

  destination().param_type = mavlink_msg_param_value_get_param_type(&source);
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::ParamSet>(const mavlink_message_t &source,
  Message<mavlink::msg::common::ParamSet> &destination, const void *) {
  destination().param_value = mavlink_msg_param_set_get_param_value(&source);
  destination().target_system = mavlink_msg_param_set_get_target_system(&source);
  destination().target_component = mavlink_msg_param_set_get_target_component(&source);

  destination().param_id.resize(17);
  mavlink_msg_param_set_get_param_id(&source, destination().param_id.data());
  utils::shrinkString(destination().param_id);

  destination().param_type = mavlink_msg_param_set_get_param_type(&source);
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::GpsRawInt>(const mavlink_message_t &source,
  Message<mavlink::msg::common::GpsRawInt> &destination, const void *) {
  destination().time_usec = mavlink_msg_gps_raw_int_get_time_usec(&source);
  destination().lat = mavlink_msg_gps_raw_int_get_lat(&source);
  destination().lon = mavlink_msg_gps_raw_int_get_lon(&source);
  destination().alt = mavlink_msg_gps_raw_int_get_alt(&source);
  destination().eph = mavlink_msg_gps_raw_int_get_eph(&source);
  destination().epv = mavlink_msg_gps_raw_int_get_epv(&source);
  destination().vel = mavlink_msg_gps_raw_int_get_vel(&source);
  destination().cog = mavlink_msg_gps_raw_int_get_cog(&source);
  destination().fix_type = mavlink_msg_gps_raw_int_get_fix_type(&source);
  destination().satellites_visible = mavlink_msg_gps_raw_int_get_satellites_visible(&source);
  destination().alt_ellipsoid = mavlink_msg_gps_raw_int_get_alt_ellipsoid(&source);
  destination().h_acc = mavlink_msg_gps_raw_int_get_h_acc(&source);
  destination().v_acc = mavlink_msg_gps_raw_int_get_v_acc(&source);
  destination().vel_acc = mavlink_msg_gps_raw_int_get_vel_acc(&source);
  destination().hdg_acc = mavlink_msg_gps_raw_int_get_hdg_acc(&source);
  destination().yaw = mavlink_msg_gps_raw_int_get_yaw(&source);
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::GpsStatus>(const mavlink_message_t &source,
  Message<mavlink::msg::common::GpsStatus> &destination, const void *) {
  destination().satellites_visible = mavlink_msg_gps_status_get_satellites_visible(&source);

  destination().satellite_prn.resize(20);
  mavlink_msg_gps_status_get_satellite_prn(&source, destination().satellite_prn.data());

  destination().satellite_used.resize(20);
  mavlink_msg_gps_status_get_satellite_used(&source, destination().satellite_used.data());

  destination().satellite_elevation.resize(20);
  mavlink_msg_gps_status_get_satellite_elevation(&source, destination().satellite_elevation.data());

  destination().satellite_azimuth.resize(20);
  mavlink_msg_gps_status_get_satellite_azimuth(&source, destination().satellite_azimuth.data());

  destination().satellite_snr.resize(20);
  mavlink_msg_gps_status_get_satellite_snr(&source, destination().satellite_snr.data());
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::ScaledImu>(const mavlink_message_t &source,
  Message<mavlink::msg::common::ScaledImu> &destination, const void *) {
  destination().time_boot_ms = mavlink_msg_scaled_imu_get_time_boot_ms(&source);
  destination().xacc = mavlink_msg_scaled_imu_get_xacc(&source);
  destination().yacc = mavlink_msg_scaled_imu_get_yacc(&source);
  destination().zacc = mavlink_msg_scaled_imu_get_zacc(&source);
  destination().xgyro = mavlink_msg_scaled_imu_get_xgyro(&source);
  destination().ygyro = mavlink_msg_scaled_imu_get_ygyro(&source);
  destination().zgyro = mavlink_msg_scaled_imu_get_zgyro(&source);
  destination().xmag = mavlink_msg_scaled_imu_get_xmag(&source);
  destination().ymag = mavlink_msg_scaled_imu_get_ymag(&source);
  destination().zmag = mavlink_msg_scaled_imu_get_zmag(&source);
  destination().temperature = mavlink_msg_scaled_imu_get_temperature(&source);
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::RawImu>(const mavlink_message_t &source,
  Message<mavlink::msg::common::RawImu> &destination, const void *) {
  destination().time_usec = mavlink_msg_raw_imu_get_time_usec(&source);
  destination().xacc = mavlink_msg_raw_imu_get_xacc(&source);
  destination().yacc = mavlink_msg_raw_imu_get_yacc(&source);
  destination().zacc = mavlink_msg_raw_imu_get_zacc(&source);
  destination().xgyro = mavlink_msg_raw_imu_get_xgyro(&source);
  destination().ygyro = mavlink_msg_raw_imu_get_ygyro(&source);
  destination().zgyro = mavlink_msg_raw_imu_get_zgyro(&source);
  destination().xmag = mavlink_msg_raw_imu_get_xmag(&source);
  destination().ymag = mavlink_msg_raw_imu_get_ymag(&source);
  destination().zmag = mavlink_msg_raw_imu_get_zmag(&source);
  destination().id = mavlink_msg_raw_imu_get_id(&source);
  destination().temperature = mavlink_msg_raw_imu_get_temperature(&source);
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::ScaledPressure>(const mavlink_message_t &source,
  Message<mavlink::msg::common::ScaledPressure> &destination, const void *) {
  destination().time_boot_ms = mavlink_msg_scaled_pressure_get_time_boot_ms(&source);
  destination().press_abs = mavlink_msg_scaled_pressure_get_press_abs(&source);
  destination().press_diff = mavlink_msg_scaled_pressure_get_press_diff(&source);
  destination().temperature = mavlink_msg_scaled_pressure_get_temperature(&source);
  destination().temperature_press_diff = mavlink_msg_scaled_pressure_get_temperature_press_diff(&source);
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::Attitude>(const mavlink_message_t &source,
  Message<mavlink::msg::common::Attitude> &destination, const void *) {
  destination().time_boot_ms = mavlink_msg_attitude_get_time_boot_ms(&source);
  destination().roll = mavlink_msg_attitude_get_roll(&source);
  destination().pitch = mavlink_msg_attitude_get_pitch(&source);
  destination().yaw = mavlink_msg_attitude_get_yaw(&source);
  destination().rollspeed = mavlink_msg_attitude_get_rollspeed(&source);
  destination().pitchspeed = mavlink_msg_attitude_get_pitchspeed(&source);
  destination().yawspeed = mavlink_msg_attitude_get_yawspeed(&source);
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::AttitudeQuaternion>(const mavlink_message_t &source,
  Message<mavlink::msg::common::AttitudeQuaternion> &destination, const void *) {
  destination().time_boot_ms = mavlink_msg_attitude_quaternion_get_time_boot_ms(&source);
  destination().q1 = mavlink_msg_attitude_quaternion_get_q1(&source);
  destination().q2 = mavlink_msg_attitude_quaternion_get_q2(&source);
  destination().q3 = mavlink_msg_attitude_quaternion_get_q3(&source);
  destination().q4 = mavlink_msg_attitude_quaternion_get_q4(&source);
  destination().rollspeed = mavlink_msg_attitude_quaternion_get_rollspeed(&source);
  destination().pitchspeed = mavlink_msg_attitude_quaternion_get_pitchspeed(&source);
  destination().yawspeed = mavlink_msg_attitude_quaternion_get_yawspeed(&source);

  destination().repr_offset_q.resize(4);
  mavlink_msg_attitude_quaternion_get_repr_offset_q(&source, destination().repr_offset_q.data());
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::LocalPositionNed>(const mavlink_message_t &source,
  Message<mavlink::msg::common::LocalPositionNed> &destination, const void *) {
  destination().time_boot_ms = mavlink_msg_local_position_ned_get_time_boot_ms(&source);
  destination().x = mavlink_msg_local_position_ned_get_x(&source);
  destination().y = mavlink_msg_local_position_ned_get_y(&source);
  destination().z = mavlink_msg_local_position_ned_get_z(&source);
  destination().vx = mavlink_msg_local_position_ned_get_vx(&source);
  destination().vy = mavlink_msg_local_position_ned_get_vy(&source);
  destination().vz = mavlink_msg_local_position_ned_get_vz(&source);
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::GlobalPositionInt>(const mavlink_message_t &source,
  Message<mavlink::msg::common::GlobalPositionInt> &destination, const void *) {
  destination().time_boot_ms = mavlink_msg_global_position_int_get_time_boot_ms(&source);
  destination().lat = mavlink_msg_global_position_int_get_lat(&source);
  destination().lon = mavlink_msg_global_position_int_get_lon(&source);
  destination().alt = mavlink_msg_global_position_int_get_alt(&source);
  destination().relative_alt = mavlink_msg_global_position_int_get_relative_alt(&source);
  destination().vx = mavlink_msg_global_position_int_get_vx(&source);
  destination().vy = mavlink_msg_global_position_int_get_vy(&source);
  destination().vz = mavlink_msg_global_position_int_get_vz(&source);
  destination().hdg = mavlink_msg_global_position_int_get_hdg(&source);
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::RcChannelsRaw>(const mavlink_message_t &source,
  Message<mavlink::msg::common::RcChannelsRaw> &destination, const void *) {
  destination().time_boot_ms = mavlink_msg_rc_channels_raw_get_time_boot_ms(&source);
  destination().chan1_raw = mavlink_msg_rc_channels_raw_get_chan1_raw(&source);
  destination().chan2_raw = mavlink_msg_rc_channels_raw_get_chan2_raw(&source);
  destination().chan3_raw = mavlink_msg_rc_channels_raw_get_chan3_raw(&source);
  destination().chan4_raw = mavlink_msg_rc_channels_raw_get_chan4_raw(&source);
  destination().chan5_raw = mavlink_msg_rc_channels_raw_get_chan5_raw(&source);
  destination().chan6_raw = mavlink_msg_rc_channels_raw_get_chan6_raw(&source);
  destination().chan7_raw = mavlink_msg_rc_channels_raw_get_chan7_raw(&source);
  destination().chan8_raw = mavlink_msg_rc_channels_raw_get_chan8_raw(&source);
  destination().port = mavlink_msg_rc_channels_raw_get_port(&source);
  destination().rssi = mavlink_msg_rc_channels_raw_get_rssi(&source);
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::RcChannels>(const mavlink_message_t &source,
  Message<mavlink::msg::common::RcChannels> &destination, const void *) {
  destination().time_boot_ms = mavlink_msg_rc_channels_get_time_boot_ms(&source);
  destination().chan1_raw = mavlink_msg_rc_channels_get_chan1_raw(&source);
  destination().chan2_raw = mavlink_msg_rc_channels_get_chan2_raw(&source);
  destination().chan3_raw = mavlink_msg_rc_channels_get_chan3_raw(&source);
  destination().chan4_raw = mavlink_msg_rc_channels_get_chan4_raw(&source);
  destination().chan5_raw = mavlink_msg_rc_channels_get_chan5_raw(&source);
  destination().chan6_raw = mavlink_msg_rc_channels_get_chan6_raw(&source);
  destination().chan7_raw = mavlink_msg_rc_channels_get_chan7_raw(&source);
  destination().chan8_raw = mavlink_msg_rc_channels_get_chan8_raw(&source);
  destination().chan9_raw = mavlink_msg_rc_channels_get_chan9_raw(&source);
  destination().chan10_raw = mavlink_msg_rc_channels_get_chan10_raw(&source);
  destination().chan11_raw = mavlink_msg_rc_channels_get_chan11_raw(&source);
  destination().chan12_raw = mavlink_msg_rc_channels_get_chan12_raw(&source);
  destination().chan13_raw = mavlink_msg_rc_channels_get_chan13_raw(&source);
  destination().chan14_raw = mavlink_msg_rc_channels_get_chan14_raw(&source);
  destination().chan15_raw = mavlink_msg_rc_channels_get_chan15_raw(&source);
  destination().chan16_raw = mavlink_msg_rc_channels_get_chan16_raw(&source);
  destination().chan17_raw = mavlink_msg_rc_channels_get_chan17_raw(&source);
  destination().chan18_raw = mavlink_msg_rc_channels_get_chan18_raw(&source);
  destination().chancount = mavlink_msg_rc_channels_get_chancount(&source);
  destination().rssi = mavlink_msg_rc_channels_get_rssi(&source);
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::ServoOutputRaw>(const mavlink_message_t &source,
  Message<mavlink::msg::common::ServoOutputRaw> &destination, const void *) {
  destination().time_usec = mavlink_msg_servo_output_raw_get_time_usec(&source);
  destination().servo1_raw = mavlink_msg_servo_output_raw_get_servo1_raw(&source);
  destination().servo2_raw = mavlink_msg_servo_output_raw_get_servo2_raw(&source);
  destination().servo3_raw = mavlink_msg_servo_output_raw_get_servo3_raw(&source);
  destination().servo4_raw = mavlink_msg_servo_output_raw_get_servo4_raw(&source);
  destination().servo5_raw = mavlink_msg_servo_output_raw_get_servo5_raw(&source);
  destination().servo6_raw = mavlink_msg_servo_output_raw_get_servo6_raw(&source);
  destination().servo7_raw = mavlink_msg_servo_output_raw_get_servo7_raw(&source);
  destination().servo8_raw = mavlink_msg_servo_output_raw_get_servo8_raw(&source);
  destination().port = mavlink_msg_servo_output_raw_get_port(&source);
  destination().servo9_raw = mavlink_msg_servo_output_raw_get_servo9_raw(&source);
  destination().servo10_raw = mavlink_msg_servo_output_raw_get_servo10_raw(&source);
  destination().servo11_raw = mavlink_msg_servo_output_raw_get_servo11_raw(&source);
  destination().servo12_raw = mavlink_msg_servo_output_raw_get_servo12_raw(&source);
  destination().servo13_raw = mavlink_msg_servo_output_raw_get_servo13_raw(&source);
  destination().servo14_raw = mavlink_msg_servo_output_raw_get_servo14_raw(&source);
  destination().servo15_raw = mavlink_msg_servo_output_raw_get_servo15_raw(&source);
  destination().servo16_raw = mavlink_msg_servo_output_raw_get_servo16_raw(&source);
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::VfrHud>(const mavlink_message_t &source,
  Message<mavlink::msg::common::VfrHud> &destination, const void *) {
  destination().airspeed = mavlink_msg_vfr_hud_get_airspeed(&source);
  destination().groundspeed = mavlink_msg_vfr_hud_get_groundspeed(&source);
  destination().alt = mavlink_msg_vfr_hud_get_alt(&source);
  destination().climb = mavlink_msg_vfr_hud_get_climb(&source);
  destination().heading = mavlink_msg_vfr_hud_get_heading(&source);
  destination().throttle = mavlink_msg_vfr_hud_get_throttle(&source);
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::CommandInt>(const mavlink_message_t &source,
  Message<mavlink::msg::common::CommandInt> &destination, const void *) {
  destination().param1 = mavlink_msg_command_int_get_param1(&source);
  destination().param2 = mavlink_msg_command_int_get_param2(&source);
  destination().param3 = mavlink_msg_command_int_get_param3(&source);
  destination().param4 = mavlink_msg_command_int_get_param4(&source);
  destination().x = mavlink_msg_command_int_get_x(&source);
  destination().y = mavlink_msg_command_int_get_y(&source);
  destination().z = mavlink_msg_command_int_get_z(&source);
  destination().command = mavlink_msg_command_int_get_command(&source);
  destination().target_system = mavlink_msg_command_int_get_target_system(&source);
  destination().target_component = mavlink_msg_command_int_get_target_component(&source);
  destination().frame = mavlink_msg_command_int_get_frame(&source);
  destination().current = mavlink_msg_command_int_get_current(&source);
  destination().autocontinue = mavlink_msg_command_int_get_autocontinue(&source);
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::CommandLong>(const mavlink_message_t &source,
  Message<mavlink::msg::common::CommandLong> &destination, const void *) {
  destination().param1 = mavlink_msg_command_long_get_param1(&source);
  destination().param2 = mavlink_msg_command_long_get_param2(&source);
  destination().param3 = mavlink_msg_command_long_get_param3(&source);
  destination().param4 = mavlink_msg_command_long_get_param4(&source);
  destination().param5 = mavlink_msg_command_long_get_param5(&source);
  destination().param6 = mavlink_msg_command_long_get_param6(&source);
  destination().param7 = mavlink_msg_command_long_get_param7(&source);
  destination().command = mavlink_msg_command_long_get_command(&source);
  destination().target_system = mavlink_msg_command_long_get_target_system(&source);
  destination().target_component = mavlink_msg_command_long_get_target_component(&source);
  destination().confirmation = mavlink_msg_command_long_get_confirmation(&source);
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::CommandAck>(const mavlink_message_t &source,
  Message<mavlink::msg::common::CommandAck> &destination, const void *) {
  destination().command = mavlink_msg_command_ack_get_command(&source);
  destination().result = mavlink_msg_command_ack_get_result(&source);
  destination().progress = mavlink_msg_command_ack_get_progress(&source);
  destination().result_param2 = mavlink_msg_command_ack_get_result_param2(&source);
  destination().target_system = mavlink_msg_command_ack_get_target_system(&source);
  destination().target_component = mavlink_msg_command_ack_get_target_component(&source);
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::PositionTargetLocalNed>(const mavlink_message_t &source,
  Message<mavlink::msg::common::PositionTargetLocalNed> &destination, const void *) {
  destination().time_boot_ms = mavlink_msg_position_target_local_ned_get_time_boot_ms(&source);
  destination().x = mavlink_msg_position_target_local_ned_get_x(&source);
  destination().y = mavlink_msg_position_target_local_ned_get_y(&source);
  destination().z = mavlink_msg_position_target_local_ned_get_z(&source);
  destination().vx = mavlink_msg_position_target_local_ned_get_vx(&source);
  destination().vy = mavlink_msg_position_target_local_ned_get_vy(&source);
  destination().vz = mavlink_msg_position_target_local_ned_get_vz(&source);
  destination().afx = mavlink_msg_position_target_local_ned_get_afx(&source);
  destination().afy = mavlink_msg_position_target_local_ned_get_afy(&source);
  destination().afz = mavlink_msg_position_target_local_ned_get_afz(&source);
  destination().yaw = mavlink_msg_position_target_local_ned_get_yaw(&source);
  destination().yaw_rate = mavlink_msg_position_target_local_ned_get_yaw_rate(&source);
  destination().type_mask = mavlink_msg_position_target_local_ned_get_type_mask(&source);
  destination().coordinate_frame = mavlink_msg_position_target_local_ned_get_coordinate_frame(&source);
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::AttitudeTarget>(const mavlink_message_t &source,
  Message<mavlink::msg::common::AttitudeTarget> &destination, const void *) {
  destination().time_boot_ms = mavlink_msg_attitude_target_get_time_boot_ms(&source);

  destination().q.resize(4);
  mavlink_msg_attitude_target_get_q(&source, destination().q.data());

  destination().body_roll_rate = mavlink_msg_attitude_target_get_body_roll_rate(&source);
  destination().body_pitch_rate = mavlink_msg_attitude_target_get_body_pitch_rate(&source);
  destination().body_yaw_rate = mavlink_msg_attitude_target_get_body_yaw_rate(&source);
  destination().thrust = mavlink_msg_attitude_target_get_thrust(&source);
  destination().type_mask = mavlink_msg_attitude_target_get_type_mask(&source);
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::PositionTargetGlobalInt>(const mavlink_message_t &source,
  Message<mavlink::msg::common::PositionTargetGlobalInt> &destination, const void *) {
  destination().time_boot_ms = mavlink_msg_position_target_global_int_get_time_boot_ms(&source);
  destination().lat_int = mavlink_msg_position_target_global_int_get_lat_int(&source);
  destination().lon_int = mavlink_msg_position_target_global_int_get_lon_int(&source);
  destination().alt = mavlink_msg_position_target_global_int_get_alt(&source);
  destination().vx = mavlink_msg_position_target_global_int_get_vx(&source);
  destination().vy = mavlink_msg_position_target_global_int_get_vy(&source);
  destination().vz = mavlink_msg_position_target_global_int_get_vz(&source);
  destination().afx = mavlink_msg_position_target_global_int_get_afx(&source);
  destination().afy = mavlink_msg_position_target_global_int_get_afy(&source);
  destination().afz = mavlink_msg_position_target_global_int_get_afz(&source);
  destination().yaw = mavlink_msg_position_target_global_int_get_yaw(&source);
  destination().yaw_rate = mavlink_msg_position_target_global_int_get_yaw_rate(&source);
  destination().type_mask = mavlink_msg_position_target_global_int_get_type_mask(&source);
  destination().coordinate_frame = mavlink_msg_position_target_global_int_get_coordinate_frame(&source);
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::LocalPositionNedSystemGlobalOffset>(const mavlink_message_t &source,
  Message<mavlink::msg::common::LocalPositionNedSystemGlobalOffset> &destination, const void *) {
  destination().time_boot_ms = mavlink_msg_local_position_ned_system_global_offset_get_time_boot_ms(&source);
  destination().x = mavlink_msg_local_position_ned_system_global_offset_get_x(&source);
  destination().y = mavlink_msg_local_position_ned_system_global_offset_get_y(&source);
  destination().z = mavlink_msg_local_position_ned_system_global_offset_get_z(&source);
  destination().roll = mavlink_msg_local_position_ned_system_global_offset_get_roll(&source);
  destination().pitch = mavlink_msg_local_position_ned_system_global_offset_get_pitch(&source);
  destination().yaw = mavlink_msg_local_position_ned_system_global_offset_get_yaw(&source);
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::HighresImu>(const mavlink_message_t &source,
  Message<mavlink::msg::common::HighresImu> &destination, const void *) {
  destination().time_usec = mavlink_msg_highres_imu_get_time_usec(&source);
  destination().xacc = mavlink_msg_highres_imu_get_xacc(&source);
  destination().yacc = mavlink_msg_highres_imu_get_yacc(&source);
  destination().zacc = mavlink_msg_highres_imu_get_zacc(&source);
  destination().xgyro = mavlink_msg_highres_imu_get_xgyro(&source);
  destination().ygyro = mavlink_msg_highres_imu_get_ygyro(&source);
  destination().zgyro = mavlink_msg_highres_imu_get_zgyro(&source);
  destination().xmag = mavlink_msg_highres_imu_get_xmag(&source);
  destination().ymag = mavlink_msg_highres_imu_get_ymag(&source);
  destination().zmag = mavlink_msg_highres_imu_get_zmag(&source);
  destination().abs_pressure = mavlink_msg_highres_imu_get_abs_pressure(&source);
  destination().diff_pressure = mavlink_msg_highres_imu_get_diff_pressure(&source);
  destination().pressure_alt = mavlink_msg_highres_imu_get_pressure_alt(&source);
  destination().temperature = mavlink_msg_highres_imu_get_temperature(&source);
  destination().fields_updated = mavlink_msg_highres_imu_get_fields_updated(&source);
  destination().id = mavlink_msg_highres_imu_get_id(&source);
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::Timesync>(const mavlink_message_t &source,
  Message<mavlink::msg::common::Timesync> &destination, const void *) {
  destination().tc1 = mavlink_msg_timesync_get_tc1(&source);
  destination().ts1 = mavlink_msg_timesync_get_ts1(&source);
  destination().target_system = mavlink_msg_timesync_get_target_system(&source);
  destination().target_component = mavlink_msg_timesync_get_target_component(&source);
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::ActuatorControlTarget>(const mavlink_message_t &source,
  Message<mavlink::msg::common::ActuatorControlTarget> &destination, const void *) {
  destination().time_usec = mavlink_msg_actuator_control_target_get_time_usec(&source);

  destination().controls.resize(8);
  mavlink_msg_actuator_control_target_get_controls(&source, destination().controls.data());

  destination().group_mlx = mavlink_msg_actuator_control_target_get_group_mlx(&source);
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::Altitude>(const mavlink_message_t &source,
  Message<mavlink::msg::common::Altitude> &destination, const void *) {
  destination().time_usec = mavlink_msg_altitude_get_time_usec(&source);
  destination().altitude_monotonic = mavlink_msg_altitude_get_altitude_monotonic(&source);
  destination().altitude_amsl = mavlink_msg_altitude_get_altitude_amsl(&source);
  destination().altitude_local = mavlink_msg_altitude_get_altitude_local(&source);
  destination().altitude_relative = mavlink_msg_altitude_get_altitude_relative(&source);
  destination().altitude_terrain = mavlink_msg_altitude_get_altitude_terrain(&source);
  destination().bottom_clearance = mavlink_msg_altitude_get_bottom_clearance(&source);
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::BatteryStatus>(const mavlink_message_t &source,
  Message<mavlink::msg::common::BatteryStatus> &destination, const void *) {
  destination().current_consumed = mavlink_msg_battery_status_get_current_consumed(&source);
  destination().energy_consumed = mavlink_msg_battery_status_get_energy_consumed(&source);
  destination().temperature = mavlink_msg_battery_status_get_temperature(&source);

  destination().voltages.resize(10);
  mavlink_msg_battery_status_get_voltages(&source, destination().voltages.data());

  destination().current_battery = mavlink_msg_battery_status_get_current_battery(&source);
  destination().id = mavlink_msg_battery_status_get_id(&source);
  destination().battery_function = mavlink_msg_battery_status_get_battery_function(&source);
  destination().type = mavlink_msg_battery_status_get_type(&source);
  destination().battery_remaining = mavlink_msg_battery_status_get_battery_remaining(&source);
  destination().time_remaining = mavlink_msg_battery_status_get_time_remaining(&source);
  destination().charge_state = mavlink_msg_battery_status_get_charge_state(&source);

  destination().voltages_ext.resize(4);
  mavlink_msg_battery_status_get_voltages_ext(&source, destination().voltages_ext.data());

  destination().mode = mavlink_msg_battery_status_get_mode(&source);
  destination().fault_bitmask = mavlink_msg_battery_status_get_fault_bitmask(&source);
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::AutopilotVersion>(const mavlink_message_t &source,
  Message<mavlink::msg::common::AutopilotVersion> &destination, const void *) {
  destination().capabilities = mavlink_msg_autopilot_version_get_capabilities(&source);
  destination().uid = mavlink_msg_autopilot_version_get_uid(&source);
  destination().flight_sw_version = mavlink_msg_autopilot_version_get_flight_sw_version(&source);
  destination().middleware_sw_version = mavlink_msg_autopilot_version_get_middleware_sw_version(&source);
  destination().os_sw_version = mavlink_msg_autopilot_version_get_os_sw_version(&source);
  destination().board_version = mavlink_msg_autopilot_version_get_board_version(&source);
  destination().vendor_id = mavlink_msg_autopilot_version_get_vendor_id(&source);
  destination().product_id = mavlink_msg_autopilot_version_get_product_id(&source);

  destination().flight_custom_version.resize(8);
  mavlink_msg_autopilot_version_get_flight_custom_version(&source, destination().flight_custom_version.data());

  destination().middleware_custom_version.resize(8);
  mavlink_msg_autopilot_version_get_middleware_custom_version(&source, destination().middleware_custom_version.data());

  destination().os_custom_version.resize(8);
  mavlink_msg_autopilot_version_get_os_custom_version(&source, destination().os_custom_version.data());

  destination().uid2.resize(18);
  mavlink_msg_autopilot_version_get_uid2(&source, destination().uid2.data());
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::EstimatorStatus>(const mavlink_message_t &source,
  Message<mavlink::msg::common::EstimatorStatus> &destination, const void *) {
  destination().time_usec = mavlink_msg_estimator_status_get_time_usec(&source);
  destination().vel_ratio = mavlink_msg_estimator_status_get_vel_ratio(&source);
  destination().pos_horiz_ratio = mavlink_msg_estimator_status_get_pos_horiz_ratio(&source);
  destination().pos_vert_ratio = mavlink_msg_estimator_status_get_pos_vert_ratio(&source);
  destination().mag_ratio = mavlink_msg_estimator_status_get_mag_ratio(&source);
  destination().hagl_ratio = mavlink_msg_estimator_status_get_hagl_ratio(&source);
  destination().tas_ratio = mavlink_msg_estimator_status_get_tas_ratio(&source);
  destination().pos_horiz_accuracy = mavlink_msg_estimator_status_get_pos_horiz_accuracy(&source);
  destination().pos_vert_accuracy = mavlink_msg_estimator_status_get_pos_vert_accuracy(&source);
  destination().flags = mavlink_msg_estimator_status_get_flags(&source);
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::Vibration>(const mavlink_message_t &source,
  Message<mavlink::msg::common::Vibration> &destination, const void *) {
  destination().time_usec = mavlink_msg_vibration_get_time_usec(&source);
  destination().vibration_x = mavlink_msg_vibration_get_vibration_x(&source);
  destination().vibration_y = mavlink_msg_vibration_get_vibration_y(&source);
  destination().vibration_z = mavlink_msg_vibration_get_vibration_z(&source);
  destination().clipping_0 = mavlink_msg_vibration_get_clipping_0(&source);
  destination().clipping_1 = mavlink_msg_vibration_get_clipping_1(&source);
  destination().clipping_2 = mavlink_msg_vibration_get_clipping_2(&source);
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::HomePosition>(const mavlink_message_t &source,
  Message<mavlink::msg::common::HomePosition> &destination, const void *) {
  destination().latitude = mavlink_msg_home_position_get_latitude(&source);
  destination().longitude = mavlink_msg_home_position_get_longitude(&source);
  destination().altitude = mavlink_msg_home_position_get_altitude(&source);
  destination().x = mavlink_msg_home_position_get_x(&source);
  destination().y = mavlink_msg_home_position_get_y(&source);
  destination().z = mavlink_msg_home_position_get_z(&source);

  destination().q.resize(4);
  mavlink_msg_home_position_get_q(&source, destination().q.data());

  destination().approach_x = mavlink_msg_home_position_get_approach_x(&source);
  destination().approach_y = mavlink_msg_home_position_get_approach_y(&source);
  destination().approach_z = mavlink_msg_home_position_get_approach_z(&source);
  destination().time_usec = mavlink_msg_home_position_get_time_usec(&source);
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::ExtendedSysState>(const mavlink_message_t &source,
  Message<mavlink::msg::common::ExtendedSysState> &destination, const void *) {
  destination().vtol_state = mavlink_msg_extended_sys_state_get_vtol_state(&source);
  destination().landed_state = mavlink_msg_extended_sys_state_get_landed_state(&source);
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::EscInfo>(const mavlink_message_t &source,
  Message<mavlink::msg::common::EscInfo> &destination, const void *) {
  destination().time_usec = mavlink_msg_esc_info_get_time_usec(&source);

  destination().error_count.resize(4);
  mavlink_msg_esc_info_get_error_count(&source, destination().error_count.data());

  destination().counter = mavlink_msg_esc_info_get_counter(&source);

  destination().failure_flags.resize(4);
  mavlink_msg_esc_info_get_failure_flags(&source, destination().failure_flags.data());

  destination().temperature.resize(4);
  mavlink_msg_esc_info_get_temperature(&source, destination().temperature.data());

  destination().index = mavlink_msg_esc_info_get_index(&source);
  destination().count = mavlink_msg_esc_info_get_count(&source);
  destination().connection_type = mavlink_msg_esc_info_get_connection_type(&source);
  destination().info = mavlink_msg_esc_info_get_info(&source);
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::EscStatus>(const mavlink_message_t &source,
  Message<mavlink::msg::common::EscStatus> &destination, const void *) {
  destination().time_usec = mavlink_msg_esc_status_get_time_usec(&source);

  destination().rpm.resize(4);
  mavlink_msg_esc_status_get_rpm(&source, destination().rpm.data());

  destination().voltage.resize(4);
  mavlink_msg_esc_status_get_voltage(&source, destination().voltage.data());

  destination().current.resize(4);
  mavlink_msg_esc_status_get_current(&source, destination().current.data());

  destination().index = mavlink_msg_esc_status_get_index(&source);
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::Odometry>(const mavlink_message_t &source,
  Message<mavlink::msg::common::Odometry> &destination, const void *) {
  destination().x = mavlink_msg_odometry_get_x(&source);
  destination().y = mavlink_msg_odometry_get_y(&source);
  destination().z = mavlink_msg_odometry_get_z(&source);

  destination().q.resize(4);
  mavlink_msg_odometry_get_q(&source, destination().q.data());

  destination().vx = mavlink_msg_odometry_get_vx(&source);
  destination().vy = mavlink_msg_odometry_get_vy(&source);
  destination().vz = mavlink_msg_odometry_get_vz(&source);
  destination().rollspeed = mavlink_msg_odometry_get_rollspeed(&source);
  destination().pitchspeed = mavlink_msg_odometry_get_pitchspeed(&source);
  destination().yawspeed = mavlink_msg_odometry_get_yawspeed(&source);

  destination().pose_covariance.resize(21);
  mavlink_msg_odometry_get_pose_covariance(&source, destination().pose_covariance.data());

  destination().velocity_covariance.resize(21);
  mavlink_msg_odometry_get_velocity_covariance(&source, destination().velocity_covariance.data());

  destination().frame_id = mavlink_msg_odometry_get_frame_id(&source);
  destination().child_frame_id = mavlink_msg_odometry_get_child_frame_id(&source);
  destination().reset_counter = mavlink_msg_odometry_get_reset_counter(&source);
  destination().estimator_type = mavlink_msg_odometry_get_estimator_type(&source);
  destination().quality = mavlink_msg_odometry_get_quality(&source);
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::UtmGlobalPosition>(const mavlink_message_t &source,
  Message<mavlink::msg::common::UtmGlobalPosition> &destination, const void *) {
  destination().time = mavlink_msg_utm_global_position_get_time(&source);
  destination().lat = mavlink_msg_utm_global_position_get_lat(&source);
  destination().lon = mavlink_msg_utm_global_position_get_lon(&source);
  destination().alt = mavlink_msg_utm_global_position_get_alt(&source);
  destination().relative_alt = mavlink_msg_utm_global_position_get_relative_alt(&source);
  destination().next_lat = mavlink_msg_utm_global_position_get_next_lat(&source);
  destination().next_lon = mavlink_msg_utm_global_position_get_next_lon(&source);
  destination().next_alt = mavlink_msg_utm_global_position_get_next_alt(&source);
  destination().vx = mavlink_msg_utm_global_position_get_vx(&source);
  destination().vy = mavlink_msg_utm_global_position_get_vy(&source);
  destination().vz = mavlink_msg_utm_global_position_get_vz(&source);
  destination().h_acc = mavlink_msg_utm_global_position_get_h_acc(&source);
  destination().v_acc = mavlink_msg_utm_global_position_get_v_acc(&source);
  destination().vel_acc = mavlink_msg_utm_global_position_get_vel_acc(&source);
  destination().update_rate = mavlink_msg_utm_global_position_get_update_rate(&source);

  destination().uas_id.resize(18);
  mavlink_msg_utm_global_position_get_uas_id(&source, destination().uas_id.data());

  destination().flight_state = mavlink_msg_utm_global_position_get_flight_state(&source);
  destination().flags = mavlink_msg_utm_global_position_get_flags(&source);
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::TimeEstimateToTarget>(const mavlink_message_t &source,
  Message<mavlink::msg::common::TimeEstimateToTarget> &destination, const void *) {
  destination().safe_return = mavlink_msg_time_estimate_to_target_get_safe_return(&source);
  destination().land = mavlink_msg_time_estimate_to_target_get_land(&source);
  destination().mission_next_item = mavlink_msg_time_estimate_to_target_get_mission_next_item(&source);
  destination().mission_end = mavlink_msg_time_estimate_to_target_get_mission_end(&source);
  destination().commanded_action = mavlink_msg_time_estimate_to_target_get_commanded_action(&source);
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::CurrentEventSequence>(const mavlink_message_t &source,
  Message<mavlink::msg::common::CurrentEventSequence> &destination, const void *) {
  destination().sequence = mavlink_msg_current_event_sequence_get_sequence(&source);
  destination().flags = mavlink_msg_current_event_sequence_get_flags(&source);
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::OpenDroneIdLocation>(const mavlink_message_t &source,
  Message<mavlink::msg::common::OpenDroneIdLocation> &destination, const void *) {
  destination().latitude = mavlink_msg_open_drone_id_location_get_latitude(&source);
  destination().longitude = mavlink_msg_open_drone_id_location_get_longitude(&source);
  destination().altitude_barometric = mavlink_msg_open_drone_id_location_get_altitude_barometric(&source);
  destination().altitude_geodetic = mavlink_msg_open_drone_id_location_get_altitude_geodetic(&source);
  destination().height = mavlink_msg_open_drone_id_location_get_height(&source);
  destination().timestamp = mavlink_msg_open_drone_id_location_get_timestamp(&source);
  destination().direction = mavlink_msg_open_drone_id_location_get_direction(&source);
  destination().speed_horizontal = mavlink_msg_open_drone_id_location_get_speed_horizontal(&source);
  destination().speed_vertical = mavlink_msg_open_drone_id_location_get_speed_vertical(&source);
  destination().target_system = mavlink_msg_open_drone_id_location_get_target_system(&source);
  destination().target_component = mavlink_msg_open_drone_id_location_get_target_component(&source);

  destination().id_or_mac.resize(20);
  mavlink_msg_open_drone_id_location_get_id_or_mac(&source, destination().id_or_mac.data());

  destination().status = mavlink_msg_open_drone_id_location_get_status(&source);
  destination().height_reference = mavlink_msg_open_drone_id_location_get_height_reference(&source);
  destination().horizontal_accuracy = mavlink_msg_open_drone_id_location_get_horizontal_accuracy(&source);
  destination().vertical_accuracy = mavlink_msg_open_drone_id_location_get_vertical_accuracy(&source);
  destination().barometer_accuracy = mavlink_msg_open_drone_id_location_get_barometer_accuracy(&source);
  destination().speed_accuracy = mavlink_msg_open_drone_id_location_get_speed_accuracy(&source);
  destination().timestamp_accuracy = mavlink_msg_open_drone_id_location_get_timestamp_accuracy(&source);
}

template <>
void MavlinkNodePrivate::MavlinkSender::convert<mavlink::msg::common::OpenDroneIdSystem>(const mavlink_message_t &source,
  Message<mavlink::msg::common::OpenDroneIdSystem> &destination, const void *) {
  destination().operator_latitude = mavlink_msg_open_drone_id_system_get_operator_latitude(&source);
  destination().operator_longitude = mavlink_msg_open_drone_id_system_get_operator_longitude(&source);
  destination().area_ceiling = mavlink_msg_open_drone_id_system_get_area_ceiling(&source);
  destination().area_floor = mavlink_msg_open_drone_id_system_get_area_floor(&source);
  destination().operator_altitude_geo = mavlink_msg_open_drone_id_system_get_operator_altitude_geo(&source);
  destination().timestamp = mavlink_msg_open_drone_id_system_get_timestamp(&source);
  destination().area_count = mavlink_msg_open_drone_id_system_get_area_count(&source);
  destination().area_radius = mavlink_msg_open_drone_id_system_get_area_radius(&source);
  destination().target_system = mavlink_msg_open_drone_id_system_get_target_system(&source);
  destination().target_component = mavlink_msg_open_drone_id_system_get_target_component(&source);

  destination().id_or_mac.resize(20);
  mavlink_msg_open_drone_id_system_get_id_or_mac(&source, destination().id_or_mac.data());

  destination().operator_location_type = mavlink_msg_open_drone_id_system_get_operator_location_type(&source);
  destination().classification_type = mavlink_msg_open_drone_id_system_get_classification_type(&source);
  destination().category_eu = mavlink_msg_open_drone_id_system_get_category_eu(&source);
  destination().class_eu = mavlink_msg_open_drone_id_system_get_class_eu(&source);
}

template <>
void MavlinkNodePrivate::MavlinkReceiver::convert<mavlink::msg::common::ParamRequestRead>(
  const Message<mavlink::msg::common::ParamRequestRead> &source, mavlink_message_t &destination, const MavlinkNode::SystemInfo *info) {
  if (source().param_id.size() > 16) {
    throw ConversionException("The parameter ID string must not be larger than 16 characters.");
  }

  mavlink_msg_param_request_read_pack_chan(info->system_id, info->component_id, info->channel_id, &destination, source().target_system,
    source().target_component, source().param_id.c_str(), source().param_index);
}

template <>
void MavlinkNodePrivate::MavlinkReceiver::convert<mavlink::msg::common::SetPositionTargetLocalNed>(
  const Message<mavlink::msg::common::SetPositionTargetLocalNed> &source, mavlink_message_t &destination,
  const MavlinkNode::SystemInfo *info) {
  mavlink_msg_set_position_target_local_ned_pack_chan(info->system_id, info->component_id, info->channel_id, &destination,
    source().time_boot_ms, source().target_system, source().target_component, source().coordinate_frame, source().type_mask, source().x,
    source().y, source().z, source().vx, source().vy, source().vz, source().afx, source().afy, source().afz, source().yaw,
    source().yaw_rate);
}

template <>
void MavlinkNodePrivate::MavlinkReceiver::convert<mavlink::msg::common::CommandInt>(const Message<mavlink::msg::common::CommandInt> &source,
  mavlink_message_t &destination, const MavlinkNode::SystemInfo *info) {
  mavlink_msg_command_int_pack_chan(info->system_id, info->component_id, info->channel_id, &destination, source().target_system,
    source().target_component, source().frame, source().command, source().current, source().autocontinue, source().param1, source().param2,
    source().param3, source().param4, source().x, source().y, source().z);
}

template <>
void MavlinkNodePrivate::MavlinkReceiver::convert<mavlink::msg::common::CommandLong>(
  const Message<mavlink::msg::common::CommandLong> &source, mavlink_message_t &destination, const MavlinkNode::SystemInfo *info) {
  mavlink_msg_command_long_pack_chan(info->system_id, info->component_id, info->channel_id, &destination, source().target_system,
    source().target_component, source().command, source().confirmation, source().param1, source().param2, source().param3, source().param4,
    source().param5, source().param6, source().param7);
}

template <>
Message<mavlink::msg::common::ParamValue>
MavlinkNodePrivate::MavlinkServer::handle<mavlink::msg::common::ParamRequestRead, mavlink::msg::common::ParamValue>(
  const Message<mavlink::msg::common::ParamRequestRead> &request,
  ServerInfo<mavlink::msg::common::ParamRequestRead, mavlink::msg::common::ParamValue> *info) {
  Message<mavlink::msg::common::ParamValue> result;

  do {
    info->sender->put(request);

    try {
      result = info->receiver->next();

    } catch (TopicNoDataAvailableException &) {
      throw ServiceUnavailableException("MAVLink parameter request failed due to flushed topic.", info->node->getLogger());
    }
  } while (result().param_id != request().param_id);

  return result;
}

template <>
Message<mavlink::msg::common::CommandAck>
MavlinkNodePrivate::MavlinkServer::handle<mavlink::msg::common::CommandInt, mavlink::msg::common::CommandAck>(
  const Message<mavlink::msg::common::CommandInt> &request,
  ServerInfo<mavlink::msg::common::CommandInt, mavlink::msg::common::CommandAck> *info) {
  Message<mavlink::msg::common::CommandAck> result;

  do {
    info->sender->put(request);

    try {
      result = info->receiver->next();

    } catch (TopicNoDataAvailableException &) {
      throw ServiceUnavailableException("MAVLink command failed due to flushed topic.", info->node->getLogger());
    }
  } while (result().command != request().command);

  return result;
}

template <>
Message<mavlink::msg::common::CommandAck>
MavlinkNodePrivate::MavlinkServer::handle<mavlink::msg::common::CommandLong, mavlink::msg::common::CommandAck>(
  const Message<mavlink::msg::common::CommandLong> &request,
  ServerInfo<mavlink::msg::common::CommandLong, mavlink::msg::common::CommandAck> *info) {
  Message<mavlink::msg::common::CommandAck> result;

  do {
    info->sender->put(request);

    try {
      result = info->receiver->next();

    } catch (TopicNoDataAvailableException &) {
      throw ServiceUnavailableException("MAVLink command failed due to flushed topic.", info->node->getLogger());
    }
  } while (result().command != request().command);

  return result;
}

MavlinkNode::MavlinkNode(const Node::Environment &environment, MavlinkConnection::Ptr &&connection) : Node(environment) {
  std::allocator<MavlinkNodePrivate> allocator;
  priv = allocator.allocate(1);

  priv = std::construct_at(priv, std::forward<MavlinkConnection::Ptr>(connection), *this);
}

MavlinkNode::~MavlinkNode() {
  delete priv;
}

const MavlinkNode::SystemInfo &MavlinkNode::getSystemInfo() const {
  return priv->system_info;
}

void MavlinkNode::registerGenericSender(Node::GenericSender<mavlink_message_t>::Ptr &&sender, u16 id) {
  if (!priv->sender.map.try_emplace(id, std::forward<Node::GenericSender<mavlink_message_t>::Ptr>(sender)).second) {
    throw ManagementException("A sender has already been registered for the MAVLink message ID '" + std::to_string(id) + "'.");
  }
}

void MavlinkNode::registerGenericReceiver(Node::GenericReceiver<mavlink_message_t>::Ptr &&receiver) {
  priv->receiver.vector.emplace_back(std::forward<Node::GenericReceiver<mavlink_message_t>::Ptr>(receiver));
}

void MavlinkNode::receiverCallback(Node::GenericReceiver<mavlink_message_t> &receiver, MavlinkNodePrivate *node) {
  utils::CleanupLock::Guard guard = node->lock.lock();

  if (!guard.valid()) {
    return;
  }
  
  node->writeMessage(receiver.latest());
}

MavlinkNodePrivate::MavlinkNodePrivate(MavlinkConnection::Ptr &&connection, MavlinkNode &node) :
  connection(std::forward<MavlinkConnection::Ptr>(connection)), node(node) {
  system_info.channel_id = MAVLINK_COMM_0;
  system_info.system_id = 0xf0;
  system_info.component_id = MAV_COMP_ID_ONBOARD_COMPUTER;

  addSender<mavlink::msg::common::Heartbeat>("/mavlink/in/heartbeat", MAVLINK_MSG_ID_HEARTBEAT);
  addSender<mavlink::msg::common::SysStatus>("/mavlink/in/sys_status", MAVLINK_MSG_ID_SYS_STATUS);
  addSender<mavlink::msg::common::SystemTime>("/mavlink/in/system_time", MAVLINK_MSG_ID_SYSTEM_TIME);
  addSender<mavlink::msg::common::Ping>("/mavlink/in/ping", MAVLINK_MSG_ID_PING);
  addSender<mavlink::msg::common::LinkNodeStatus>("/mavlink/in/link_node_status", MAVLINK_MSG_ID_LINK_NODE_STATUS);
  addSender<mavlink::msg::common::ParamValue>("/mavlink/in/param_value", MAVLINK_MSG_ID_PARAM_VALUE);
  addSender<mavlink::msg::common::ParamSet>("/mavlink/in/param_set", MAVLINK_MSG_ID_PARAM_SET);
  addSender<mavlink::msg::common::GpsRawInt>("/mavlink/in/gps_raw_int", MAVLINK_MSG_ID_GPS_RAW_INT);
  addSender<mavlink::msg::common::GpsStatus>("/mavlink/in/gps_status", MAVLINK_MSG_ID_GPS_STATUS);
  addSender<mavlink::msg::common::ScaledImu>("/mavlink/in/scaled_imu", MAVLINK_MSG_ID_SCALED_IMU);
  addSender<mavlink::msg::common::RawImu>("/mavlink/in/raw_imu", MAVLINK_MSG_ID_RAW_IMU);
  addSender<mavlink::msg::common::ScaledPressure>("/mavlink/in/scaled_pressure", MAVLINK_MSG_ID_SCALED_PRESSURE);
  addSender<mavlink::msg::common::Attitude>("/mavlink/in/attitude", MAVLINK_MSG_ID_ATTITUDE);
  addSender<mavlink::msg::common::AttitudeQuaternion>("/mavlink/in/attitude_quaternion", MAVLINK_MSG_ID_ATTITUDE_QUATERNION);
  addSender<mavlink::msg::common::LocalPositionNed>("/mavlink/in/local_position_ned", MAVLINK_MSG_ID_LOCAL_POSITION_NED);
  addSender<mavlink::msg::common::GlobalPositionInt>("/mavlink/in/global_position_int", MAVLINK_MSG_ID_GLOBAL_POSITION_INT);
  addSender<mavlink::msg::common::RcChannels>("/mavlink/in/rc_channels", MAVLINK_MSG_ID_RC_CHANNELS);
  addSender<mavlink::msg::common::RcChannelsRaw>("/mavlink/in/rc_channels_raw", MAVLINK_MSG_ID_RC_CHANNELS_RAW);
  addSender<mavlink::msg::common::ServoOutputRaw>("/mavlink/in/servo_output_raw", MAVLINK_MSG_ID_SERVO_OUTPUT_RAW);
  addSender<mavlink::msg::common::VfrHud>("/mavlink/in/vfr_hud", MAVLINK_MSG_ID_VFR_HUD);
  addSender<mavlink::msg::common::CommandInt>("/mavlink/in/command_int", MAVLINK_MSG_ID_COMMAND_INT);
  addSender<mavlink::msg::common::CommandLong>("/mavlink/in/command_long", MAVLINK_MSG_ID_COMMAND_LONG);
  addSender<mavlink::msg::common::CommandAck>("/mavlink/in/command_ack", MAVLINK_MSG_ID_COMMAND_ACK);
  addSender<mavlink::msg::common::PositionTargetLocalNed>("/mavlink/in/position_target_local_ned",
    MAVLINK_MSG_ID_POSITION_TARGET_LOCAL_NED);
  addSender<mavlink::msg::common::AttitudeTarget>("/mavlink/in/attitude_target", MAVLINK_MSG_ID_ATTITUDE_TARGET);
  addSender<mavlink::msg::common::PositionTargetGlobalInt>("/mavlink/in/position_target_global_int",
    MAVLINK_MSG_ID_POSITION_TARGET_GLOBAL_INT);
  addSender<mavlink::msg::common::LocalPositionNedSystemGlobalOffset>(
    "/mavlink/in/local_position_ned_system_global_offset", MAVLINK_MSG_ID_LOCAL_POSITION_NED_SYSTEM_GLOBAL_OFFSET);
  addSender<mavlink::msg::common::HighresImu>("/mavlink/in/highres_imu", MAVLINK_MSG_ID_HIGHRES_IMU);
  addSender<mavlink::msg::common::Timesync>("/mavlink/in/timesync", MAVLINK_MSG_ID_TIMESYNC);
  addSender<mavlink::msg::common::ActuatorControlTarget>("/mavlink/in/actuator_control_target", MAVLINK_MSG_ID_ACTUATOR_CONTROL_TARGET);
  addSender<mavlink::msg::common::Altitude>("/mavlink/in/altitude", MAVLINK_MSG_ID_ALTITUDE);
  addSender<mavlink::msg::common::BatteryStatus>("/mavlink/in/battery_status", MAVLINK_MSG_ID_BATTERY_STATUS);
  addSender<mavlink::msg::common::AutopilotVersion>("/mavlink/in/autopilot_version", MAVLINK_MSG_ID_AUTOPILOT_VERSION);
  addSender<mavlink::msg::common::EstimatorStatus>("/mavlink/in/estimator_status", MAVLINK_MSG_ID_ESTIMATOR_STATUS);
  addSender<mavlink::msg::common::Vibration>("/mavlink/in/vibration", MAVLINK_MSG_ID_VIBRATION);
  addSender<mavlink::msg::common::HomePosition>("/mavlink/in/home_position", MAVLINK_MSG_ID_HOME_POSITION);
  addSender<mavlink::msg::common::ExtendedSysState>("/mavlink/in/extended_sys_state", MAVLINK_MSG_ID_EXTENDED_SYS_STATE);
  addSender<mavlink::msg::common::EscInfo>("/mavlink/in/esc_info", MAVLINK_MSG_ID_ESC_INFO);
  addSender<mavlink::msg::common::EscStatus>("/mavlink/in/esc_status", MAVLINK_MSG_ID_ESC_STATUS);
  addSender<mavlink::msg::common::Odometry>("/mavlink/in/odometry", MAVLINK_MSG_ID_ODOMETRY);
  addSender<mavlink::msg::common::UtmGlobalPosition>("/mavlink/in/utm_global_position", MAVLINK_MSG_ID_UTM_GLOBAL_POSITION);
  addSender<mavlink::msg::common::TimeEstimateToTarget>("/mavlink/in/time_estimate_to_target", MAVLINK_MSG_ID_TIME_ESTIMATE_TO_TARGET);
  addSender<mavlink::msg::common::CurrentEventSequence>("/mavlink/in/current_event_sequence", MAVLINK_MSG_ID_CURRENT_EVENT_SEQUENCE);
  addSender<mavlink::msg::common::OpenDroneIdLocation>("/mavlink/in/open_drone_id_location", MAVLINK_MSG_ID_OPEN_DRONE_ID_LOCATION);
  addSender<mavlink::msg::common::OpenDroneIdSystem>("/mavlink/in/open_drone_id_system", MAVLINK_MSG_ID_OPEN_DRONE_ID_SYSTEM);

  addReceiver<mavlink::msg::common::ParamRequestRead>("/mavlink/out/param_request_read");
  addReceiver<mavlink::msg::common::SetPositionTargetLocalNed>("/mavlink/out/set_position_target_local_ned");
  addReceiver<mavlink::msg::common::CommandInt>("/mavlink/out/command_int");
  addReceiver<mavlink::msg::common::CommandLong>("/mavlink/out/command_long");

  server.param_request_read = addServer<mavlink::msg::common::ParamRequestRead, mavlink::msg::common::ParamValue>(
    "/mavlink/srv/param_request_read", "/mavlink/out/param_request_read", "/mavlink/in/param_value");
  server.command_int = addServer<mavlink::msg::common::CommandInt, mavlink::msg::common::CommandAck>("/mavlink/srv/command_int",
    "/mavlink/out/command_int", "/mavlink/in/command_ack");
  server.command_long = addServer<mavlink::msg::common::CommandLong, mavlink::msg::common::CommandAck>("/mavlink/srv/command_long",
    "/mavlink/out/command_long", "/mavlink/in/command_ack");

  read_thread = utils::LoopThread(&MavlinkNodePrivate::readLoop, this);
  heartbeat_thread = utils::TimerThread(&MavlinkNodePrivate::heartbeatLoop, std::chrono::milliseconds(500), this);
}

MavlinkNodePrivate::~MavlinkNodePrivate() = default;

void MavlinkNodePrivate::readLoop() {
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

void MavlinkNodePrivate::heartbeatLoop() {
  mavlink_message_t message;
  mavlink_msg_heartbeat_pack_chan(system_info.system_id, system_info.component_id, system_info.channel_id, &message,
    MAV_TYPE_ONBOARD_CONTROLLER, MAV_AUTOPILOT_INVALID, 0, 0, MAV_STATE_ACTIVE);

  writeMessage(message);
}

void MavlinkNodePrivate::readMessage(const mavlink_message_t &message) {
  auto iterator = sender.map.find(message.msgid);

  if (iterator == sender.map.end()) {
    node.getLogger().logDebug() << "Received MAVLink message without handling implementation (ID: " << message.msgid << ").";
    return;
  }

  iterator->second->put(message);
}

void MavlinkNodePrivate::writeMessage(const mavlink_message_t &message) {
  std::array<u8, MAVLINK_MAX_PACKET_LEN> buffer;
  const std::size_t number_bytes = mavlink_msg_to_send_buffer(buffer.data(), &message);

  std::lock_guard guard(mutex);

  connection->write(buffer.data(), number_bytes);
}

}  // namespace labrat::robot::plugins
