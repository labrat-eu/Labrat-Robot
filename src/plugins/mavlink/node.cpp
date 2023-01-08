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
#include <labrat/robot/plugins/mavlink/msg/actuator_control_target.pb.h>
#include <labrat/robot/plugins/mavlink/msg/altitude.pb.h>
#include <labrat/robot/plugins/mavlink/msg/attitude.pb.h>
#include <labrat/robot/plugins/mavlink/msg/attitude_quaternion.pb.h>
#include <labrat/robot/plugins/mavlink/msg/attitude_target.pb.h>
#include <labrat/robot/plugins/mavlink/msg/autopilot_version.pb.h>
#include <labrat/robot/plugins/mavlink/msg/battery_status.pb.h>
#include <labrat/robot/plugins/mavlink/msg/command_ack.pb.h>
#include <labrat/robot/plugins/mavlink/msg/command_int.pb.h>
#include <labrat/robot/plugins/mavlink/msg/command_long.pb.h>
#include <labrat/robot/plugins/mavlink/msg/current_event_sequence.pb.h>
#include <labrat/robot/plugins/mavlink/msg/esc_info.pb.h>
#include <labrat/robot/plugins/mavlink/msg/esc_status.pb.h>
#include <labrat/robot/plugins/mavlink/msg/estimator_status.pb.h>
#include <labrat/robot/plugins/mavlink/msg/extended_sys_state.pb.h>
#include <labrat/robot/plugins/mavlink/msg/global_position_int.pb.h>
#include <labrat/robot/plugins/mavlink/msg/gps_raw_int.pb.h>
#include <labrat/robot/plugins/mavlink/msg/gps_status.pb.h>
#include <labrat/robot/plugins/mavlink/msg/heartbeat.pb.h>
#include <labrat/robot/plugins/mavlink/msg/highres_imu.pb.h>
#include <labrat/robot/plugins/mavlink/msg/home_position.pb.h>
#include <labrat/robot/plugins/mavlink/msg/link_node_status.pb.h>
#include <labrat/robot/plugins/mavlink/msg/local_position_ned.pb.h>
#include <labrat/robot/plugins/mavlink/msg/local_position_ned_system_global_offset.pb.h>
#include <labrat/robot/plugins/mavlink/msg/odometry.pb.h>
#include <labrat/robot/plugins/mavlink/msg/open_drone_id_location.pb.h>
#include <labrat/robot/plugins/mavlink/msg/open_drone_id_system.pb.h>
#include <labrat/robot/plugins/mavlink/msg/param_request_read.pb.h>
#include <labrat/robot/plugins/mavlink/msg/param_set.pb.h>
#include <labrat/robot/plugins/mavlink/msg/param_value.pb.h>
#include <labrat/robot/plugins/mavlink/msg/ping.pb.h>
#include <labrat/robot/plugins/mavlink/msg/position_target_global_int.pb.h>
#include <labrat/robot/plugins/mavlink/msg/position_target_local_ned.pb.h>
#include <labrat/robot/plugins/mavlink/msg/raw_imu.pb.h>
#include <labrat/robot/plugins/mavlink/msg/rc_channels.pb.h>
#include <labrat/robot/plugins/mavlink/msg/rc_channels_raw.pb.h>
#include <labrat/robot/plugins/mavlink/msg/scaled_imu.pb.h>
#include <labrat/robot/plugins/mavlink/msg/scaled_pressure.pb.h>
#include <labrat/robot/plugins/mavlink/msg/servo_output_raw.pb.h>
#include <labrat/robot/plugins/mavlink/msg/set_position_target_local_ned.pb.h>
#include <labrat/robot/plugins/mavlink/msg/sys_status.pb.h>
#include <labrat/robot/plugins/mavlink/msg/system_time.pb.h>
#include <labrat/robot/plugins/mavlink/msg/time_estimate_to_target.pb.h>
#include <labrat/robot/plugins/mavlink/msg/timesync.pb.h>
#include <labrat/robot/plugins/mavlink/msg/utm_global_position.pb.h>
#include <labrat/robot/plugins/mavlink/msg/vfr_hud.pb.h>
#include <labrat/robot/plugins/mavlink/msg/vibration.pb.h>
#include <labrat/robot/plugins/mavlink/node.hpp>

#include <array>

#include <mavlink/common/mavlink.h>

namespace labrat::robot::plugins {

struct MavlinkSender {
  template <typename T>
  static void convert(const mavlink_message_t &source, Message<T> &destination, const void *);

  template <>
  void convert<mavlink::msg::common::Heartbeat>(const mavlink_message_t &source, Message<mavlink::msg::common::Heartbeat> &destination,
    const void *) {
    destination().set_type(mavlink_msg_heartbeat_get_type(&source));
    destination().set_autopilot(mavlink_msg_heartbeat_get_autopilot(&source));
    destination().set_base_mode(mavlink_msg_heartbeat_get_base_mode(&source));
    destination().set_custom_mode(mavlink_msg_heartbeat_get_custom_mode(&source));
    destination().set_system_status(mavlink_msg_heartbeat_get_system_status(&source));
    destination().set_mavlink_version(mavlink_msg_heartbeat_get_mavlink_version(&source));
  }

  template <>
  void convert<mavlink::msg::common::SysStatus>(const mavlink_message_t &source, Message<mavlink::msg::common::SysStatus> &destination,
    const void *) {
    destination().set_onboard_control_sensors_present(mavlink_msg_sys_status_get_onboard_control_sensors_present(&source));
    destination().set_onboard_control_sensors_enabled(mavlink_msg_sys_status_get_onboard_control_sensors_enabled(&source));
    destination().set_onboard_control_sensors_health(mavlink_msg_sys_status_get_onboard_control_sensors_health(&source));
    destination().set_load(mavlink_msg_sys_status_get_load(&source));
    destination().set_voltage_battery(mavlink_msg_sys_status_get_voltage_battery(&source));
    destination().set_current_battery(mavlink_msg_sys_status_get_current_battery(&source));
    destination().set_drop_rate_comm(mavlink_msg_sys_status_get_drop_rate_comm(&source));
    destination().set_errors_comm(mavlink_msg_sys_status_get_errors_comm(&source));
    destination().set_errors_count1(mavlink_msg_sys_status_get_errors_count1(&source));
    destination().set_errors_count2(mavlink_msg_sys_status_get_errors_count2(&source));
    destination().set_errors_count3(mavlink_msg_sys_status_get_errors_count3(&source));
    destination().set_errors_count4(mavlink_msg_sys_status_get_errors_count4(&source));
    destination().set_battery_remaining(mavlink_msg_sys_status_get_battery_remaining(&source));
    destination().set_onboard_control_sensors_present_extended(
      mavlink_msg_sys_status_get_onboard_control_sensors_present_extended(&source));
    destination().set_onboard_control_sensors_enabled_extended(
      mavlink_msg_sys_status_get_onboard_control_sensors_enabled_extended(&source));
    destination().set_onboard_control_sensors_health_extended(mavlink_msg_sys_status_get_onboard_control_sensors_health_extended(&source));
  }

  template <>
  void convert<mavlink::msg::common::SystemTime>(const mavlink_message_t &source, Message<mavlink::msg::common::SystemTime> &destination,
    const void *) {
    destination().set_time_unix_usec(mavlink_msg_system_time_get_time_unix_usec(&source));
    destination().set_time_boot_ms(mavlink_msg_system_time_get_time_boot_ms(&source));
  }

  template <>
  void convert<mavlink::msg::common::Ping>(const mavlink_message_t &source, Message<mavlink::msg::common::Ping> &destination,
    const void *) {
    destination().set_time_usec(mavlink_msg_ping_get_time_usec(&source));
    destination().set_seq(mavlink_msg_ping_get_seq(&source));
    destination().set_target_system(mavlink_msg_ping_get_target_system(&source));
    destination().set_target_component(mavlink_msg_ping_get_target_component(&source));
  }

  template <>
  void convert<mavlink::msg::common::LinkNodeStatus>(const mavlink_message_t &source,
    Message<mavlink::msg::common::LinkNodeStatus> &destination, const void *) {
    destination().set_timestamp(mavlink_msg_link_node_status_get_timestamp(&source));
    destination().set_tx_rate(mavlink_msg_link_node_status_get_tx_rate(&source));
    destination().set_rx_rate(mavlink_msg_link_node_status_get_rx_rate(&source));
    destination().set_messages_sent(mavlink_msg_link_node_status_get_messages_sent(&source));
    destination().set_messages_received(mavlink_msg_link_node_status_get_messages_received(&source));
    destination().set_messages_lost(mavlink_msg_link_node_status_get_messages_lost(&source));
    destination().set_rx_parse_err(mavlink_msg_link_node_status_get_rx_parse_err(&source));
    destination().set_tx_overflows(mavlink_msg_link_node_status_get_tx_overflows(&source));
    destination().set_rx_overflows(mavlink_msg_link_node_status_get_rx_overflows(&source));
    destination().set_tx_buf(mavlink_msg_link_node_status_get_tx_buf(&source));
    destination().set_rx_buf(mavlink_msg_link_node_status_get_rx_buf(&source));
  }

  template <>
  void convert<mavlink::msg::common::ParamValue>(const mavlink_message_t &source, Message<mavlink::msg::common::ParamValue> &destination,
    const void *) {
    destination().set_param_value(mavlink_msg_param_value_get_param_value(&source));
    destination().set_param_count(mavlink_msg_param_value_get_param_count(&source));
    destination().set_param_index(mavlink_msg_param_value_get_param_index(&source));

    std::array<char, 17> param_id;
    mavlink_msg_param_value_get_param_id(&source, param_id.data());
    destination().set_param_id(param_id.data());

    destination().set_param_type(mavlink_msg_param_value_get_param_type(&source));
  }

  template <>
  void convert<mavlink::msg::common::ParamSet>(const mavlink_message_t &source, Message<mavlink::msg::common::ParamSet> &destination,
    const void *) {
    destination().set_param_value(mavlink_msg_param_set_get_param_value(&source));
    destination().set_target_system(mavlink_msg_param_set_get_target_system(&source));
    destination().set_target_component(mavlink_msg_param_set_get_target_component(&source));

    std::array<char, 17> param_id;
    mavlink_msg_param_set_get_param_id(&source, param_id.data());
    destination().set_param_id(param_id.data());

    destination().set_param_type(mavlink_msg_param_set_get_param_type(&source));
  }

  template <>
  void convert<mavlink::msg::common::GpsRawInt>(const mavlink_message_t &source, Message<mavlink::msg::common::GpsRawInt> &destination,
    const void *) {
    destination().set_time_usec(mavlink_msg_gps_raw_int_get_time_usec(&source));
    destination().set_lat(mavlink_msg_gps_raw_int_get_lat(&source));
    destination().set_lon(mavlink_msg_gps_raw_int_get_lon(&source));
    destination().set_alt(mavlink_msg_gps_raw_int_get_alt(&source));
    destination().set_eph(mavlink_msg_gps_raw_int_get_eph(&source));
    destination().set_epv(mavlink_msg_gps_raw_int_get_epv(&source));
    destination().set_vel(mavlink_msg_gps_raw_int_get_vel(&source));
    destination().set_cog(mavlink_msg_gps_raw_int_get_cog(&source));
    destination().set_fix_type(mavlink_msg_gps_raw_int_get_fix_type(&source));
    destination().set_satellites_visible(mavlink_msg_gps_raw_int_get_satellites_visible(&source));
    destination().set_alt_ellipsoid(mavlink_msg_gps_raw_int_get_alt_ellipsoid(&source));
    destination().set_h_acc(mavlink_msg_gps_raw_int_get_h_acc(&source));
    destination().set_v_acc(mavlink_msg_gps_raw_int_get_v_acc(&source));
    destination().set_vel_acc(mavlink_msg_gps_raw_int_get_vel_acc(&source));
    destination().set_hdg_acc(mavlink_msg_gps_raw_int_get_hdg_acc(&source));
    destination().set_yaw(mavlink_msg_gps_raw_int_get_yaw(&source));
  }

  template <>
  void convert<mavlink::msg::common::GpsStatus>(const mavlink_message_t &source, Message<mavlink::msg::common::GpsStatus> &destination,
    const void *) {
    destination().set_satellites_visible(mavlink_msg_gps_status_get_satellites_visible(&source));

    std::array<u8, 20> satellite_prn;
    mavlink_msg_gps_status_get_satellite_prn(&source, satellite_prn.data());
    destination().set_satellite_prn(satellite_prn.data(), satellite_prn.size());

    std::array<u8, 20> satellite_used;
    mavlink_msg_gps_status_get_satellite_used(&source, satellite_used.data());
    destination().set_satellite_used(satellite_used.data(), satellite_used.size());

    std::array<u8, 20> satellite_elevation;
    mavlink_msg_gps_status_get_satellite_elevation(&source, satellite_elevation.data());
    destination().set_satellite_elevation(satellite_elevation.data(), satellite_elevation.size());

    std::array<u8, 20> satellite_azimuth;
    mavlink_msg_gps_status_get_satellite_azimuth(&source, satellite_azimuth.data());
    destination().set_satellite_azimuth(satellite_azimuth.data(), satellite_azimuth.size());

    std::array<u8, 20> satellite_snr;
    mavlink_msg_gps_status_get_satellite_snr(&source, satellite_snr.data());
    destination().set_satellite_snr(satellite_snr.data(), satellite_snr.size());
  }

  template <>
  void convert<mavlink::msg::common::ScaledImu>(const mavlink_message_t &source, Message<mavlink::msg::common::ScaledImu> &destination,
    const void *) {
    destination().set_time_boot_ms(mavlink_msg_scaled_imu_get_time_boot_ms(&source));
    destination().set_xacc(mavlink_msg_scaled_imu_get_xacc(&source));
    destination().set_yacc(mavlink_msg_scaled_imu_get_yacc(&source));
    destination().set_zacc(mavlink_msg_scaled_imu_get_zacc(&source));
    destination().set_xgyro(mavlink_msg_scaled_imu_get_xgyro(&source));
    destination().set_ygyro(mavlink_msg_scaled_imu_get_ygyro(&source));
    destination().set_zgyro(mavlink_msg_scaled_imu_get_zgyro(&source));
    destination().set_xmag(mavlink_msg_scaled_imu_get_xmag(&source));
    destination().set_ymag(mavlink_msg_scaled_imu_get_ymag(&source));
    destination().set_zmag(mavlink_msg_scaled_imu_get_zmag(&source));
    destination().set_temperature(mavlink_msg_scaled_imu_get_temperature(&source));
  }

  template <>
  void convert<mavlink::msg::common::RawImu>(const mavlink_message_t &source, Message<mavlink::msg::common::RawImu> &destination,
    const void *) {
    destination().set_time_usec(mavlink_msg_raw_imu_get_time_usec(&source));
    destination().set_xacc(mavlink_msg_raw_imu_get_xacc(&source));
    destination().set_yacc(mavlink_msg_raw_imu_get_yacc(&source));
    destination().set_zacc(mavlink_msg_raw_imu_get_zacc(&source));
    destination().set_xgyro(mavlink_msg_raw_imu_get_xgyro(&source));
    destination().set_ygyro(mavlink_msg_raw_imu_get_ygyro(&source));
    destination().set_zgyro(mavlink_msg_raw_imu_get_zgyro(&source));
    destination().set_xmag(mavlink_msg_raw_imu_get_xmag(&source));
    destination().set_ymag(mavlink_msg_raw_imu_get_ymag(&source));
    destination().set_zmag(mavlink_msg_raw_imu_get_zmag(&source));
    destination().set_id(mavlink_msg_raw_imu_get_id(&source));
    destination().set_temperature(mavlink_msg_raw_imu_get_temperature(&source));
  }

  template <>
  void convert<mavlink::msg::common::ScaledPressure>(const mavlink_message_t &source,
    Message<mavlink::msg::common::ScaledPressure> &destination, const void *) {
    destination().set_time_boot_ms(mavlink_msg_scaled_pressure_get_time_boot_ms(&source));
    destination().set_press_abs(mavlink_msg_scaled_pressure_get_press_abs(&source));
    destination().set_press_diff(mavlink_msg_scaled_pressure_get_press_diff(&source));
    destination().set_temperature(mavlink_msg_scaled_pressure_get_temperature(&source));
    destination().set_temperature_press_diff(mavlink_msg_scaled_pressure_get_temperature_press_diff(&source));
  }

  template <>
  void convert<mavlink::msg::common::Attitude>(const mavlink_message_t &source, Message<mavlink::msg::common::Attitude> &destination,
    const void *) {
    destination().set_time_boot_ms(mavlink_msg_attitude_get_time_boot_ms(&source));
    destination().set_roll(mavlink_msg_attitude_get_roll(&source));
    destination().set_pitch(mavlink_msg_attitude_get_pitch(&source));
    destination().set_yaw(mavlink_msg_attitude_get_yaw(&source));
    destination().set_rollspeed(mavlink_msg_attitude_get_rollspeed(&source));
    destination().set_pitchspeed(mavlink_msg_attitude_get_pitchspeed(&source));
    destination().set_yawspeed(mavlink_msg_attitude_get_yawspeed(&source));
  }

  template <>
  void convert<mavlink::msg::common::AttitudeQuaternion>(const mavlink_message_t &source,
    Message<mavlink::msg::common::AttitudeQuaternion> &destination, const void *) {
    destination().set_time_boot_ms(mavlink_msg_attitude_quaternion_get_time_boot_ms(&source));
    destination().set_q1(mavlink_msg_attitude_quaternion_get_q1(&source));
    destination().set_q2(mavlink_msg_attitude_quaternion_get_q2(&source));
    destination().set_q3(mavlink_msg_attitude_quaternion_get_q3(&source));
    destination().set_q4(mavlink_msg_attitude_quaternion_get_q4(&source));
    destination().set_rollspeed(mavlink_msg_attitude_quaternion_get_rollspeed(&source));
    destination().set_pitchspeed(mavlink_msg_attitude_quaternion_get_pitchspeed(&source));
    destination().set_yawspeed(mavlink_msg_attitude_quaternion_get_yawspeed(&source));

    std::array<float, 4> repr_offset_q;
    mavlink_msg_attitude_quaternion_get_repr_offset_q(&source, repr_offset_q.data());
    for (const float value : repr_offset_q) {
      destination().add_repr_offset_q(value);
    }
  }

  template <>
  void convert<mavlink::msg::common::LocalPositionNed>(const mavlink_message_t &source,
    Message<mavlink::msg::common::LocalPositionNed> &destination, const void *) {
    destination().set_time_boot_ms(mavlink_msg_local_position_ned_get_time_boot_ms(&source));
    destination().set_x(mavlink_msg_local_position_ned_get_x(&source));
    destination().set_y(mavlink_msg_local_position_ned_get_y(&source));
    destination().set_z(mavlink_msg_local_position_ned_get_z(&source));
    destination().set_vx(mavlink_msg_local_position_ned_get_vx(&source));
    destination().set_vy(mavlink_msg_local_position_ned_get_vy(&source));
    destination().set_vz(mavlink_msg_local_position_ned_get_vz(&source));
  }

  template <>
  void convert<mavlink::msg::common::GlobalPositionInt>(const mavlink_message_t &source,
    Message<mavlink::msg::common::GlobalPositionInt> &destination, const void *) {
    destination().set_time_boot_ms(mavlink_msg_global_position_int_get_time_boot_ms(&source));
    destination().set_lat(mavlink_msg_global_position_int_get_lat(&source));
    destination().set_lon(mavlink_msg_global_position_int_get_lon(&source));
    destination().set_alt(mavlink_msg_global_position_int_get_alt(&source));
    destination().set_relative_alt(mavlink_msg_global_position_int_get_relative_alt(&source));
    destination().set_vx(mavlink_msg_global_position_int_get_vx(&source));
    destination().set_vy(mavlink_msg_global_position_int_get_vy(&source));
    destination().set_vz(mavlink_msg_global_position_int_get_vz(&source));
    destination().set_hdg(mavlink_msg_global_position_int_get_hdg(&source));
  }

  template <>
  void convert<mavlink::msg::common::RcChannelsRaw>(const mavlink_message_t &source,
    Message<mavlink::msg::common::RcChannelsRaw> &destination, const void *) {
    destination().set_time_boot_ms(mavlink_msg_rc_channels_raw_get_time_boot_ms(&source));
    destination().set_chan1_raw(mavlink_msg_rc_channels_raw_get_chan1_raw(&source));
    destination().set_chan2_raw(mavlink_msg_rc_channels_raw_get_chan2_raw(&source));
    destination().set_chan3_raw(mavlink_msg_rc_channels_raw_get_chan3_raw(&source));
    destination().set_chan4_raw(mavlink_msg_rc_channels_raw_get_chan4_raw(&source));
    destination().set_chan5_raw(mavlink_msg_rc_channels_raw_get_chan5_raw(&source));
    destination().set_chan6_raw(mavlink_msg_rc_channels_raw_get_chan6_raw(&source));
    destination().set_chan7_raw(mavlink_msg_rc_channels_raw_get_chan7_raw(&source));
    destination().set_chan8_raw(mavlink_msg_rc_channels_raw_get_chan8_raw(&source));
    destination().set_port(mavlink_msg_rc_channels_raw_get_port(&source));
    destination().set_rssi(mavlink_msg_rc_channels_raw_get_rssi(&source));
  }

  template <>
  void convert<mavlink::msg::common::RcChannels>(const mavlink_message_t &source, Message<mavlink::msg::common::RcChannels> &destination,
    const void *) {
    destination().set_time_boot_ms(mavlink_msg_rc_channels_get_time_boot_ms(&source));
    destination().set_chan1_raw(mavlink_msg_rc_channels_get_chan1_raw(&source));
    destination().set_chan2_raw(mavlink_msg_rc_channels_get_chan2_raw(&source));
    destination().set_chan3_raw(mavlink_msg_rc_channels_get_chan3_raw(&source));
    destination().set_chan4_raw(mavlink_msg_rc_channels_get_chan4_raw(&source));
    destination().set_chan5_raw(mavlink_msg_rc_channels_get_chan5_raw(&source));
    destination().set_chan6_raw(mavlink_msg_rc_channels_get_chan6_raw(&source));
    destination().set_chan7_raw(mavlink_msg_rc_channels_get_chan7_raw(&source));
    destination().set_chan8_raw(mavlink_msg_rc_channels_get_chan8_raw(&source));
    destination().set_chan9_raw(mavlink_msg_rc_channels_get_chan9_raw(&source));
    destination().set_chan10_raw(mavlink_msg_rc_channels_get_chan10_raw(&source));
    destination().set_chan11_raw(mavlink_msg_rc_channels_get_chan11_raw(&source));
    destination().set_chan12_raw(mavlink_msg_rc_channels_get_chan12_raw(&source));
    destination().set_chan13_raw(mavlink_msg_rc_channels_get_chan13_raw(&source));
    destination().set_chan14_raw(mavlink_msg_rc_channels_get_chan14_raw(&source));
    destination().set_chan15_raw(mavlink_msg_rc_channels_get_chan15_raw(&source));
    destination().set_chan16_raw(mavlink_msg_rc_channels_get_chan16_raw(&source));
    destination().set_chan17_raw(mavlink_msg_rc_channels_get_chan17_raw(&source));
    destination().set_chan18_raw(mavlink_msg_rc_channels_get_chan18_raw(&source));
    destination().set_chancount(mavlink_msg_rc_channels_get_chancount(&source));
    destination().set_rssi(mavlink_msg_rc_channels_get_rssi(&source));
  }

  template <>
  void convert<mavlink::msg::common::ServoOutputRaw>(const mavlink_message_t &source,
    Message<mavlink::msg::common::ServoOutputRaw> &destination, const void *) {
    destination().set_time_usec(mavlink_msg_servo_output_raw_get_time_usec(&source));
    destination().set_servo1_raw(mavlink_msg_servo_output_raw_get_servo1_raw(&source));
    destination().set_servo2_raw(mavlink_msg_servo_output_raw_get_servo2_raw(&source));
    destination().set_servo3_raw(mavlink_msg_servo_output_raw_get_servo3_raw(&source));
    destination().set_servo4_raw(mavlink_msg_servo_output_raw_get_servo4_raw(&source));
    destination().set_servo5_raw(mavlink_msg_servo_output_raw_get_servo5_raw(&source));
    destination().set_servo6_raw(mavlink_msg_servo_output_raw_get_servo6_raw(&source));
    destination().set_servo7_raw(mavlink_msg_servo_output_raw_get_servo7_raw(&source));
    destination().set_servo8_raw(mavlink_msg_servo_output_raw_get_servo8_raw(&source));
    destination().set_port(mavlink_msg_servo_output_raw_get_port(&source));
    destination().set_servo9_raw(mavlink_msg_servo_output_raw_get_servo9_raw(&source));
    destination().set_servo10_raw(mavlink_msg_servo_output_raw_get_servo10_raw(&source));
    destination().set_servo11_raw(mavlink_msg_servo_output_raw_get_servo11_raw(&source));
    destination().set_servo12_raw(mavlink_msg_servo_output_raw_get_servo12_raw(&source));
    destination().set_servo13_raw(mavlink_msg_servo_output_raw_get_servo13_raw(&source));
    destination().set_servo14_raw(mavlink_msg_servo_output_raw_get_servo14_raw(&source));
    destination().set_servo15_raw(mavlink_msg_servo_output_raw_get_servo15_raw(&source));
    destination().set_servo16_raw(mavlink_msg_servo_output_raw_get_servo16_raw(&source));
  }

  template <>
  void convert<mavlink::msg::common::VfrHud>(const mavlink_message_t &source, Message<mavlink::msg::common::VfrHud> &destination,
    const void *) {
    destination().set_airspeed(mavlink_msg_vfr_hud_get_airspeed(&source));
    destination().set_groundspeed(mavlink_msg_vfr_hud_get_groundspeed(&source));
    destination().set_alt(mavlink_msg_vfr_hud_get_alt(&source));
    destination().set_climb(mavlink_msg_vfr_hud_get_climb(&source));
    destination().set_heading(mavlink_msg_vfr_hud_get_heading(&source));
    destination().set_throttle(mavlink_msg_vfr_hud_get_throttle(&source));
  }

  template <>
  void convert<mavlink::msg::common::CommandInt>(const mavlink_message_t &source, Message<mavlink::msg::common::CommandInt> &destination,
    const void *) {
    destination().set_param1(mavlink_msg_command_int_get_param1(&source));
    destination().set_param2(mavlink_msg_command_int_get_param2(&source));
    destination().set_param3(mavlink_msg_command_int_get_param3(&source));
    destination().set_param4(mavlink_msg_command_int_get_param4(&source));
    destination().set_x(mavlink_msg_command_int_get_x(&source));
    destination().set_y(mavlink_msg_command_int_get_y(&source));
    destination().set_z(mavlink_msg_command_int_get_z(&source));
    destination().set_command(mavlink_msg_command_int_get_command(&source));
    destination().set_target_system(mavlink_msg_command_int_get_target_system(&source));
    destination().set_target_component(mavlink_msg_command_int_get_target_component(&source));
    destination().set_frame(mavlink_msg_command_int_get_frame(&source));
    destination().set_current(mavlink_msg_command_int_get_current(&source));
    destination().set_autocontinue(mavlink_msg_command_int_get_autocontinue(&source));
  }

  template <>
  void convert<mavlink::msg::common::CommandLong>(const mavlink_message_t &source, Message<mavlink::msg::common::CommandLong> &destination,
    const void *) {
    destination().set_param1(mavlink_msg_command_long_get_param1(&source));
    destination().set_param2(mavlink_msg_command_long_get_param2(&source));
    destination().set_param3(mavlink_msg_command_long_get_param3(&source));
    destination().set_param4(mavlink_msg_command_long_get_param4(&source));
    destination().set_param5(mavlink_msg_command_long_get_param5(&source));
    destination().set_param6(mavlink_msg_command_long_get_param6(&source));
    destination().set_param7(mavlink_msg_command_long_get_param7(&source));
    destination().set_command(mavlink_msg_command_long_get_command(&source));
    destination().set_target_system(mavlink_msg_command_long_get_target_system(&source));
    destination().set_target_component(mavlink_msg_command_long_get_target_component(&source));
    destination().set_confirmation(mavlink_msg_command_long_get_confirmation(&source));
  }

  template <>
  void convert<mavlink::msg::common::CommandAck>(const mavlink_message_t &source, Message<mavlink::msg::common::CommandAck> &destination,
    const void *) {
    destination().set_command(mavlink_msg_command_ack_get_command(&source));
    destination().set_result(mavlink_msg_command_ack_get_result(&source));
    destination().set_progress(mavlink_msg_command_ack_get_progress(&source));
    destination().set_result_param2(mavlink_msg_command_ack_get_result_param2(&source));
    destination().set_target_system(mavlink_msg_command_ack_get_target_system(&source));
    destination().set_target_component(mavlink_msg_command_ack_get_target_component(&source));
  }

  template <>
  void convert<mavlink::msg::common::PositionTargetLocalNed>(const mavlink_message_t &source,
    Message<mavlink::msg::common::PositionTargetLocalNed> &destination, const void *) {
    destination().set_time_boot_ms(mavlink_msg_position_target_local_ned_get_time_boot_ms(&source));
    destination().set_x(mavlink_msg_position_target_local_ned_get_x(&source));
    destination().set_y(mavlink_msg_position_target_local_ned_get_y(&source));
    destination().set_z(mavlink_msg_position_target_local_ned_get_z(&source));
    destination().set_vx(mavlink_msg_position_target_local_ned_get_vx(&source));
    destination().set_vy(mavlink_msg_position_target_local_ned_get_vy(&source));
    destination().set_vz(mavlink_msg_position_target_local_ned_get_vz(&source));
    destination().set_afx(mavlink_msg_position_target_local_ned_get_afx(&source));
    destination().set_afy(mavlink_msg_position_target_local_ned_get_afy(&source));
    destination().set_afz(mavlink_msg_position_target_local_ned_get_afz(&source));
    destination().set_yaw(mavlink_msg_position_target_local_ned_get_yaw(&source));
    destination().set_yaw_rate(mavlink_msg_position_target_local_ned_get_yaw_rate(&source));
    destination().set_type_mask(mavlink_msg_position_target_local_ned_get_type_mask(&source));
    destination().set_coordinate_frame(mavlink_msg_position_target_local_ned_get_coordinate_frame(&source));
  }

  template <>
  void convert<mavlink::msg::common::AttitudeTarget>(const mavlink_message_t &source,
    Message<mavlink::msg::common::AttitudeTarget> &destination, const void *) {
    destination().set_time_boot_ms(mavlink_msg_attitude_target_get_time_boot_ms(&source));

    std::array<float, 4> q;
    mavlink_msg_attitude_target_get_q(&source, q.data());
    for (const float value : q) {
      destination().add_q(value);
    }

    destination().set_body_roll_rate(mavlink_msg_attitude_target_get_body_roll_rate(&source));
    destination().set_body_pitch_rate(mavlink_msg_attitude_target_get_body_pitch_rate(&source));
    destination().set_body_yaw_rate(mavlink_msg_attitude_target_get_body_yaw_rate(&source));
    destination().set_thrust(mavlink_msg_attitude_target_get_thrust(&source));
    destination().set_type_mask(mavlink_msg_attitude_target_get_type_mask(&source));
  }

  template <>
  void convert<mavlink::msg::common::PositionTargetGlobalInt>(const mavlink_message_t &source,
    Message<mavlink::msg::common::PositionTargetGlobalInt> &destination, const void *) {
    destination().set_time_boot_ms(mavlink_msg_position_target_global_int_get_time_boot_ms(&source));
    destination().set_lat_int(mavlink_msg_position_target_global_int_get_lat_int(&source));
    destination().set_lon_int(mavlink_msg_position_target_global_int_get_lon_int(&source));
    destination().set_alt(mavlink_msg_position_target_global_int_get_alt(&source));
    destination().set_vx(mavlink_msg_position_target_global_int_get_vx(&source));
    destination().set_vy(mavlink_msg_position_target_global_int_get_vy(&source));
    destination().set_vz(mavlink_msg_position_target_global_int_get_vz(&source));
    destination().set_afx(mavlink_msg_position_target_global_int_get_afx(&source));
    destination().set_afy(mavlink_msg_position_target_global_int_get_afy(&source));
    destination().set_afz(mavlink_msg_position_target_global_int_get_afz(&source));
    destination().set_yaw(mavlink_msg_position_target_global_int_get_yaw(&source));
    destination().set_yaw_rate(mavlink_msg_position_target_global_int_get_yaw_rate(&source));
    destination().set_type_mask(mavlink_msg_position_target_global_int_get_type_mask(&source));
    destination().set_coordinate_frame(mavlink_msg_position_target_global_int_get_coordinate_frame(&source));
  }

  template <>
  void convert<mavlink::msg::common::LocalPositionNedSystemGlobalOffset>(const mavlink_message_t &source,
    Message<mavlink::msg::common::LocalPositionNedSystemGlobalOffset> &destination, const void *) {
    destination().set_time_boot_ms(mavlink_msg_local_position_ned_system_global_offset_get_time_boot_ms(&source));
    destination().set_x(mavlink_msg_local_position_ned_system_global_offset_get_x(&source));
    destination().set_y(mavlink_msg_local_position_ned_system_global_offset_get_y(&source));
    destination().set_z(mavlink_msg_local_position_ned_system_global_offset_get_z(&source));
    destination().set_roll(mavlink_msg_local_position_ned_system_global_offset_get_roll(&source));
    destination().set_pitch(mavlink_msg_local_position_ned_system_global_offset_get_pitch(&source));
    destination().set_yaw(mavlink_msg_local_position_ned_system_global_offset_get_yaw(&source));
  }

  template <>
  void convert<mavlink::msg::common::HighresImu>(const mavlink_message_t &source, Message<mavlink::msg::common::HighresImu> &destination,
    const void *) {
    destination().set_time_usec(mavlink_msg_highres_imu_get_time_usec(&source));
    destination().set_xacc(mavlink_msg_highres_imu_get_xacc(&source));
    destination().set_yacc(mavlink_msg_highres_imu_get_yacc(&source));
    destination().set_zacc(mavlink_msg_highres_imu_get_zacc(&source));
    destination().set_xgyro(mavlink_msg_highres_imu_get_xgyro(&source));
    destination().set_ygyro(mavlink_msg_highres_imu_get_ygyro(&source));
    destination().set_zgyro(mavlink_msg_highres_imu_get_zgyro(&source));
    destination().set_xmag(mavlink_msg_highres_imu_get_xmag(&source));
    destination().set_ymag(mavlink_msg_highres_imu_get_ymag(&source));
    destination().set_zmag(mavlink_msg_highres_imu_get_zmag(&source));
    destination().set_abs_pressure(mavlink_msg_highres_imu_get_abs_pressure(&source));
    destination().set_diff_pressure(mavlink_msg_highres_imu_get_diff_pressure(&source));
    destination().set_pressure_alt(mavlink_msg_highres_imu_get_pressure_alt(&source));
    destination().set_temperature(mavlink_msg_highres_imu_get_temperature(&source));
    destination().set_fields_updated(mavlink_msg_highres_imu_get_fields_updated(&source));
    destination().set_id(mavlink_msg_highres_imu_get_id(&source));
  }

  template <>
  void convert<mavlink::msg::common::Timesync>(const mavlink_message_t &source, Message<mavlink::msg::common::Timesync> &destination,
    const void *) {
    destination().set_tc1(mavlink_msg_timesync_get_tc1(&source));
    destination().set_ts1(mavlink_msg_timesync_get_ts1(&source));
    destination().set_target_system(mavlink_msg_timesync_get_target_system(&source));
    destination().set_target_component(mavlink_msg_timesync_get_target_component(&source));
  }

  template <>
  void convert<mavlink::msg::common::ActuatorControlTarget>(const mavlink_message_t &source,
    Message<mavlink::msg::common::ActuatorControlTarget> &destination, const void *) {
    destination().set_time_usec(mavlink_msg_actuator_control_target_get_time_usec(&source));

    std::array<float, 8> controls;
    mavlink_msg_actuator_control_target_get_controls(&source, controls.data());
    for (const float value : controls) {
      destination().add_controls(value);
    }

    destination().set_group_mlx(mavlink_msg_actuator_control_target_get_group_mlx(&source));
  }

  template <>
  void convert<mavlink::msg::common::Altitude>(const mavlink_message_t &source, Message<mavlink::msg::common::Altitude> &destination,
    const void *) {
    destination().set_time_usec(mavlink_msg_altitude_get_time_usec(&source));
    destination().set_altitude_monotonic(mavlink_msg_altitude_get_altitude_monotonic(&source));
    destination().set_altitude_amsl(mavlink_msg_altitude_get_altitude_amsl(&source));
    destination().set_altitude_local(mavlink_msg_altitude_get_altitude_local(&source));
    destination().set_altitude_relative(mavlink_msg_altitude_get_altitude_relative(&source));
    destination().set_altitude_terrain(mavlink_msg_altitude_get_altitude_terrain(&source));
    destination().set_bottom_clearance(mavlink_msg_altitude_get_bottom_clearance(&source));
  }

  template <>
  void convert<mavlink::msg::common::BatteryStatus>(const mavlink_message_t &source,
    Message<mavlink::msg::common::BatteryStatus> &destination, const void *) {
    destination().set_current_consumed(mavlink_msg_battery_status_get_current_consumed(&source));
    destination().set_energy_consumed(mavlink_msg_battery_status_get_energy_consumed(&source));
    destination().set_temperature(mavlink_msg_battery_status_get_temperature(&source));

    std::array<u16, 10> voltages;
    mavlink_msg_battery_status_get_voltages(&source, voltages.data());
    for (const u16 value : voltages) {
      destination().add_voltages(value);
    }

    destination().set_current_battery(mavlink_msg_battery_status_get_current_battery(&source));
    destination().set_id(mavlink_msg_battery_status_get_id(&source));
    destination().set_battery_function(mavlink_msg_battery_status_get_battery_function(&source));
    destination().set_type(mavlink_msg_battery_status_get_type(&source));
    destination().set_battery_remaining(mavlink_msg_battery_status_get_battery_remaining(&source));
    destination().set_time_remaining(mavlink_msg_battery_status_get_time_remaining(&source));
    destination().set_charge_state(mavlink_msg_battery_status_get_charge_state(&source));

    std::array<u16, 4> voltages_ext;
    mavlink_msg_battery_status_get_voltages_ext(&source, voltages_ext.data());
    for (const u16 value : voltages_ext) {
      destination().add_voltages_ext(value);
    }

    destination().set_mode(mavlink_msg_battery_status_get_mode(&source));
    destination().set_fault_bitmask(mavlink_msg_battery_status_get_fault_bitmask(&source));
  }

  template <>
  void convert<mavlink::msg::common::AutopilotVersion>(const mavlink_message_t &source,
    Message<mavlink::msg::common::AutopilotVersion> &destination, const void *) {
    destination().set_capabilities(mavlink_msg_autopilot_version_get_capabilities(&source));
    destination().set_uid(mavlink_msg_autopilot_version_get_uid(&source));
    destination().set_flight_sw_version(mavlink_msg_autopilot_version_get_flight_sw_version(&source));
    destination().set_middleware_sw_version(mavlink_msg_autopilot_version_get_middleware_sw_version(&source));
    destination().set_os_sw_version(mavlink_msg_autopilot_version_get_os_sw_version(&source));
    destination().set_board_version(mavlink_msg_autopilot_version_get_board_version(&source));
    destination().set_vendor_id(mavlink_msg_autopilot_version_get_vendor_id(&source));
    destination().set_product_id(mavlink_msg_autopilot_version_get_product_id(&source));

    std::array<u8, 8> flight_custom_version;
    mavlink_msg_autopilot_version_get_flight_custom_version(&source, flight_custom_version.data());
    destination().set_flight_custom_version(flight_custom_version.data(), flight_custom_version.size());

    std::array<u8, 8> middleware_custom_version;
    mavlink_msg_autopilot_version_get_middleware_custom_version(&source, middleware_custom_version.data());
    destination().set_middleware_custom_version(middleware_custom_version.data(), middleware_custom_version.size());

    std::array<u8, 8> os_custom_version;
    mavlink_msg_autopilot_version_get_os_custom_version(&source, os_custom_version.data());
    destination().set_os_custom_version(os_custom_version.data(), os_custom_version.size());

    std::array<u8, 18> uid2;
    mavlink_msg_autopilot_version_get_uid2(&source, uid2.data());
    destination().set_uid2(uid2.data(), uid2.size());
  }

  template <>
  void convert<mavlink::msg::common::EstimatorStatus>(const mavlink_message_t &source,
    Message<mavlink::msg::common::EstimatorStatus> &destination, const void *) {
    destination().set_time_usec(mavlink_msg_estimator_status_get_time_usec(&source));
    destination().set_vel_ratio(mavlink_msg_estimator_status_get_vel_ratio(&source));
    destination().set_pos_horiz_ratio(mavlink_msg_estimator_status_get_pos_horiz_ratio(&source));
    destination().set_pos_vert_ratio(mavlink_msg_estimator_status_get_pos_vert_ratio(&source));
    destination().set_mag_ratio(mavlink_msg_estimator_status_get_mag_ratio(&source));
    destination().set_hagl_ratio(mavlink_msg_estimator_status_get_hagl_ratio(&source));
    destination().set_tas_ratio(mavlink_msg_estimator_status_get_tas_ratio(&source));
    destination().set_pos_horiz_accuracy(mavlink_msg_estimator_status_get_pos_horiz_accuracy(&source));
    destination().set_pos_vert_accuracy(mavlink_msg_estimator_status_get_pos_vert_accuracy(&source));
    destination().set_flags(mavlink_msg_estimator_status_get_flags(&source));
  }

  template <>
  void convert<mavlink::msg::common::Vibration>(const mavlink_message_t &source, Message<mavlink::msg::common::Vibration> &destination,
    const void *) {
    destination().set_time_usec(mavlink_msg_vibration_get_time_usec(&source));
    destination().set_vibration_x(mavlink_msg_vibration_get_vibration_x(&source));
    destination().set_vibration_y(mavlink_msg_vibration_get_vibration_y(&source));
    destination().set_vibration_z(mavlink_msg_vibration_get_vibration_z(&source));
    destination().set_clipping_0(mavlink_msg_vibration_get_clipping_0(&source));
    destination().set_clipping_1(mavlink_msg_vibration_get_clipping_1(&source));
    destination().set_clipping_2(mavlink_msg_vibration_get_clipping_2(&source));
  }

  template <>
  void convert<mavlink::msg::common::HomePosition>(const mavlink_message_t &source,
    Message<mavlink::msg::common::HomePosition> &destination, const void *) {
    destination().set_latitude(mavlink_msg_home_position_get_latitude(&source));
    destination().set_longitude(mavlink_msg_home_position_get_longitude(&source));
    destination().set_altitude(mavlink_msg_home_position_get_altitude(&source));
    destination().set_x(mavlink_msg_home_position_get_x(&source));
    destination().set_y(mavlink_msg_home_position_get_y(&source));
    destination().set_z(mavlink_msg_home_position_get_z(&source));

    std::array<float, 4> q;
    mavlink_msg_home_position_get_q(&source, q.data());
    for (const float value : q) {
      destination().add_q(value);
    }

    destination().set_approach_x(mavlink_msg_home_position_get_approach_x(&source));
    destination().set_approach_y(mavlink_msg_home_position_get_approach_y(&source));
    destination().set_approach_z(mavlink_msg_home_position_get_approach_z(&source));
    destination().set_time_usec(mavlink_msg_home_position_get_time_usec(&source));
  }

  template <>
  void convert<mavlink::msg::common::ExtendedSysState>(const mavlink_message_t &source,
    Message<mavlink::msg::common::ExtendedSysState> &destination, const void *) {
    destination().set_vtol_state(mavlink_msg_extended_sys_state_get_vtol_state(&source));
    destination().set_landed_state(mavlink_msg_extended_sys_state_get_landed_state(&source));
  }

  template <>
  void convert<mavlink::msg::common::EscInfo>(const mavlink_message_t &source, Message<mavlink::msg::common::EscInfo> &destination,
    const void *) {
    destination().set_time_usec(mavlink_msg_esc_info_get_time_usec(&source));

    std::array<u32, 4> error_count;
    mavlink_msg_esc_info_get_error_count(&source, error_count.data());
    for (const u32 value : error_count) {
      destination().add_error_count(value);
    }

    destination().set_counter(mavlink_msg_esc_info_get_counter(&source));

    std::array<u16, 4> failure_flags;
    mavlink_msg_esc_info_get_failure_flags(&source, failure_flags.data());
    for (const u16 value : failure_flags) {
      destination().add_failure_flags(value);
    }

    std::array<i16, 4> temperature;
    mavlink_msg_esc_info_get_temperature(&source, temperature.data());
    for (const i16 value : temperature) {
      destination().add_temperature(value);
    }

    destination().set_index(mavlink_msg_esc_info_get_index(&source));
    destination().set_count(mavlink_msg_esc_info_get_count(&source));
    destination().set_connection_type(mavlink_msg_esc_info_get_connection_type(&source));
    destination().set_info(mavlink_msg_esc_info_get_info(&source));
  }

  template <>
  void convert<mavlink::msg::common::EscStatus>(const mavlink_message_t &source, Message<mavlink::msg::common::EscStatus> &destination,
    const void *) {
    destination().set_time_usec(mavlink_msg_esc_status_get_time_usec(&source));

    std::array<i32, 4> rpm;
    mavlink_msg_esc_status_get_rpm(&source, rpm.data());
    for (const i32 value : rpm) {
      destination().add_rpm(value);
    }

    std::array<float, 4> voltage;
    mavlink_msg_esc_status_get_voltage(&source, voltage.data());
    for (const float value : voltage) {
      destination().add_voltage(value);
    }

    std::array<float, 4> current;
    mavlink_msg_esc_status_get_current(&source, current.data());
    for (const float value : current) {
      destination().add_current(value);
    }

    destination().set_index(mavlink_msg_esc_status_get_index(&source));
  }

  template <>
  void convert<mavlink::msg::common::Odometry>(const mavlink_message_t &source, Message<mavlink::msg::common::Odometry> &destination,
    const void *) {
    destination().set_x(mavlink_msg_odometry_get_x(&source));
    destination().set_y(mavlink_msg_odometry_get_y(&source));
    destination().set_z(mavlink_msg_odometry_get_z(&source));

    std::array<float, 4> q;
    mavlink_msg_odometry_get_q(&source, q.data());
    for (const float value : q) {
      destination().add_q(value);
    }

    destination().set_vx(mavlink_msg_odometry_get_vx(&source));
    destination().set_vy(mavlink_msg_odometry_get_vy(&source));
    destination().set_vz(mavlink_msg_odometry_get_vz(&source));
    destination().set_rollspeed(mavlink_msg_odometry_get_rollspeed(&source));
    destination().set_pitchspeed(mavlink_msg_odometry_get_pitchspeed(&source));
    destination().set_yawspeed(mavlink_msg_odometry_get_yawspeed(&source));

    std::array<float, 21> pose_covariance;
    mavlink_msg_odometry_get_pose_covariance(&source, pose_covariance.data());
    for (const float value : pose_covariance) {
      destination().add_pose_covariance(value);
    }

    std::array<float, 21> velocity_covariance;
    mavlink_msg_odometry_get_velocity_covariance(&source, velocity_covariance.data());
    for (const float value : velocity_covariance) {
      destination().add_velocity_covariance(value);
    }

    destination().set_frame_id(mavlink_msg_odometry_get_frame_id(&source));
    destination().set_child_frame_id(mavlink_msg_odometry_get_child_frame_id(&source));
    destination().set_reset_counter(mavlink_msg_odometry_get_reset_counter(&source));
    destination().set_estimator_type(mavlink_msg_odometry_get_estimator_type(&source));
    destination().set_quality(mavlink_msg_odometry_get_quality(&source));
  }

  template <>
  void convert<mavlink::msg::common::UtmGlobalPosition>(const mavlink_message_t &source,
    Message<mavlink::msg::common::UtmGlobalPosition> &destination, const void *) {
    destination().set_time(mavlink_msg_utm_global_position_get_time(&source));
    destination().set_lat(mavlink_msg_utm_global_position_get_lat(&source));
    destination().set_lon(mavlink_msg_utm_global_position_get_lon(&source));
    destination().set_alt(mavlink_msg_utm_global_position_get_alt(&source));
    destination().set_relative_alt(mavlink_msg_utm_global_position_get_relative_alt(&source));
    destination().set_next_lat(mavlink_msg_utm_global_position_get_next_lat(&source));
    destination().set_next_lon(mavlink_msg_utm_global_position_get_next_lon(&source));
    destination().set_next_alt(mavlink_msg_utm_global_position_get_next_alt(&source));
    destination().set_vx(mavlink_msg_utm_global_position_get_vx(&source));
    destination().set_vy(mavlink_msg_utm_global_position_get_vy(&source));
    destination().set_vz(mavlink_msg_utm_global_position_get_vz(&source));
    destination().set_h_acc(mavlink_msg_utm_global_position_get_h_acc(&source));
    destination().set_v_acc(mavlink_msg_utm_global_position_get_v_acc(&source));
    destination().set_vel_acc(mavlink_msg_utm_global_position_get_vel_acc(&source));
    destination().set_update_rate(mavlink_msg_utm_global_position_get_update_rate(&source));

    std::array<u8, 18> uas_id;
    mavlink_msg_utm_global_position_get_uas_id(&source, uas_id.data());
    destination().set_uas_id(uas_id.data(), uas_id.size());

    destination().set_flight_state(mavlink_msg_utm_global_position_get_flight_state(&source));
    destination().set_flags(mavlink_msg_utm_global_position_get_flags(&source));
  }

  template <>
  void convert<mavlink::msg::common::TimeEstimateToTarget>(const mavlink_message_t &source,
    Message<mavlink::msg::common::TimeEstimateToTarget> &destination, const void *) {
    destination().set_safe_return(mavlink_msg_time_estimate_to_target_get_safe_return(&source));
    destination().set_land(mavlink_msg_time_estimate_to_target_get_land(&source));
    destination().set_mission_next_item(mavlink_msg_time_estimate_to_target_get_mission_next_item(&source));
    destination().set_mission_end(mavlink_msg_time_estimate_to_target_get_mission_end(&source));
    destination().set_commanded_action(mavlink_msg_time_estimate_to_target_get_commanded_action(&source));
  }

  template <>
  void convert<mavlink::msg::common::CurrentEventSequence>(const mavlink_message_t &source,
    Message<mavlink::msg::common::CurrentEventSequence> &destination, const void *) {
    destination().set_sequence(mavlink_msg_current_event_sequence_get_sequence(&source));
    destination().set_flags(mavlink_msg_current_event_sequence_get_flags(&source));
  }

  template <>
  void convert<mavlink::msg::common::OpenDroneIdLocation>(const mavlink_message_t &source,
    Message<mavlink::msg::common::OpenDroneIdLocation> &destination, const void *) {
    destination().set_latitude(mavlink_msg_open_drone_id_location_get_latitude(&source));
    destination().set_longitude(mavlink_msg_open_drone_id_location_get_longitude(&source));
    destination().set_altitude_barometric(mavlink_msg_open_drone_id_location_get_altitude_barometric(&source));
    destination().set_altitude_geodetic(mavlink_msg_open_drone_id_location_get_altitude_geodetic(&source));
    destination().set_height(mavlink_msg_open_drone_id_location_get_height(&source));
    destination().set_timestamp(mavlink_msg_open_drone_id_location_get_timestamp(&source));
    destination().set_direction(mavlink_msg_open_drone_id_location_get_direction(&source));
    destination().set_speed_horizontal(mavlink_msg_open_drone_id_location_get_speed_horizontal(&source));
    destination().set_speed_vertical(mavlink_msg_open_drone_id_location_get_speed_vertical(&source));
    destination().set_target_system(mavlink_msg_open_drone_id_location_get_target_system(&source));
    destination().set_target_component(mavlink_msg_open_drone_id_location_get_target_component(&source));

    std::array<u8, 20> id_or_mac;
    mavlink_msg_open_drone_id_location_get_id_or_mac(&source, id_or_mac.data());
    destination().set_id_or_mac(id_or_mac.data(), id_or_mac.size());

    destination().set_status(mavlink_msg_open_drone_id_location_get_status(&source));
    destination().set_height_reference(mavlink_msg_open_drone_id_location_get_height_reference(&source));
    destination().set_horizontal_accuracy(mavlink_msg_open_drone_id_location_get_horizontal_accuracy(&source));
    destination().set_vertical_accuracy(mavlink_msg_open_drone_id_location_get_vertical_accuracy(&source));
    destination().set_barometer_accuracy(mavlink_msg_open_drone_id_location_get_barometer_accuracy(&source));
    destination().set_speed_accuracy(mavlink_msg_open_drone_id_location_get_speed_accuracy(&source));
    destination().set_timestamp_accuracy(mavlink_msg_open_drone_id_location_get_timestamp_accuracy(&source));
  }

  template <>
  void convert<mavlink::msg::common::OpenDroneIdSystem>(const mavlink_message_t &source,
    Message<mavlink::msg::common::OpenDroneIdSystem> &destination, const void *) {
    destination().set_operator_latitude(mavlink_msg_open_drone_id_system_get_operator_latitude(&source));
    destination().set_operator_longitude(mavlink_msg_open_drone_id_system_get_operator_longitude(&source));
    destination().set_area_ceiling(mavlink_msg_open_drone_id_system_get_area_ceiling(&source));
    destination().set_area_floor(mavlink_msg_open_drone_id_system_get_area_floor(&source));
    destination().set_operator_altitude_geo(mavlink_msg_open_drone_id_system_get_operator_altitude_geo(&source));
    destination().set_timestamp(mavlink_msg_open_drone_id_system_get_timestamp(&source));
    destination().set_area_count(mavlink_msg_open_drone_id_system_get_area_count(&source));
    destination().set_area_radius(mavlink_msg_open_drone_id_system_get_area_radius(&source));
    destination().set_target_system(mavlink_msg_open_drone_id_system_get_target_system(&source));
    destination().set_target_component(mavlink_msg_open_drone_id_system_get_target_component(&source));

    std::array<u8, 20> id_or_mac;
    mavlink_msg_open_drone_id_system_get_id_or_mac(&source, id_or_mac.data());
    destination().set_id_or_mac(id_or_mac.data(), id_or_mac.size());

    destination().set_operator_location_type(mavlink_msg_open_drone_id_system_get_operator_location_type(&source));
    destination().set_classification_type(mavlink_msg_open_drone_id_system_get_classification_type(&source));
    destination().set_category_eu(mavlink_msg_open_drone_id_system_get_category_eu(&source));
    destination().set_class_eu(mavlink_msg_open_drone_id_system_get_class_eu(&source));
  }

  Node::Sender<Message<mavlink::msg::common::Heartbeat>, mavlink_message_t>::Ptr heartbeat;
  Node::Sender<Message<mavlink::msg::common::SysStatus>, mavlink_message_t>::Ptr sys_status;
  Node::Sender<Message<mavlink::msg::common::SystemTime>, mavlink_message_t>::Ptr system_time;
  Node::Sender<Message<mavlink::msg::common::Ping>, mavlink_message_t>::Ptr ping;
  Node::Sender<Message<mavlink::msg::common::LinkNodeStatus>, mavlink_message_t>::Ptr link_node_status;
  Node::Sender<Message<mavlink::msg::common::ParamValue>, mavlink_message_t>::Ptr param_value;
  Node::Sender<Message<mavlink::msg::common::ParamSet>, mavlink_message_t>::Ptr param_set;
  Node::Sender<Message<mavlink::msg::common::GpsRawInt>, mavlink_message_t>::Ptr gps_raw_int;
  Node::Sender<Message<mavlink::msg::common::GpsStatus>, mavlink_message_t>::Ptr gps_status;
  Node::Sender<Message<mavlink::msg::common::ScaledImu>, mavlink_message_t>::Ptr scaled_imu;
  Node::Sender<Message<mavlink::msg::common::RawImu>, mavlink_message_t>::Ptr raw_imu;
  Node::Sender<Message<mavlink::msg::common::ScaledPressure>, mavlink_message_t>::Ptr scaled_pressure;
  Node::Sender<Message<mavlink::msg::common::Attitude>, mavlink_message_t>::Ptr attitude;
  Node::Sender<Message<mavlink::msg::common::AttitudeQuaternion>, mavlink_message_t>::Ptr attitude_quaternion;
  Node::Sender<Message<mavlink::msg::common::LocalPositionNed>, mavlink_message_t>::Ptr local_position_ned;
  Node::Sender<Message<mavlink::msg::common::GlobalPositionInt>, mavlink_message_t>::Ptr global_position_int;
  Node::Sender<Message<mavlink::msg::common::RcChannels>, mavlink_message_t>::Ptr rc_channels;
  Node::Sender<Message<mavlink::msg::common::RcChannelsRaw>, mavlink_message_t>::Ptr rc_channels_raw;
  Node::Sender<Message<mavlink::msg::common::ServoOutputRaw>, mavlink_message_t>::Ptr servo_output_raw;
  Node::Sender<Message<mavlink::msg::common::VfrHud>, mavlink_message_t>::Ptr vfr_hud;
  Node::Sender<Message<mavlink::msg::common::CommandInt>, mavlink_message_t>::Ptr command_int;
  Node::Sender<Message<mavlink::msg::common::CommandLong>, mavlink_message_t>::Ptr command_long;
  Node::Sender<Message<mavlink::msg::common::CommandAck>, mavlink_message_t>::Ptr command_ack;
  Node::Sender<Message<mavlink::msg::common::PositionTargetLocalNed>, mavlink_message_t>::Ptr position_target_local_ned;
  Node::Sender<Message<mavlink::msg::common::AttitudeTarget>, mavlink_message_t>::Ptr attitude_target;
  Node::Sender<Message<mavlink::msg::common::PositionTargetGlobalInt>, mavlink_message_t>::Ptr position_target_global_int;
  Node::Sender<Message<mavlink::msg::common::LocalPositionNedSystemGlobalOffset>, mavlink_message_t>::Ptr
    local_position_ned_system_global_offset;
  Node::Sender<Message<mavlink::msg::common::HighresImu>, mavlink_message_t>::Ptr highres_imu;
  Node::Sender<Message<mavlink::msg::common::Timesync>, mavlink_message_t>::Ptr timesync;
  Node::Sender<Message<mavlink::msg::common::ActuatorControlTarget>, mavlink_message_t>::Ptr actuator_control_target;
  Node::Sender<Message<mavlink::msg::common::Altitude>, mavlink_message_t>::Ptr altitude;
  Node::Sender<Message<mavlink::msg::common::BatteryStatus>, mavlink_message_t>::Ptr battery_status;
  Node::Sender<Message<mavlink::msg::common::AutopilotVersion>, mavlink_message_t>::Ptr autopilot_version;
  Node::Sender<Message<mavlink::msg::common::EstimatorStatus>, mavlink_message_t>::Ptr estimator_status;
  Node::Sender<Message<mavlink::msg::common::Vibration>, mavlink_message_t>::Ptr vibration;
  Node::Sender<Message<mavlink::msg::common::HomePosition>, mavlink_message_t>::Ptr home_position;
  Node::Sender<Message<mavlink::msg::common::ExtendedSysState>, mavlink_message_t>::Ptr extended_sys_state;
  Node::Sender<Message<mavlink::msg::common::EscInfo>, mavlink_message_t>::Ptr esc_info;
  Node::Sender<Message<mavlink::msg::common::EscStatus>, mavlink_message_t>::Ptr esc_status;
  Node::Sender<Message<mavlink::msg::common::Odometry>, mavlink_message_t>::Ptr odometry;
  Node::Sender<Message<mavlink::msg::common::UtmGlobalPosition>, mavlink_message_t>::Ptr utm_global_position;
  Node::Sender<Message<mavlink::msg::common::TimeEstimateToTarget>, mavlink_message_t>::Ptr time_estimate_to_target;
  Node::Sender<Message<mavlink::msg::common::CurrentEventSequence>, mavlink_message_t>::Ptr current_event_sequence;
  Node::Sender<Message<mavlink::msg::common::OpenDroneIdLocation>, mavlink_message_t>::Ptr open_drone_id_location;
  Node::Sender<Message<mavlink::msg::common::OpenDroneIdSystem>, mavlink_message_t>::Ptr open_drone_id_system;
};

struct MavlinkReceiver {
  template <typename T>
  static void convert(const Message<T> &source, mavlink_message_t &destination, const MavlinkNode::SystemInfo *info);

  template <>
  void convert<mavlink::msg::common::ParamRequestRead>(const Message<mavlink::msg::common::ParamRequestRead> &source,
    mavlink_message_t &destination, const MavlinkNode::SystemInfo *info) {
    if (source().param_id().size() > 16) {
      throw ConversionException("The parameter ID string must not be larger than 16 characters.");
    }

    mavlink_msg_param_request_read_pack_chan(info->system_id, info->component_id, info->channel_id, &destination, source().target_system(),
      source().target_component(), source().param_id().c_str(), source().param_index());
  }

  template <>
  void convert<mavlink::msg::common::SetPositionTargetLocalNed>(const Message<mavlink::msg::common::SetPositionTargetLocalNed> &source,
    mavlink_message_t &destination, const MavlinkNode::SystemInfo *info) {
    mavlink_msg_set_position_target_local_ned_pack_chan(info->system_id, info->component_id, info->channel_id, &destination,
      source().time_boot_ms(), source().target_system(), source().target_component(), source().coordinate_frame(), source().type_mask(),
      source().x(), source().y(), source().z(), source().vx(), source().vy(), source().vz(), source().afx(), source().afy(), source().afz(),
      source().yaw(), source().yaw_rate());
  }

  template <>
  void convert<mavlink::msg::common::CommandInt>(const Message<mavlink::msg::common::CommandInt> &source, mavlink_message_t &destination,
    const MavlinkNode::SystemInfo *info) {
    mavlink_msg_command_int_pack_chan(info->system_id, info->component_id, info->channel_id, &destination, source().target_system(),
      source().target_component(), source().frame(), source().command(), source().current(), source().autocontinue(), source().param1(),
      source().param2(), source().param3(), source().param4(), source().x(), source().y(), source().z());
  }

  template <>
  void convert<mavlink::msg::common::CommandLong>(const Message<mavlink::msg::common::CommandLong> &source, mavlink_message_t &destination,
    const MavlinkNode::SystemInfo *info) {
    mavlink_msg_command_long_pack_chan(info->system_id, info->component_id, info->channel_id, &destination, source().target_system(),
      source().target_component(), source().command(), source().confirmation(), source().param1(), source().param2(), source().param3(),
      source().param4(), source().param5(), source().param6(), source().param7());
  }

  template <typename T>
  static void callback(Node::Receiver<Message<T>, mavlink_message_t> &receiver, MavlinkNode *node) {
    node->writeMessage(receiver.latest());
  }

  Node::Receiver<Message<mavlink::msg::common::ParamRequestRead>, mavlink_message_t>::Ptr param_request_read;
  Node::Receiver<Message<mavlink::msg::common::SetPositionTargetLocalNed>, mavlink_message_t>::Ptr set_position_target_local_ned;
  Node::Receiver<Message<mavlink::msg::common::CommandInt>, mavlink_message_t>::Ptr command_int;
  Node::Receiver<Message<mavlink::msg::common::CommandLong>, mavlink_message_t>::Ptr command_long;
};

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

  template <>
  Message<mavlink::msg::common::ParamValue> handle<mavlink::msg::common::ParamRequestRead, mavlink::msg::common::ParamValue>(
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
    } while (result().param_id() != request().param_id());

    return result;
  }

  template <>
  Message<mavlink::msg::common::CommandAck> handle<mavlink::msg::common::CommandInt, mavlink::msg::common::CommandAck>(
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
    } while (result().command() != request().command());

    return result;
  }

  template <>
  Message<mavlink::msg::common::CommandAck> handle<mavlink::msg::common::CommandLong, mavlink::msg::common::CommandAck>(
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
    } while (result().command() != request().command());

    return result;
  }

  ServerInfo<mavlink::msg::common::ParamRequestRead, mavlink::msg::common::ParamValue>::Ptr param_request_read;
  ServerInfo<mavlink::msg::common::CommandInt, mavlink::msg::common::CommandAck>::Ptr command_int;
  ServerInfo<mavlink::msg::common::CommandLong, mavlink::msg::common::CommandAck>::Ptr command_long;
};

MavlinkNode::MavlinkNode(const Node::Environment &environment, MavlinkConnection::Ptr &&connection) :
  Node(environment), connection(std::forward<MavlinkConnection::Ptr>(connection)) {
  sender = new MavlinkSender;
  receiver = new MavlinkReceiver;
  server = new MavlinkServer;

  system_info.channel_id = MAVLINK_COMM_0;
  system_info.system_id = 0xf0;
  system_info.component_id = MAV_COMP_ID_ONBOARD_COMPUTER;

  sender->heartbeat = addSender<Message<mavlink::msg::common::Heartbeat>, mavlink_message_t>("/mavlink/in/heartbeat",
    MavlinkSender::convert<mavlink::msg::common::Heartbeat>);
  sender->sys_status = addSender<Message<mavlink::msg::common::SysStatus>, mavlink_message_t>("/mavlink/in/sys_status",
    MavlinkSender::convert<mavlink::msg::common::SysStatus>);
  sender->system_time = addSender<Message<mavlink::msg::common::SystemTime>, mavlink_message_t>("/mavlink/in/system_time",
    MavlinkSender::convert<mavlink::msg::common::SystemTime>);
  sender->ping = addSender<Message<mavlink::msg::common::Ping>, mavlink_message_t>("/mavlink/in/ping",
    MavlinkSender::convert<mavlink::msg::common::Ping>);
  sender->link_node_status = addSender<Message<mavlink::msg::common::LinkNodeStatus>, mavlink_message_t>("/mavlink/in/link_node_status",
    MavlinkSender::convert<mavlink::msg::common::LinkNodeStatus>);
  sender->param_value = addSender<Message<mavlink::msg::common::ParamValue>, mavlink_message_t>("/mavlink/in/param_value",
    MavlinkSender::convert<mavlink::msg::common::ParamValue>);
  sender->param_set = addSender<Message<mavlink::msg::common::ParamSet>, mavlink_message_t>("/mavlink/in/param_set",
    MavlinkSender::convert<mavlink::msg::common::ParamSet>);
  sender->gps_raw_int = addSender<Message<mavlink::msg::common::GpsRawInt>, mavlink_message_t>("/mavlink/in/gps_raw_int",
    MavlinkSender::convert<mavlink::msg::common::GpsRawInt>);
  sender->gps_status = addSender<Message<mavlink::msg::common::GpsStatus>, mavlink_message_t>("/mavlink/in/gps_status",
    MavlinkSender::convert<mavlink::msg::common::GpsStatus>);
  sender->scaled_imu = addSender<Message<mavlink::msg::common::ScaledImu>, mavlink_message_t>("/mavlink/in/scaled_imu",
    MavlinkSender::convert<mavlink::msg::common::ScaledImu>);
  sender->raw_imu = addSender<Message<mavlink::msg::common::RawImu>, mavlink_message_t>("/mavlink/in/raw_imu",
    MavlinkSender::convert<mavlink::msg::common::RawImu>);
  sender->scaled_pressure = addSender<Message<mavlink::msg::common::ScaledPressure>, mavlink_message_t>("/mavlink/in/scaled_pressure",
    MavlinkSender::convert<mavlink::msg::common::ScaledPressure>);
  sender->attitude = addSender<Message<mavlink::msg::common::Attitude>, mavlink_message_t>("/mavlink/in/attitude",
    MavlinkSender::convert<mavlink::msg::common::Attitude>);
  sender->attitude_quaternion = addSender<Message<mavlink::msg::common::AttitudeQuaternion>, mavlink_message_t>(
    "/mavlink/in/attitude_quaternion", MavlinkSender::convert<mavlink::msg::common::AttitudeQuaternion>);
  sender->local_position_ned = addSender<Message<mavlink::msg::common::LocalPositionNed>, mavlink_message_t>(
    "/mavlink/in/local_position_ned", MavlinkSender::convert<mavlink::msg::common::LocalPositionNed>);
  sender->global_position_int = addSender<Message<mavlink::msg::common::GlobalPositionInt>, mavlink_message_t>(
    "/mavlink/in/global_position_int", MavlinkSender::convert<mavlink::msg::common::GlobalPositionInt>);
  sender->rc_channels = addSender<Message<mavlink::msg::common::RcChannels>, mavlink_message_t>("/mavlink/in/rc_channels",
    MavlinkSender::convert<mavlink::msg::common::RcChannels>);
  sender->rc_channels_raw = addSender<Message<mavlink::msg::common::RcChannelsRaw>, mavlink_message_t>("/mavlink/in/rc_channels_raw",
    MavlinkSender::convert<mavlink::msg::common::RcChannelsRaw>);
  sender->servo_output_raw = addSender<Message<mavlink::msg::common::ServoOutputRaw>, mavlink_message_t>("/mavlink/in/servo_output_raw",
    MavlinkSender::convert<mavlink::msg::common::ServoOutputRaw>);
  sender->vfr_hud = addSender<Message<mavlink::msg::common::VfrHud>, mavlink_message_t>("/mavlink/in/vfr_hud",
    MavlinkSender::convert<mavlink::msg::common::VfrHud>);
  sender->command_int = addSender<Message<mavlink::msg::common::CommandInt>, mavlink_message_t>("/mavlink/in/command_int",
    MavlinkSender::convert<mavlink::msg::common::CommandInt>);
  sender->command_long = addSender<Message<mavlink::msg::common::CommandLong>, mavlink_message_t>("/mavlink/in/command_long",
    MavlinkSender::convert<mavlink::msg::common::CommandLong>);
  sender->command_ack = addSender<Message<mavlink::msg::common::CommandAck>, mavlink_message_t>("/mavlink/in/command_ack",
    MavlinkSender::convert<mavlink::msg::common::CommandAck>);
  sender->position_target_local_ned = addSender<Message<mavlink::msg::common::PositionTargetLocalNed>, mavlink_message_t>(
    "/mavlink/in/position_target_local_ned", MavlinkSender::convert<mavlink::msg::common::PositionTargetLocalNed>);
  sender->attitude_target = addSender<Message<mavlink::msg::common::AttitudeTarget>, mavlink_message_t>("/mavlink/in/attitude_target",
    MavlinkSender::convert<mavlink::msg::common::AttitudeTarget>);
  sender->position_target_global_int = addSender<Message<mavlink::msg::common::PositionTargetGlobalInt>, mavlink_message_t>(
    "/mavlink/in/position_target_global_int", MavlinkSender::convert<mavlink::msg::common::PositionTargetGlobalInt>);
  sender->local_position_ned_system_global_offset =
    addSender<Message<mavlink::msg::common::LocalPositionNedSystemGlobalOffset>, mavlink_message_t>(
      "/mavlink/in/local_position_ned_system_global_offset",
      MavlinkSender::convert<mavlink::msg::common::LocalPositionNedSystemGlobalOffset>);
  sender->highres_imu = addSender<Message<mavlink::msg::common::HighresImu>, mavlink_message_t>("/mavlink/in/highres_imu",
    MavlinkSender::convert<mavlink::msg::common::HighresImu>);
  sender->timesync = addSender<Message<mavlink::msg::common::Timesync>, mavlink_message_t>("/mavlink/in/timesync",
    MavlinkSender::convert<mavlink::msg::common::Timesync>);
  sender->actuator_control_target = addSender<Message<mavlink::msg::common::ActuatorControlTarget>, mavlink_message_t>(
    "/mavlink/in/actuator_control_target", MavlinkSender::convert<mavlink::msg::common::ActuatorControlTarget>);
  sender->altitude = addSender<Message<mavlink::msg::common::Altitude>, mavlink_message_t>("/mavlink/in/altitude",
    MavlinkSender::convert<mavlink::msg::common::Altitude>);
  sender->battery_status = addSender<Message<mavlink::msg::common::BatteryStatus>, mavlink_message_t>("/mavlink/in/battery_status",
    MavlinkSender::convert<mavlink::msg::common::BatteryStatus>);
  sender->autopilot_version = addSender<Message<mavlink::msg::common::AutopilotVersion>, mavlink_message_t>("/mavlink/in/autopilot_version",
    MavlinkSender::convert<mavlink::msg::common::AutopilotVersion>);
  sender->estimator_status = addSender<Message<mavlink::msg::common::EstimatorStatus>, mavlink_message_t>("/mavlink/in/estimator_status",
    MavlinkSender::convert<mavlink::msg::common::EstimatorStatus>);
  sender->vibration = addSender<Message<mavlink::msg::common::Vibration>, mavlink_message_t>("/mavlink/in/vibration",
    MavlinkSender::convert<mavlink::msg::common::Vibration>);
  sender->home_position = addSender<Message<mavlink::msg::common::HomePosition>, mavlink_message_t>("/mavlink/in/home_position",
    MavlinkSender::convert<mavlink::msg::common::HomePosition>);
  sender->extended_sys_state = addSender<Message<mavlink::msg::common::ExtendedSysState>, mavlink_message_t>(
    "/mavlink/in/extended_sys_state", MavlinkSender::convert<mavlink::msg::common::ExtendedSysState>);
  sender->esc_info = addSender<Message<mavlink::msg::common::EscInfo>, mavlink_message_t>("/mavlink/in/esc_info",
    MavlinkSender::convert<mavlink::msg::common::EscInfo>);
  sender->esc_status = addSender<Message<mavlink::msg::common::EscStatus>, mavlink_message_t>("/mavlink/in/esc_status",
    MavlinkSender::convert<mavlink::msg::common::EscStatus>);
  sender->odometry = addSender<Message<mavlink::msg::common::Odometry>, mavlink_message_t>("/mavlink/in/odometry",
    MavlinkSender::convert<mavlink::msg::common::Odometry>);
  sender->utm_global_position = addSender<Message<mavlink::msg::common::UtmGlobalPosition>, mavlink_message_t>(
    "/mavlink/in/utm_global_position", MavlinkSender::convert<mavlink::msg::common::UtmGlobalPosition>);
  sender->time_estimate_to_target = addSender<Message<mavlink::msg::common::TimeEstimateToTarget>, mavlink_message_t>(
    "/mavlink/in/time_estimate_to_target", MavlinkSender::convert<mavlink::msg::common::TimeEstimateToTarget>);
  sender->current_event_sequence = addSender<Message<mavlink::msg::common::CurrentEventSequence>, mavlink_message_t>(
    "/mavlink/in/current_event_sequence", MavlinkSender::convert<mavlink::msg::common::CurrentEventSequence>);
  sender->open_drone_id_location = addSender<Message<mavlink::msg::common::OpenDroneIdLocation>, mavlink_message_t>(
    "/mavlink/in/open_drone_id_location", MavlinkSender::convert<mavlink::msg::common::OpenDroneIdLocation>);
  sender->open_drone_id_system = addSender<Message<mavlink::msg::common::OpenDroneIdSystem>, mavlink_message_t>(
    "/mavlink/in/open_drone_id_system", MavlinkSender::convert<mavlink::msg::common::OpenDroneIdSystem>);

  receiver->param_request_read = addReceiver<Message<mavlink::msg::common::ParamRequestRead>, mavlink_message_t>(
    "/mavlink/out/param_request_read", MavlinkReceiver::convert<mavlink::msg::common::ParamRequestRead>, &system_info);
  receiver->set_position_target_local_ned = addReceiver<Message<mavlink::msg::common::SetPositionTargetLocalNed>, mavlink_message_t>(
    "/mavlink/out/set_position_target_local_ned", MavlinkReceiver::convert<mavlink::msg::common::SetPositionTargetLocalNed>, &system_info);
  receiver->command_int = addReceiver<Message<mavlink::msg::common::CommandInt>, mavlink_message_t>("/mavlink/out/command_int",
    MavlinkReceiver::convert<mavlink::msg::common::CommandInt>, &system_info);
  receiver->command_long = addReceiver<Message<mavlink::msg::common::CommandLong>, mavlink_message_t>("/mavlink/out/command_long",
    MavlinkReceiver::convert<mavlink::msg::common::CommandLong>, &system_info);

  receiver->param_request_read->setCallback(MavlinkReceiver::callback<mavlink::msg::common::ParamRequestRead>, this);
  receiver->set_position_target_local_ned->setCallback(MavlinkReceiver::callback<mavlink::msg::common::SetPositionTargetLocalNed>, this);
  receiver->command_int->setCallback(MavlinkReceiver::callback<mavlink::msg::common::CommandInt>, this);
  receiver->command_long->setCallback(MavlinkReceiver::callback<mavlink::msg::common::CommandLong>, this);

  server->param_request_read =
    std::make_unique<MavlinkServer::ServerInfo<mavlink::msg::common::ParamRequestRead, mavlink::msg::common::ParamValue>>(this,
      "/mavlink/srv/param_request_read", "/mavlink/out/param_request_read", "/mavlink/in/param_value");
  server->command_int = std::make_unique<MavlinkServer::ServerInfo<mavlink::msg::common::CommandInt, mavlink::msg::common::CommandAck>>(
    this, "/mavlink/srv/command_int", "/mavlink/out/command_int", "/mavlink/in/command_ack");
  server->command_long = std::make_unique<MavlinkServer::ServerInfo<mavlink::msg::common::CommandLong, mavlink::msg::common::CommandAck>>(
    this, "/mavlink/srv/command_long", "/mavlink/out/command_long", "/mavlink/in/command_ack");

  exit_flag.clear();
  read_thread = std::thread(&MavlinkNode::readLoop, this);
  heartbeat_thread = std::thread(&MavlinkNode::heartbeatLoop, this);
}

MavlinkNode::~MavlinkNode() {
  exit_flag.test_and_set();
  read_thread.join();
  heartbeat_thread.join();

  delete sender;
  delete receiver;
  delete server;
}

void MavlinkNode::readLoop() {
  std::array<u8, buffer_size> buffer;

  while (!exit_flag.test()) {
    const std::size_t number_bytes = connection->read(buffer.data(), buffer_size);

    for (std::size_t i = 0; i < number_bytes; ++i) {
      mavlink_message_t message;
      mavlink_status_t status;

      if (mavlink_parse_char(system_info.channel_id, buffer[i], &message, &status) == MAVLINK_FRAMING_OK) {
        readMessage(message);
      }
    }
  }
}

void MavlinkNode::heartbeatLoop() {
  mavlink_message_t message;
  mavlink_msg_heartbeat_pack_chan(system_info.system_id, system_info.component_id, system_info.channel_id, &message,
    MAV_TYPE_ONBOARD_CONTROLLER, MAV_AUTOPILOT_INVALID, 0, 0, MAV_STATE_ACTIVE);

  while (!exit_flag.test()) {
    writeMessage(message);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}

void MavlinkNode::readMessage(const mavlink_message_t &message) {
  switch (message.msgid) {
    case (MAVLINK_MSG_ID_HEARTBEAT): {
      sender->heartbeat->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_SYS_STATUS): {
      sender->sys_status->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_SYSTEM_TIME): {
      sender->system_time->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_PING): {
      sender->ping->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_LINK_NODE_STATUS): {
      sender->link_node_status->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_PARAM_VALUE): {
      sender->param_value->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_PARAM_SET): {
      sender->param_set->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_GPS_RAW_INT): {
      sender->gps_raw_int->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_GPS_STATUS): {
      sender->gps_status->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_SCALED_IMU): {
      sender->scaled_imu->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_RAW_IMU): {
      sender->raw_imu->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_SCALED_PRESSURE): {
      sender->scaled_pressure->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_ATTITUDE): {
      sender->attitude->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_ATTITUDE_QUATERNION): {
      sender->attitude_quaternion->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_LOCAL_POSITION_NED): {
      sender->local_position_ned->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_GLOBAL_POSITION_INT): {
      sender->global_position_int->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_RC_CHANNELS_RAW): {
      sender->rc_channels_raw->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_RC_CHANNELS): {
      sender->rc_channels->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_SERVO_OUTPUT_RAW): {
      sender->servo_output_raw->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_VFR_HUD): {
      sender->vfr_hud->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_COMMAND_INT): {
      sender->command_int->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_COMMAND_LONG): {
      sender->command_long->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_COMMAND_ACK): {
      sender->command_ack->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_POSITION_TARGET_LOCAL_NED): {
      sender->position_target_local_ned->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_ATTITUDE_TARGET): {
      sender->attitude_target->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_POSITION_TARGET_GLOBAL_INT): {
      sender->position_target_global_int->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_LOCAL_POSITION_NED_SYSTEM_GLOBAL_OFFSET): {
      sender->local_position_ned_system_global_offset->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_HIGHRES_IMU): {
      sender->highres_imu->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_TIMESYNC): {
      sender->timesync->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_ACTUATOR_CONTROL_TARGET): {
      sender->actuator_control_target->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_ALTITUDE): {
      sender->altitude->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_BATTERY_STATUS): {
      sender->battery_status->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_AUTOPILOT_VERSION): {
      sender->autopilot_version->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_ESTIMATOR_STATUS): {
      sender->estimator_status->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_VIBRATION): {
      sender->vibration->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_HOME_POSITION): {
      sender->home_position->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_EXTENDED_SYS_STATE): {
      sender->extended_sys_state->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_ESC_INFO): {
      sender->esc_info->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_ESC_STATUS): {
      sender->esc_status->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_ODOMETRY): {
      sender->odometry->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_UTM_GLOBAL_POSITION): {
      sender->utm_global_position->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_TIME_ESTIMATE_TO_TARGET): {
      sender->time_estimate_to_target->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_CURRENT_EVENT_SEQUENCE): {
      sender->current_event_sequence->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_OPEN_DRONE_ID_LOCATION): {
      sender->open_drone_id_location->put(message);
      break;
    }

    case (MAVLINK_MSG_ID_OPEN_DRONE_ID_SYSTEM): {
      sender->open_drone_id_system->put(message);
      break;
    }

    default: {
      getLogger().debug() << "Received MAVLink message without handling implementation (ID: " << message.msgid << ").";
      break;
    }
  }
}

void MavlinkNode::writeMessage(const mavlink_message_t &message) {
  std::array<u8, MAVLINK_MAX_PACKET_LEN> buffer;
  const std::size_t number_bytes = mavlink_msg_to_send_buffer(buffer.data(), &message);

  std::lock_guard guard(mutex);

  connection->write(buffer.data(), number_bytes);
}

}  // namespace labrat::robot::plugins
