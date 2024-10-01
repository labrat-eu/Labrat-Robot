/**
 * @file node.cpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v2.1 or later (LGPL-2.1-or-later)
 *
 */

#include <labrat/lbot/message.hpp>
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
#include <labrat/lbot/utils/string.hpp>

#include <mavlink/common/mavlink.h>

inline namespace labrat {
namespace lbot::plugins {

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::Heartbeat>::convertFrom(const Converted &source, Storage &destination)
{
  destination.type = mavlink_msg_heartbeat_get_type(&source);
  destination.autopilot = mavlink_msg_heartbeat_get_autopilot(&source);
  destination.base_mode = mavlink_msg_heartbeat_get_base_mode(&source);
  destination.custom_mode = mavlink_msg_heartbeat_get_custom_mode(&source);
  destination.system_status = mavlink_msg_heartbeat_get_system_status(&source);
  destination.mavlink_version = mavlink_msg_heartbeat_get_mavlink_version(&source);
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::SysStatus>::convertFrom(const Converted &source, Storage &destination)
{
  destination.onboard_control_sensors_present =
    static_cast<mavlink::common::SysStatusSensor>(mavlink_msg_sys_status_get_onboard_control_sensors_present(&source));
  destination.onboard_control_sensors_enabled =
    static_cast<mavlink::common::SysStatusSensor>(mavlink_msg_sys_status_get_onboard_control_sensors_enabled(&source));
  destination.onboard_control_sensors_health =
    static_cast<mavlink::common::SysStatusSensor>(mavlink_msg_sys_status_get_onboard_control_sensors_health(&source));
  destination.load = mavlink_msg_sys_status_get_load(&source);
  destination.voltage_battery = mavlink_msg_sys_status_get_voltage_battery(&source);
  destination.current_battery = mavlink_msg_sys_status_get_current_battery(&source);
  destination.drop_rate_comm = mavlink_msg_sys_status_get_drop_rate_comm(&source);
  destination.errors_comm = mavlink_msg_sys_status_get_errors_comm(&source);
  destination.errors_count1 = mavlink_msg_sys_status_get_errors_count1(&source);
  destination.errors_count2 = mavlink_msg_sys_status_get_errors_count2(&source);
  destination.errors_count3 = mavlink_msg_sys_status_get_errors_count3(&source);
  destination.errors_count4 = mavlink_msg_sys_status_get_errors_count4(&source);
  destination.battery_remaining = mavlink_msg_sys_status_get_battery_remaining(&source);
  destination.onboard_control_sensors_present_extended =
    static_cast<mavlink::common::SysStatusSensorExtended>(mavlink_msg_sys_status_get_onboard_control_sensors_present_extended(&source));
  destination.onboard_control_sensors_enabled_extended =
    static_cast<mavlink::common::SysStatusSensorExtended>(mavlink_msg_sys_status_get_onboard_control_sensors_enabled_extended(&source));
  destination.onboard_control_sensors_health_extended =
    static_cast<mavlink::common::SysStatusSensorExtended>(mavlink_msg_sys_status_get_onboard_control_sensors_health_extended(&source));
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::SystemTime>::convertFrom(const Converted &source, Storage &destination)
{
  destination.time_unix_usec = mavlink_msg_system_time_get_time_unix_usec(&source);
  destination.time_boot_ms = mavlink_msg_system_time_get_time_boot_ms(&source);
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::Ping>::convertFrom(const Converted &source, Storage &destination)
{
  destination.time_usec = mavlink_msg_ping_get_time_usec(&source);
  destination.seq = mavlink_msg_ping_get_seq(&source);
  destination.target_system = mavlink_msg_ping_get_target_system(&source);
  destination.target_component = mavlink_msg_ping_get_target_component(&source);
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::LinkNodeStatus>::convertFrom(const Converted &source, Storage &destination)
{
  destination.timestamp = mavlink_msg_link_node_status_get_timestamp(&source);
  destination.tx_rate = mavlink_msg_link_node_status_get_tx_rate(&source);
  destination.rx_rate = mavlink_msg_link_node_status_get_rx_rate(&source);
  destination.messages_sent = mavlink_msg_link_node_status_get_messages_sent(&source);
  destination.messages_received = mavlink_msg_link_node_status_get_messages_received(&source);
  destination.messages_lost = mavlink_msg_link_node_status_get_messages_lost(&source);
  destination.rx_parse_err = mavlink_msg_link_node_status_get_rx_parse_err(&source);
  destination.tx_overflows = mavlink_msg_link_node_status_get_tx_overflows(&source);
  destination.rx_overflows = mavlink_msg_link_node_status_get_rx_overflows(&source);
  destination.tx_buf = mavlink_msg_link_node_status_get_tx_buf(&source);
  destination.rx_buf = mavlink_msg_link_node_status_get_rx_buf(&source);
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::ParamValue>::convertFrom(const Converted &source, Storage &destination)
{
  destination.param_value = mavlink_msg_param_value_get_param_value(&source);
  destination.param_count = mavlink_msg_param_value_get_param_count(&source);
  destination.param_index = mavlink_msg_param_value_get_param_index(&source);

  destination.param_id.resize(17);
  mavlink_msg_param_value_get_param_id(&source, destination.param_id.data());
  shrinkString(destination.param_id);

  destination.param_type = static_cast<mavlink::common::ParamType>(mavlink_msg_param_value_get_param_type(&source));
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::ParamSet>::convertFrom(const Converted &source, Storage &destination)
{
  destination.param_value = mavlink_msg_param_set_get_param_value(&source);
  destination.target_system = mavlink_msg_param_set_get_target_system(&source);
  destination.target_component = mavlink_msg_param_set_get_target_component(&source);

  destination.param_id.resize(17);
  mavlink_msg_param_set_get_param_id(&source, destination.param_id.data());
  shrinkString(destination.param_id);

  destination.param_type = static_cast<mavlink::common::ParamType>(mavlink_msg_param_set_get_param_type(&source));
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::GpsRawInt>::convertFrom(const Converted &source, Storage &destination)
{
  destination.time_usec = mavlink_msg_gps_raw_int_get_time_usec(&source);
  destination.lat = mavlink_msg_gps_raw_int_get_lat(&source);
  destination.lon = mavlink_msg_gps_raw_int_get_lon(&source);
  destination.alt = mavlink_msg_gps_raw_int_get_alt(&source);
  destination.eph = mavlink_msg_gps_raw_int_get_eph(&source);
  destination.epv = mavlink_msg_gps_raw_int_get_epv(&source);
  destination.vel = mavlink_msg_gps_raw_int_get_vel(&source);
  destination.cog = mavlink_msg_gps_raw_int_get_cog(&source);
  destination.fix_type = static_cast<mavlink::common::GpsFixType>(mavlink_msg_gps_raw_int_get_fix_type(&source));
  destination.satellites_visible = mavlink_msg_gps_raw_int_get_satellites_visible(&source);
  destination.alt_ellipsoid = mavlink_msg_gps_raw_int_get_alt_ellipsoid(&source);
  destination.h_acc = mavlink_msg_gps_raw_int_get_h_acc(&source);
  destination.v_acc = mavlink_msg_gps_raw_int_get_v_acc(&source);
  destination.vel_acc = mavlink_msg_gps_raw_int_get_vel_acc(&source);
  destination.hdg_acc = mavlink_msg_gps_raw_int_get_hdg_acc(&source);
  destination.yaw = mavlink_msg_gps_raw_int_get_yaw(&source);
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::GpsStatus>::convertFrom(const Converted &source, Storage &destination)
{
  destination.satellites_visible = mavlink_msg_gps_status_get_satellites_visible(&source);

  destination.satellite_prn.resize(20);
  mavlink_msg_gps_status_get_satellite_prn(&source, destination.satellite_prn.data());

  destination.satellite_used.resize(20);
  mavlink_msg_gps_status_get_satellite_used(&source, destination.satellite_used.data());

  destination.satellite_elevation.resize(20);
  mavlink_msg_gps_status_get_satellite_elevation(&source, destination.satellite_elevation.data());

  destination.satellite_azimuth.resize(20);
  mavlink_msg_gps_status_get_satellite_azimuth(&source, destination.satellite_azimuth.data());

  destination.satellite_snr.resize(20);
  mavlink_msg_gps_status_get_satellite_snr(&source, destination.satellite_snr.data());
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::ScaledImu>::convertFrom(const Converted &source, Storage &destination)
{
  destination.time_boot_ms = mavlink_msg_scaled_imu_get_time_boot_ms(&source);
  destination.xacc = mavlink_msg_scaled_imu_get_xacc(&source);
  destination.yacc = mavlink_msg_scaled_imu_get_yacc(&source);
  destination.zacc = mavlink_msg_scaled_imu_get_zacc(&source);
  destination.xgyro = mavlink_msg_scaled_imu_get_xgyro(&source);
  destination.ygyro = mavlink_msg_scaled_imu_get_ygyro(&source);
  destination.zgyro = mavlink_msg_scaled_imu_get_zgyro(&source);
  destination.xmag = mavlink_msg_scaled_imu_get_xmag(&source);
  destination.ymag = mavlink_msg_scaled_imu_get_ymag(&source);
  destination.zmag = mavlink_msg_scaled_imu_get_zmag(&source);
  destination.temperature = mavlink_msg_scaled_imu_get_temperature(&source);
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::RawImu>::convertFrom(const Converted &source, Storage &destination)
{
  destination.time_usec = mavlink_msg_raw_imu_get_time_usec(&source);
  destination.xacc = mavlink_msg_raw_imu_get_xacc(&source);
  destination.yacc = mavlink_msg_raw_imu_get_yacc(&source);
  destination.zacc = mavlink_msg_raw_imu_get_zacc(&source);
  destination.xgyro = mavlink_msg_raw_imu_get_xgyro(&source);
  destination.ygyro = mavlink_msg_raw_imu_get_ygyro(&source);
  destination.zgyro = mavlink_msg_raw_imu_get_zgyro(&source);
  destination.xmag = mavlink_msg_raw_imu_get_xmag(&source);
  destination.ymag = mavlink_msg_raw_imu_get_ymag(&source);
  destination.zmag = mavlink_msg_raw_imu_get_zmag(&source);
  destination.id = mavlink_msg_raw_imu_get_id(&source);
  destination.temperature = mavlink_msg_raw_imu_get_temperature(&source);
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::ScaledPressure>::convertFrom(const Converted &source, Storage &destination)
{
  destination.time_boot_ms = mavlink_msg_scaled_pressure_get_time_boot_ms(&source);
  destination.press_abs = mavlink_msg_scaled_pressure_get_press_abs(&source);
  destination.press_diff = mavlink_msg_scaled_pressure_get_press_diff(&source);
  destination.temperature = mavlink_msg_scaled_pressure_get_temperature(&source);
  destination.temperature_press_diff = mavlink_msg_scaled_pressure_get_temperature_press_diff(&source);
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::Attitude>::convertFrom(const Converted &source, Storage &destination)
{
  destination.time_boot_ms = mavlink_msg_attitude_get_time_boot_ms(&source);
  destination.roll = mavlink_msg_attitude_get_roll(&source);
  destination.pitch = mavlink_msg_attitude_get_pitch(&source);
  destination.yaw = mavlink_msg_attitude_get_yaw(&source);
  destination.rollspeed = mavlink_msg_attitude_get_rollspeed(&source);
  destination.pitchspeed = mavlink_msg_attitude_get_pitchspeed(&source);
  destination.yawspeed = mavlink_msg_attitude_get_yawspeed(&source);
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::AttitudeQuaternion>::convertFrom(const Converted &source, Storage &destination)
{
  destination.time_boot_ms = mavlink_msg_attitude_quaternion_get_time_boot_ms(&source);
  destination.q1 = mavlink_msg_attitude_quaternion_get_q1(&source);
  destination.q2 = mavlink_msg_attitude_quaternion_get_q2(&source);
  destination.q3 = mavlink_msg_attitude_quaternion_get_q3(&source);
  destination.q4 = mavlink_msg_attitude_quaternion_get_q4(&source);
  destination.rollspeed = mavlink_msg_attitude_quaternion_get_rollspeed(&source);
  destination.pitchspeed = mavlink_msg_attitude_quaternion_get_pitchspeed(&source);
  destination.yawspeed = mavlink_msg_attitude_quaternion_get_yawspeed(&source);

  destination.repr_offset_q.resize(4);
  mavlink_msg_attitude_quaternion_get_repr_offset_q(&source, destination.repr_offset_q.data());
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::LocalPositionNed>::convertFrom(const Converted &source, Storage &destination)
{
  destination.time_boot_ms = mavlink_msg_local_position_ned_get_time_boot_ms(&source);
  destination.x = mavlink_msg_local_position_ned_get_x(&source);
  destination.y = mavlink_msg_local_position_ned_get_y(&source);
  destination.z = mavlink_msg_local_position_ned_get_z(&source);
  destination.vx = mavlink_msg_local_position_ned_get_vx(&source);
  destination.vy = mavlink_msg_local_position_ned_get_vy(&source);
  destination.vz = mavlink_msg_local_position_ned_get_vz(&source);
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::GlobalPositionInt>::convertFrom(const Converted &source, Storage &destination)
{
  destination.time_boot_ms = mavlink_msg_global_position_int_get_time_boot_ms(&source);
  destination.lat = mavlink_msg_global_position_int_get_lat(&source);
  destination.lon = mavlink_msg_global_position_int_get_lon(&source);
  destination.alt = mavlink_msg_global_position_int_get_alt(&source);
  destination.relative_alt = mavlink_msg_global_position_int_get_relative_alt(&source);
  destination.vx = mavlink_msg_global_position_int_get_vx(&source);
  destination.vy = mavlink_msg_global_position_int_get_vy(&source);
  destination.vz = mavlink_msg_global_position_int_get_vz(&source);
  destination.hdg = mavlink_msg_global_position_int_get_hdg(&source);
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::GpsGlobalOrigin>::convertFrom(const Converted &source, Storage &destination)
{
  destination.latitude = mavlink_msg_gps_global_origin_get_latitude(&source);
  destination.longitude = mavlink_msg_gps_global_origin_get_longitude(&source);
  destination.altitude = mavlink_msg_gps_global_origin_get_altitude(&source);
  destination.time_usec = mavlink_msg_gps_global_origin_get_time_usec(&source);
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::NavControllerOutput>::convertFrom(const Converted &source, Storage &destination)
{
  destination.nav_roll = mavlink_msg_nav_controller_output_get_nav_roll(&source);
  destination.nav_pitch = mavlink_msg_nav_controller_output_get_nav_pitch(&source);
  destination.nav_bearing = mavlink_msg_nav_controller_output_get_nav_bearing(&source);
  destination.target_bearing = mavlink_msg_nav_controller_output_get_target_bearing(&source);
  destination.wp_dist = mavlink_msg_nav_controller_output_get_wp_dist(&source);
  destination.alt_error = mavlink_msg_nav_controller_output_get_alt_error(&source);
  destination.aspd_error = mavlink_msg_nav_controller_output_get_aspd_error(&source);
  destination.xtrack_error = mavlink_msg_nav_controller_output_get_xtrack_error(&source);
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::RcChannelsRaw>::convertFrom(const Converted &source, Storage &destination)
{
  destination.time_boot_ms = mavlink_msg_rc_channels_raw_get_time_boot_ms(&source);
  destination.chan1_raw = mavlink_msg_rc_channels_raw_get_chan1_raw(&source);
  destination.chan2_raw = mavlink_msg_rc_channels_raw_get_chan2_raw(&source);
  destination.chan3_raw = mavlink_msg_rc_channels_raw_get_chan3_raw(&source);
  destination.chan4_raw = mavlink_msg_rc_channels_raw_get_chan4_raw(&source);
  destination.chan5_raw = mavlink_msg_rc_channels_raw_get_chan5_raw(&source);
  destination.chan6_raw = mavlink_msg_rc_channels_raw_get_chan6_raw(&source);
  destination.chan7_raw = mavlink_msg_rc_channels_raw_get_chan7_raw(&source);
  destination.chan8_raw = mavlink_msg_rc_channels_raw_get_chan8_raw(&source);
  destination.port = mavlink_msg_rc_channels_raw_get_port(&source);
  destination.rssi = mavlink_msg_rc_channels_raw_get_rssi(&source);
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::RcChannels>::convertFrom(const Converted &source, Storage &destination)
{
  destination.time_boot_ms = mavlink_msg_rc_channels_get_time_boot_ms(&source);
  destination.chan1_raw = mavlink_msg_rc_channels_get_chan1_raw(&source);
  destination.chan2_raw = mavlink_msg_rc_channels_get_chan2_raw(&source);
  destination.chan3_raw = mavlink_msg_rc_channels_get_chan3_raw(&source);
  destination.chan4_raw = mavlink_msg_rc_channels_get_chan4_raw(&source);
  destination.chan5_raw = mavlink_msg_rc_channels_get_chan5_raw(&source);
  destination.chan6_raw = mavlink_msg_rc_channels_get_chan6_raw(&source);
  destination.chan7_raw = mavlink_msg_rc_channels_get_chan7_raw(&source);
  destination.chan8_raw = mavlink_msg_rc_channels_get_chan8_raw(&source);
  destination.chan9_raw = mavlink_msg_rc_channels_get_chan9_raw(&source);
  destination.chan10_raw = mavlink_msg_rc_channels_get_chan10_raw(&source);
  destination.chan11_raw = mavlink_msg_rc_channels_get_chan11_raw(&source);
  destination.chan12_raw = mavlink_msg_rc_channels_get_chan12_raw(&source);
  destination.chan13_raw = mavlink_msg_rc_channels_get_chan13_raw(&source);
  destination.chan14_raw = mavlink_msg_rc_channels_get_chan14_raw(&source);
  destination.chan15_raw = mavlink_msg_rc_channels_get_chan15_raw(&source);
  destination.chan16_raw = mavlink_msg_rc_channels_get_chan16_raw(&source);
  destination.chan17_raw = mavlink_msg_rc_channels_get_chan17_raw(&source);
  destination.chan18_raw = mavlink_msg_rc_channels_get_chan18_raw(&source);
  destination.chancount = mavlink_msg_rc_channels_get_chancount(&source);
  destination.rssi = mavlink_msg_rc_channels_get_rssi(&source);
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::RcChannelsOverride>::convertFrom(const Converted &source, Storage &destination)
{
  destination.target_system = mavlink_msg_rc_channels_override_get_target_system(&source);
  destination.target_component = mavlink_msg_rc_channels_override_get_target_component(&source);
  destination.chan1_raw = mavlink_msg_rc_channels_override_get_chan1_raw(&source);
  destination.chan2_raw = mavlink_msg_rc_channels_override_get_chan2_raw(&source);
  destination.chan3_raw = mavlink_msg_rc_channels_override_get_chan3_raw(&source);
  destination.chan4_raw = mavlink_msg_rc_channels_override_get_chan4_raw(&source);
  destination.chan5_raw = mavlink_msg_rc_channels_override_get_chan5_raw(&source);
  destination.chan6_raw = mavlink_msg_rc_channels_override_get_chan6_raw(&source);
  destination.chan7_raw = mavlink_msg_rc_channels_override_get_chan7_raw(&source);
  destination.chan8_raw = mavlink_msg_rc_channels_override_get_chan8_raw(&source);
  destination.chan9_raw = mavlink_msg_rc_channels_override_get_chan9_raw(&source);
  destination.chan10_raw = mavlink_msg_rc_channels_override_get_chan10_raw(&source);
  destination.chan11_raw = mavlink_msg_rc_channels_override_get_chan11_raw(&source);
  destination.chan12_raw = mavlink_msg_rc_channels_override_get_chan12_raw(&source);
  destination.chan13_raw = mavlink_msg_rc_channels_override_get_chan13_raw(&source);
  destination.chan14_raw = mavlink_msg_rc_channels_override_get_chan14_raw(&source);
  destination.chan15_raw = mavlink_msg_rc_channels_override_get_chan15_raw(&source);
  destination.chan16_raw = mavlink_msg_rc_channels_override_get_chan16_raw(&source);
  destination.chan17_raw = mavlink_msg_rc_channels_override_get_chan17_raw(&source);
  destination.chan18_raw = mavlink_msg_rc_channels_override_get_chan18_raw(&source);
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::ServoOutputRaw>::convertFrom(const Converted &source, Storage &destination)
{
  destination.time_usec = mavlink_msg_servo_output_raw_get_time_usec(&source);
  destination.servo1_raw = mavlink_msg_servo_output_raw_get_servo1_raw(&source);
  destination.servo2_raw = mavlink_msg_servo_output_raw_get_servo2_raw(&source);
  destination.servo3_raw = mavlink_msg_servo_output_raw_get_servo3_raw(&source);
  destination.servo4_raw = mavlink_msg_servo_output_raw_get_servo4_raw(&source);
  destination.servo5_raw = mavlink_msg_servo_output_raw_get_servo5_raw(&source);
  destination.servo6_raw = mavlink_msg_servo_output_raw_get_servo6_raw(&source);
  destination.servo7_raw = mavlink_msg_servo_output_raw_get_servo7_raw(&source);
  destination.servo8_raw = mavlink_msg_servo_output_raw_get_servo8_raw(&source);
  destination.port = mavlink_msg_servo_output_raw_get_port(&source);
  destination.servo9_raw = mavlink_msg_servo_output_raw_get_servo9_raw(&source);
  destination.servo10_raw = mavlink_msg_servo_output_raw_get_servo10_raw(&source);
  destination.servo11_raw = mavlink_msg_servo_output_raw_get_servo11_raw(&source);
  destination.servo12_raw = mavlink_msg_servo_output_raw_get_servo12_raw(&source);
  destination.servo13_raw = mavlink_msg_servo_output_raw_get_servo13_raw(&source);
  destination.servo14_raw = mavlink_msg_servo_output_raw_get_servo14_raw(&source);
  destination.servo15_raw = mavlink_msg_servo_output_raw_get_servo15_raw(&source);
  destination.servo16_raw = mavlink_msg_servo_output_raw_get_servo16_raw(&source);
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::VfrHud>::convertFrom(const Converted &source, Storage &destination)
{
  destination.airspeed = mavlink_msg_vfr_hud_get_airspeed(&source);
  destination.groundspeed = mavlink_msg_vfr_hud_get_groundspeed(&source);
  destination.alt = mavlink_msg_vfr_hud_get_alt(&source);
  destination.climb = mavlink_msg_vfr_hud_get_climb(&source);
  destination.heading = mavlink_msg_vfr_hud_get_heading(&source);
  destination.throttle = mavlink_msg_vfr_hud_get_throttle(&source);
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::CommandInt>::convertFrom(const Converted &source, Storage &destination)
{
  destination.param1 = mavlink_msg_command_int_get_param1(&source);
  destination.param2 = mavlink_msg_command_int_get_param2(&source);
  destination.param3 = mavlink_msg_command_int_get_param3(&source);
  destination.param4 = mavlink_msg_command_int_get_param4(&source);
  destination.x = mavlink_msg_command_int_get_x(&source);
  destination.y = mavlink_msg_command_int_get_y(&source);
  destination.z = mavlink_msg_command_int_get_z(&source);
  destination.command = static_cast<mavlink::common::Cmd>(mavlink_msg_command_int_get_command(&source));
  destination.target_system = mavlink_msg_command_int_get_target_system(&source);
  destination.target_component = mavlink_msg_command_int_get_target_component(&source);
  destination.frame = static_cast<mavlink::common::Frame>(mavlink_msg_command_int_get_frame(&source));
  destination.current = mavlink_msg_command_int_get_current(&source);
  destination.autocontinue = mavlink_msg_command_int_get_autocontinue(&source);
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::CommandLong>::convertFrom(const Converted &source, Storage &destination)
{
  destination.param1 = mavlink_msg_command_long_get_param1(&source);
  destination.param2 = mavlink_msg_command_long_get_param2(&source);
  destination.param3 = mavlink_msg_command_long_get_param3(&source);
  destination.param4 = mavlink_msg_command_long_get_param4(&source);
  destination.param5 = mavlink_msg_command_long_get_param5(&source);
  destination.param6 = mavlink_msg_command_long_get_param6(&source);
  destination.param7 = mavlink_msg_command_long_get_param7(&source);
  destination.command = static_cast<mavlink::common::Cmd>(mavlink_msg_command_long_get_command(&source));
  destination.target_system = mavlink_msg_command_long_get_target_system(&source);
  destination.target_component = mavlink_msg_command_long_get_target_component(&source);
  destination.confirmation = mavlink_msg_command_long_get_confirmation(&source);
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::CommandAck>::convertFrom(const Converted &source, Storage &destination)
{
  destination.command = static_cast<mavlink::common::Cmd>(mavlink_msg_command_ack_get_command(&source));
  destination.result = static_cast<mavlink::common::Result>(mavlink_msg_command_ack_get_result(&source));
  destination.progress = mavlink_msg_command_ack_get_progress(&source);
  destination.result_param2 = mavlink_msg_command_ack_get_result_param2(&source);
  destination.target_system = mavlink_msg_command_ack_get_target_system(&source);
  destination.target_component = mavlink_msg_command_ack_get_target_component(&source);
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::PositionTargetLocalNed>::convertFrom(const Converted &source, Storage &destination)
{
  destination.time_boot_ms = mavlink_msg_position_target_local_ned_get_time_boot_ms(&source);
  destination.x = mavlink_msg_position_target_local_ned_get_x(&source);
  destination.y = mavlink_msg_position_target_local_ned_get_y(&source);
  destination.z = mavlink_msg_position_target_local_ned_get_z(&source);
  destination.vx = mavlink_msg_position_target_local_ned_get_vx(&source);
  destination.vy = mavlink_msg_position_target_local_ned_get_vy(&source);
  destination.vz = mavlink_msg_position_target_local_ned_get_vz(&source);
  destination.afx = mavlink_msg_position_target_local_ned_get_afx(&source);
  destination.afy = mavlink_msg_position_target_local_ned_get_afy(&source);
  destination.afz = mavlink_msg_position_target_local_ned_get_afz(&source);
  destination.yaw = mavlink_msg_position_target_local_ned_get_yaw(&source);
  destination.yaw_rate = mavlink_msg_position_target_local_ned_get_yaw_rate(&source);
  destination.type_mask =
    static_cast<mavlink::common::PositionTargetTypemask>(mavlink_msg_position_target_local_ned_get_type_mask(&source));
  destination.coordinate_frame = static_cast<mavlink::common::Frame>(mavlink_msg_position_target_local_ned_get_coordinate_frame(&source));
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::AttitudeTarget>::convertFrom(const Converted &source, Storage &destination)
{
  destination.time_boot_ms = mavlink_msg_attitude_target_get_time_boot_ms(&source);

  destination.q.resize(4);
  mavlink_msg_attitude_target_get_q(&source, destination.q.data());

  destination.body_roll_rate = mavlink_msg_attitude_target_get_body_roll_rate(&source);
  destination.body_pitch_rate = mavlink_msg_attitude_target_get_body_pitch_rate(&source);
  destination.body_yaw_rate = mavlink_msg_attitude_target_get_body_yaw_rate(&source);
  destination.thrust = mavlink_msg_attitude_target_get_thrust(&source);
  destination.type_mask = static_cast<mavlink::common::AttitudeTargetTypemask>(mavlink_msg_attitude_target_get_type_mask(&source));
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::PositionTargetGlobalInt>::convertFrom(const Converted &source, Storage &destination)
{
  destination.time_boot_ms = mavlink_msg_position_target_global_int_get_time_boot_ms(&source);
  destination.lat_int = mavlink_msg_position_target_global_int_get_lat_int(&source);
  destination.lon_int = mavlink_msg_position_target_global_int_get_lon_int(&source);
  destination.alt = mavlink_msg_position_target_global_int_get_alt(&source);
  destination.vx = mavlink_msg_position_target_global_int_get_vx(&source);
  destination.vy = mavlink_msg_position_target_global_int_get_vy(&source);
  destination.vz = mavlink_msg_position_target_global_int_get_vz(&source);
  destination.afx = mavlink_msg_position_target_global_int_get_afx(&source);
  destination.afy = mavlink_msg_position_target_global_int_get_afy(&source);
  destination.afz = mavlink_msg_position_target_global_int_get_afz(&source);
  destination.yaw = mavlink_msg_position_target_global_int_get_yaw(&source);
  destination.yaw_rate = mavlink_msg_position_target_global_int_get_yaw_rate(&source);
  destination.type_mask =
    static_cast<mavlink::common::PositionTargetTypemask>(mavlink_msg_position_target_global_int_get_type_mask(&source));
  destination.coordinate_frame = static_cast<mavlink::common::Frame>(mavlink_msg_position_target_global_int_get_coordinate_frame(&source));
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::LocalPositionNedSystemGlobalOffset>::convertFrom(
  const Converted &source,
  Storage &destination
)
{
  destination.time_boot_ms = mavlink_msg_local_position_ned_system_global_offset_get_time_boot_ms(&source);
  destination.x = mavlink_msg_local_position_ned_system_global_offset_get_x(&source);
  destination.y = mavlink_msg_local_position_ned_system_global_offset_get_y(&source);
  destination.z = mavlink_msg_local_position_ned_system_global_offset_get_z(&source);
  destination.roll = mavlink_msg_local_position_ned_system_global_offset_get_roll(&source);
  destination.pitch = mavlink_msg_local_position_ned_system_global_offset_get_pitch(&source);
  destination.yaw = mavlink_msg_local_position_ned_system_global_offset_get_yaw(&source);
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::HighresImu>::convertFrom(const Converted &source, Storage &destination)
{
  destination.time_usec = mavlink_msg_highres_imu_get_time_usec(&source);
  destination.xacc = mavlink_msg_highres_imu_get_xacc(&source);
  destination.yacc = mavlink_msg_highres_imu_get_yacc(&source);
  destination.zacc = mavlink_msg_highres_imu_get_zacc(&source);
  destination.xgyro = mavlink_msg_highres_imu_get_xgyro(&source);
  destination.ygyro = mavlink_msg_highres_imu_get_ygyro(&source);
  destination.zgyro = mavlink_msg_highres_imu_get_zgyro(&source);
  destination.xmag = mavlink_msg_highres_imu_get_xmag(&source);
  destination.ymag = mavlink_msg_highres_imu_get_ymag(&source);
  destination.zmag = mavlink_msg_highres_imu_get_zmag(&source);
  destination.abs_pressure = mavlink_msg_highres_imu_get_abs_pressure(&source);
  destination.diff_pressure = mavlink_msg_highres_imu_get_diff_pressure(&source);
  destination.pressure_alt = mavlink_msg_highres_imu_get_pressure_alt(&source);
  destination.temperature = mavlink_msg_highres_imu_get_temperature(&source);
  destination.fields_updated = static_cast<mavlink::common::HighresImuUpdatedFlags>(mavlink_msg_highres_imu_get_fields_updated(&source));
  destination.id = mavlink_msg_highres_imu_get_id(&source);
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::Timesync>::convertFrom(const Converted &source, Storage &destination)
{
  destination.tc1 = mavlink_msg_timesync_get_tc1(&source);
  destination.ts1 = mavlink_msg_timesync_get_ts1(&source);
  destination.target_system = mavlink_msg_timesync_get_target_system(&source);
  destination.target_component = mavlink_msg_timesync_get_target_component(&source);
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::ActuatorControlTarget>::convertFrom(const Converted &source, Storage &destination)
{
  destination.time_usec = mavlink_msg_actuator_control_target_get_time_usec(&source);

  destination.controls.resize(8);
  mavlink_msg_actuator_control_target_get_controls(&source, destination.controls.data());

  destination.group_mlx = mavlink_msg_actuator_control_target_get_group_mlx(&source);
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::Altitude>::convertFrom(const Converted &source, Storage &destination)
{
  destination.time_usec = mavlink_msg_altitude_get_time_usec(&source);
  destination.altitude_monotonic = mavlink_msg_altitude_get_altitude_monotonic(&source);
  destination.altitude_amsl = mavlink_msg_altitude_get_altitude_amsl(&source);
  destination.altitude_local = mavlink_msg_altitude_get_altitude_local(&source);
  destination.altitude_relative = mavlink_msg_altitude_get_altitude_relative(&source);
  destination.altitude_terrain = mavlink_msg_altitude_get_altitude_terrain(&source);
  destination.bottom_clearance = mavlink_msg_altitude_get_bottom_clearance(&source);
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::BatteryStatus>::convertFrom(const Converted &source, Storage &destination)
{
  destination.current_consumed = mavlink_msg_battery_status_get_current_consumed(&source);
  destination.energy_consumed = mavlink_msg_battery_status_get_energy_consumed(&source);
  destination.temperature = mavlink_msg_battery_status_get_temperature(&source);

  destination.voltages.resize(10);
  mavlink_msg_battery_status_get_voltages(&source, destination.voltages.data());

  destination.current_battery = mavlink_msg_battery_status_get_current_battery(&source);
  destination.id = mavlink_msg_battery_status_get_id(&source);
  destination.battery_function = static_cast<mavlink::common::BatteryFunction>(mavlink_msg_battery_status_get_battery_function(&source));
  destination.type = static_cast<mavlink::common::BatteryType>(mavlink_msg_battery_status_get_type(&source));
  destination.battery_remaining = mavlink_msg_battery_status_get_battery_remaining(&source);
  destination.time_remaining = mavlink_msg_battery_status_get_time_remaining(&source);
  destination.charge_state = static_cast<mavlink::common::BatteryChargeState>(mavlink_msg_battery_status_get_charge_state(&source));

  destination.voltages_ext.resize(4);
  mavlink_msg_battery_status_get_voltages_ext(&source, destination.voltages_ext.data());

  destination.mode = static_cast<mavlink::common::BatteryMode>(mavlink_msg_battery_status_get_mode(&source));
  destination.fault_bitmask = static_cast<mavlink::common::BatteryFault>(mavlink_msg_battery_status_get_fault_bitmask(&source));
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::AutopilotVersion>::convertFrom(const Converted &source, Storage &destination)
{
  destination.capabilities = static_cast<mavlink::common::ProtocolCapability>(mavlink_msg_autopilot_version_get_capabilities(&source));
  destination.uid = mavlink_msg_autopilot_version_get_uid(&source);
  destination.flight_sw_version = mavlink_msg_autopilot_version_get_flight_sw_version(&source);
  destination.middleware_sw_version = mavlink_msg_autopilot_version_get_middleware_sw_version(&source);
  destination.os_sw_version = mavlink_msg_autopilot_version_get_os_sw_version(&source);
  destination.board_version = mavlink_msg_autopilot_version_get_board_version(&source);
  destination.vendor_id = mavlink_msg_autopilot_version_get_vendor_id(&source);
  destination.product_id = mavlink_msg_autopilot_version_get_product_id(&source);

  destination.flight_custom_version.resize(8);
  mavlink_msg_autopilot_version_get_flight_custom_version(&source, destination.flight_custom_version.data());

  destination.middleware_custom_version.resize(8);
  mavlink_msg_autopilot_version_get_middleware_custom_version(&source, destination.middleware_custom_version.data());

  destination.os_custom_version.resize(8);
  mavlink_msg_autopilot_version_get_os_custom_version(&source, destination.os_custom_version.data());

  destination.uid2.resize(18);
  mavlink_msg_autopilot_version_get_uid2(&source, destination.uid2.data());
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::EstimatorStatus>::convertFrom(const Converted &source, Storage &destination)
{
  destination.time_usec = mavlink_msg_estimator_status_get_time_usec(&source);
  destination.vel_ratio = mavlink_msg_estimator_status_get_vel_ratio(&source);
  destination.pos_horiz_ratio = mavlink_msg_estimator_status_get_pos_horiz_ratio(&source);
  destination.pos_vert_ratio = mavlink_msg_estimator_status_get_pos_vert_ratio(&source);
  destination.mag_ratio = mavlink_msg_estimator_status_get_mag_ratio(&source);
  destination.hagl_ratio = mavlink_msg_estimator_status_get_hagl_ratio(&source);
  destination.tas_ratio = mavlink_msg_estimator_status_get_tas_ratio(&source);
  destination.pos_horiz_accuracy = mavlink_msg_estimator_status_get_pos_horiz_accuracy(&source);
  destination.pos_vert_accuracy = mavlink_msg_estimator_status_get_pos_vert_accuracy(&source);
  destination.flags = static_cast<mavlink::common::EstimatorStatusFlags>(mavlink_msg_estimator_status_get_flags(&source));
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::Vibration>::convertFrom(const Converted &source, Storage &destination)
{
  destination.time_usec = mavlink_msg_vibration_get_time_usec(&source);
  destination.vibration_x = mavlink_msg_vibration_get_vibration_x(&source);
  destination.vibration_y = mavlink_msg_vibration_get_vibration_y(&source);
  destination.vibration_z = mavlink_msg_vibration_get_vibration_z(&source);
  destination.clipping_0 = mavlink_msg_vibration_get_clipping_0(&source);
  destination.clipping_1 = mavlink_msg_vibration_get_clipping_1(&source);
  destination.clipping_2 = mavlink_msg_vibration_get_clipping_2(&source);
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::HomePosition>::convertFrom(const Converted &source, Storage &destination)
{
  destination.latitude = mavlink_msg_home_position_get_latitude(&source);
  destination.longitude = mavlink_msg_home_position_get_longitude(&source);
  destination.altitude = mavlink_msg_home_position_get_altitude(&source);
  destination.x = mavlink_msg_home_position_get_x(&source);
  destination.y = mavlink_msg_home_position_get_y(&source);
  destination.z = mavlink_msg_home_position_get_z(&source);

  destination.q.resize(4);
  mavlink_msg_home_position_get_q(&source, destination.q.data());

  destination.approach_x = mavlink_msg_home_position_get_approach_x(&source);
  destination.approach_y = mavlink_msg_home_position_get_approach_y(&source);
  destination.approach_z = mavlink_msg_home_position_get_approach_z(&source);
  destination.time_usec = mavlink_msg_home_position_get_time_usec(&source);
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::ExtendedSysState>::convertFrom(const Converted &source, Storage &destination)
{
  destination.vtol_state = static_cast<mavlink::common::VtolState>(mavlink_msg_extended_sys_state_get_vtol_state(&source));
  destination.landed_state = static_cast<mavlink::common::LandedState>(mavlink_msg_extended_sys_state_get_landed_state(&source));
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::EscInfo>::convertFrom(const Converted &source, Storage &destination)
{
  destination.time_usec = mavlink_msg_esc_info_get_time_usec(&source);

  destination.error_count.resize(4);
  mavlink_msg_esc_info_get_error_count(&source, destination.error_count.data());

  destination.counter = mavlink_msg_esc_info_get_counter(&source);

  destination.failure_flags.resize(4);
  mavlink_msg_esc_info_get_failure_flags(&source, reinterpret_cast<u16 *>(destination.failure_flags.data()));

  destination.temperature.resize(4);
  mavlink_msg_esc_info_get_temperature(&source, destination.temperature.data());

  destination.index = mavlink_msg_esc_info_get_index(&source);
  destination.count = mavlink_msg_esc_info_get_count(&source);
  destination.connection_type = static_cast<mavlink::common::EscConnectionType>(mavlink_msg_esc_info_get_connection_type(&source));
  destination.info = mavlink_msg_esc_info_get_info(&source);
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::EscStatus>::convertFrom(const Converted &source, Storage &destination)
{
  destination.time_usec = mavlink_msg_esc_status_get_time_usec(&source);

  destination.rpm.resize(4);
  mavlink_msg_esc_status_get_rpm(&source, destination.rpm.data());

  destination.voltage.resize(4);
  mavlink_msg_esc_status_get_voltage(&source, destination.voltage.data());

  destination.current.resize(4);
  mavlink_msg_esc_status_get_current(&source, destination.current.data());

  destination.index = mavlink_msg_esc_status_get_index(&source);
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::MissionCurrent>::convertFrom(const Converted &source, Storage &destination)
{
  destination.seq = mavlink_msg_mission_current_get_seq(&source);
  destination.total = mavlink_msg_mission_current_get_total(&source);
  destination.mission_state = static_cast<mavlink::common::MissionState>(mavlink_msg_mission_current_get_mission_state(&source));
  destination.mission_mode = mavlink_msg_mission_current_get_mission_mode(&source);
  destination.mission_id = mavlink_msg_mission_current_get_mission_id(&source);
  destination.fence_id = mavlink_msg_mission_current_get_fence_id(&source);
  destination.rally_points_id = mavlink_msg_mission_current_get_rally_points_id(&source);
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::MissionRequestList>::convertFrom(const Converted &source, Storage &destination)
{
  destination.target_system = mavlink_msg_mission_request_list_get_target_system(&source);
  destination.target_component = mavlink_msg_mission_request_list_get_target_component(&source);
  destination.mission_type = static_cast<mavlink::common::MissionType>(mavlink_msg_mission_request_list_get_mission_type(&source));
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::MissionRequestInt>::convertFrom(const Converted &source, Storage &destination)
{
  destination.target_system = mavlink_msg_mission_request_int_get_target_system(&source);
  destination.target_component = mavlink_msg_mission_request_int_get_target_component(&source);
  destination.seq = mavlink_msg_mission_request_int_get_seq(&source);
  destination.mission_type = static_cast<mavlink::common::MissionType>(mavlink_msg_mission_request_int_get_mission_type(&source));
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::MissionCount>::convertFrom(const Converted &source, Storage &destination)
{
  destination.target_system = mavlink_msg_mission_count_get_target_system(&source);
  destination.target_component = mavlink_msg_mission_count_get_target_component(&source);
  destination.count = mavlink_msg_mission_count_get_count(&source);
  destination.mission_type = static_cast<mavlink::common::MissionType>(mavlink_msg_mission_count_get_mission_type(&source));
  destination.opaque_id = mavlink_msg_mission_count_get_opaque_id(&source);
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::MissionClearAll>::convertFrom(const Converted &source, Storage &destination)
{
  destination.target_system = mavlink_msg_mission_clear_all_get_target_system(&source);
  destination.target_component = mavlink_msg_mission_clear_all_get_target_component(&source);
  destination.mission_type = static_cast<mavlink::common::MissionType>(mavlink_msg_mission_clear_all_get_mission_type(&source));
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::MissionItemReached>::convertFrom(const Converted &source, Storage &destination)
{
  destination.seq = mavlink_msg_mission_item_reached_get_seq(&source);
  ;
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::MissionItemInt>::convertFrom(const Converted &source, Storage &destination)
{
  destination.target_system = mavlink_msg_mission_item_int_get_target_system(&source);
  destination.target_component = mavlink_msg_mission_item_int_get_target_component(&source);
  destination.seq = mavlink_msg_mission_item_int_get_seq(&source);
  destination.frame = static_cast<mavlink::common::Frame>(mavlink_msg_mission_item_int_get_frame(&source));
  destination.command = static_cast<mavlink::common::Cmd>(mavlink_msg_mission_item_int_get_command(&source));
  destination.current = mavlink_msg_mission_item_int_get_current(&source);
  destination.autocontinue = mavlink_msg_mission_item_int_get_autocontinue(&source);
  destination.param1 = mavlink_msg_mission_item_int_get_param1(&source);
  destination.param2 = mavlink_msg_mission_item_int_get_param2(&source);
  destination.param3 = mavlink_msg_mission_item_int_get_param3(&source);
  destination.param4 = mavlink_msg_mission_item_int_get_param4(&source);
  destination.x = mavlink_msg_mission_item_int_get_x(&source);
  destination.y = mavlink_msg_mission_item_int_get_y(&source);
  destination.z = mavlink_msg_mission_item_int_get_z(&source);
  destination.mission_type = static_cast<mavlink::common::MissionType>(mavlink_msg_mission_item_int_get_mission_type(&source));
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::MissionAck>::convertFrom(const Converted &source, Storage &destination)
{
  destination.target_system = mavlink_msg_mission_count_get_target_system(&source);
  destination.target_component = mavlink_msg_mission_count_get_target_component(&source);
  destination.type = static_cast<mavlink::common::MissionResult>(mavlink_msg_mission_count_get_count(&source));
  destination.mission_type = static_cast<mavlink::common::MissionType>(mavlink_msg_mission_count_get_mission_type(&source));
  destination.opaque_id = mavlink_msg_mission_count_get_opaque_id(&source);
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::Odometry>::convertFrom(const Converted &source, Storage &destination)
{
  destination.x = mavlink_msg_odometry_get_x(&source);
  destination.y = mavlink_msg_odometry_get_y(&source);
  destination.z = mavlink_msg_odometry_get_z(&source);

  destination.q.resize(4);
  mavlink_msg_odometry_get_q(&source, destination.q.data());

  destination.vx = mavlink_msg_odometry_get_vx(&source);
  destination.vy = mavlink_msg_odometry_get_vy(&source);
  destination.vz = mavlink_msg_odometry_get_vz(&source);
  destination.rollspeed = mavlink_msg_odometry_get_rollspeed(&source);
  destination.pitchspeed = mavlink_msg_odometry_get_pitchspeed(&source);
  destination.yawspeed = mavlink_msg_odometry_get_yawspeed(&source);

  destination.pose_covariance.resize(21);
  mavlink_msg_odometry_get_pose_covariance(&source, destination.pose_covariance.data());

  destination.velocity_covariance.resize(21);
  mavlink_msg_odometry_get_velocity_covariance(&source, destination.velocity_covariance.data());

  destination.frame_id = static_cast<mavlink::common::Frame>(mavlink_msg_odometry_get_frame_id(&source));
  destination.child_frame_id = static_cast<mavlink::common::Frame>(mavlink_msg_odometry_get_child_frame_id(&source));
  destination.reset_counter = mavlink_msg_odometry_get_reset_counter(&source);
  destination.estimator_type = static_cast<mavlink::common::EstimatorType>(mavlink_msg_odometry_get_estimator_type(&source));
  destination.quality = mavlink_msg_odometry_get_quality(&source);
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::UtmGlobalPosition>::convertFrom(const Converted &source, Storage &destination)
{
  destination.time = mavlink_msg_utm_global_position_get_time(&source);
  destination.lat = mavlink_msg_utm_global_position_get_lat(&source);
  destination.lon = mavlink_msg_utm_global_position_get_lon(&source);
  destination.alt = mavlink_msg_utm_global_position_get_alt(&source);
  destination.relative_alt = mavlink_msg_utm_global_position_get_relative_alt(&source);
  destination.next_lat = mavlink_msg_utm_global_position_get_next_lat(&source);
  destination.next_lon = mavlink_msg_utm_global_position_get_next_lon(&source);
  destination.next_alt = mavlink_msg_utm_global_position_get_next_alt(&source);
  destination.vx = mavlink_msg_utm_global_position_get_vx(&source);
  destination.vy = mavlink_msg_utm_global_position_get_vy(&source);
  destination.vz = mavlink_msg_utm_global_position_get_vz(&source);
  destination.h_acc = mavlink_msg_utm_global_position_get_h_acc(&source);
  destination.v_acc = mavlink_msg_utm_global_position_get_v_acc(&source);
  destination.vel_acc = mavlink_msg_utm_global_position_get_vel_acc(&source);
  destination.update_rate = mavlink_msg_utm_global_position_get_update_rate(&source);

  destination.uas_id.resize(18);
  mavlink_msg_utm_global_position_get_uas_id(&source, destination.uas_id.data());

  destination.flight_state = static_cast<mavlink::common::UtmFlightState>(mavlink_msg_utm_global_position_get_flight_state(&source));
  destination.flags = static_cast<mavlink::common::UtmDataAvailFlags>(mavlink_msg_utm_global_position_get_flags(&source));
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::TimeEstimateToTarget>::convertFrom(const Converted &source, Storage &destination)
{
  destination.safe_return = mavlink_msg_time_estimate_to_target_get_safe_return(&source);
  destination.land = mavlink_msg_time_estimate_to_target_get_land(&source);
  destination.mission_next_item = mavlink_msg_time_estimate_to_target_get_mission_next_item(&source);
  destination.mission_end = mavlink_msg_time_estimate_to_target_get_mission_end(&source);
  destination.commanded_action = mavlink_msg_time_estimate_to_target_get_commanded_action(&source);
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::CurrentEventSequence>::convertFrom(const Converted &source, Storage &destination)
{
  destination.sequence = mavlink_msg_current_event_sequence_get_sequence(&source);
  destination.flags = static_cast<mavlink::common::EventCurrentSequenceFlags>(mavlink_msg_current_event_sequence_get_flags(&source));
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::DistanceSensor>::convertFrom(const Converted &source, Storage &destination)
{
  destination.time_boot_ms = mavlink_msg_distance_sensor_get_time_boot_ms(&source);
  destination.min_distance = mavlink_msg_distance_sensor_get_min_distance(&source);
  destination.max_distance = mavlink_msg_distance_sensor_get_max_distance(&source);
  destination.current_distance = mavlink_msg_distance_sensor_get_current_distance(&source);
  destination.type = static_cast<mavlink::common::DistanceSensorType>(mavlink_msg_distance_sensor_get_type(&source));
  destination.id = mavlink_msg_distance_sensor_get_id(&source);
  destination.orientation = static_cast<mavlink::common::SensorOrientation>(mavlink_msg_distance_sensor_get_orientation(&source));
  destination.covariance = mavlink_msg_distance_sensor_get_covariance(&source);
  destination.horizontal_fov = mavlink_msg_distance_sensor_get_horizontal_fov(&source);
  destination.vertical_fov = mavlink_msg_distance_sensor_get_vertical_fov(&source);
  destination.signal_quality = mavlink_msg_distance_sensor_get_signal_quality(&source);

  destination.quaternion.resize(4);
  mavlink_msg_distance_sensor_get_quaternion(&source, destination.quaternion.data());
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::OpenDroneIdLocation>::convertFrom(const Converted &source, Storage &destination)
{
  destination.latitude = mavlink_msg_open_drone_id_location_get_latitude(&source);
  destination.longitude = mavlink_msg_open_drone_id_location_get_longitude(&source);
  destination.altitude_barometric = mavlink_msg_open_drone_id_location_get_altitude_barometric(&source);
  destination.altitude_geodetic = mavlink_msg_open_drone_id_location_get_altitude_geodetic(&source);
  destination.height = mavlink_msg_open_drone_id_location_get_height(&source);
  destination.timestamp = mavlink_msg_open_drone_id_location_get_timestamp(&source);
  destination.direction = mavlink_msg_open_drone_id_location_get_direction(&source);
  destination.speed_horizontal = mavlink_msg_open_drone_id_location_get_speed_horizontal(&source);
  destination.speed_vertical = mavlink_msg_open_drone_id_location_get_speed_vertical(&source);
  destination.target_system = mavlink_msg_open_drone_id_location_get_target_system(&source);
  destination.target_component = mavlink_msg_open_drone_id_location_get_target_component(&source);

  destination.id_or_mac.resize(20);
  mavlink_msg_open_drone_id_location_get_id_or_mac(&source, destination.id_or_mac.data());

  destination.status = static_cast<mavlink::common::OdidStatus>(mavlink_msg_open_drone_id_location_get_status(&source));
  destination.height_reference =
    static_cast<mavlink::common::OdidHeightRef>(mavlink_msg_open_drone_id_location_get_height_reference(&source));
  destination.horizontal_accuracy =
    static_cast<mavlink::common::OdidHorAcc>(mavlink_msg_open_drone_id_location_get_horizontal_accuracy(&source));
  destination.vertical_accuracy =
    static_cast<mavlink::common::OdidVerAcc>(mavlink_msg_open_drone_id_location_get_vertical_accuracy(&source));
  destination.barometer_accuracy =
    static_cast<mavlink::common::OdidVerAcc>(mavlink_msg_open_drone_id_location_get_barometer_accuracy(&source));
  destination.speed_accuracy = static_cast<mavlink::common::OdidSpeedAcc>(mavlink_msg_open_drone_id_location_get_speed_accuracy(&source));
  destination.timestamp_accuracy =
    static_cast<mavlink::common::OdidTimeAcc>(mavlink_msg_open_drone_id_location_get_timestamp_accuracy(&source));
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::OpenDroneIdSystem>::convertFrom(const Converted &source, Storage &destination)
{
  destination.operator_latitude = mavlink_msg_open_drone_id_system_get_operator_latitude(&source);
  destination.operator_longitude = mavlink_msg_open_drone_id_system_get_operator_longitude(&source);
  destination.area_ceiling = mavlink_msg_open_drone_id_system_get_area_ceiling(&source);
  destination.area_floor = mavlink_msg_open_drone_id_system_get_area_floor(&source);
  destination.operator_altitude_geo = mavlink_msg_open_drone_id_system_get_operator_altitude_geo(&source);
  destination.timestamp = mavlink_msg_open_drone_id_system_get_timestamp(&source);
  destination.area_count = mavlink_msg_open_drone_id_system_get_area_count(&source);
  destination.area_radius = mavlink_msg_open_drone_id_system_get_area_radius(&source);
  destination.target_system = mavlink_msg_open_drone_id_system_get_target_system(&source);
  destination.target_component = mavlink_msg_open_drone_id_system_get_target_component(&source);

  destination.id_or_mac.resize(20);
  mavlink_msg_open_drone_id_system_get_id_or_mac(&source, destination.id_or_mac.data());

  destination.operator_location_type =
    static_cast<mavlink::common::OdidOperatorLocationType>(mavlink_msg_open_drone_id_system_get_operator_location_type(&source));
  destination.classification_type =
    static_cast<mavlink::common::OdidClassificationType>(mavlink_msg_open_drone_id_system_get_classification_type(&source));
  destination.category_eu = static_cast<mavlink::common::OdidCategoryEu>(mavlink_msg_open_drone_id_system_get_category_eu(&source));
  destination.class_eu = static_cast<mavlink::common::OdidClassEu>(mavlink_msg_open_drone_id_system_get_class_eu(&source));
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::WindCov>::convertFrom(const Converted &source, Storage &destination)
{
  destination.time_usec = mavlink_msg_wind_cov_get_time_usec(&source);
  destination.wind_x = mavlink_msg_wind_cov_get_wind_x(&source);
  destination.wind_y = mavlink_msg_wind_cov_get_wind_y(&source);
  destination.wind_z = mavlink_msg_wind_cov_get_wind_z(&source);
  destination.var_horiz = mavlink_msg_wind_cov_get_var_horiz(&source);
  destination.var_vert = mavlink_msg_wind_cov_get_var_vert(&source);
  destination.wind_alt = mavlink_msg_wind_cov_get_wind_alt(&source);
  destination.horiz_accuracy = mavlink_msg_wind_cov_get_horiz_accuracy(&source);
  destination.vert_accuracy = mavlink_msg_wind_cov_get_vert_accuracy(&source);
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::Event>::convertFrom(const Converted &source, Storage &destination)
{
  destination.destination_component = mavlink_msg_event_get_destination_component(&source);
  destination.destination_system = mavlink_msg_event_get_destination_system(&source);
  destination.id = mavlink_msg_event_get_id(&source);
  destination.event_time_boot_ms = mavlink_msg_event_get_event_time_boot_ms(&source);
  destination.sequence = mavlink_msg_event_get_sequence(&source);
  destination.log_levels = mavlink_msg_event_get_log_levels(&source);

  destination.arguments.resize(40);
  mavlink_msg_event_get_arguments(&source, destination.arguments.data());
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::ParamRequestRead>::convertTo(
  const Storage &source,
  Converted &destination,
  const Mavlink::SystemInfo *info
)
{
  if (source.param_id.size() > 16) {
    throw ConversionException("The parameter ID string must not be larger than 16 characters.");
  }

  mavlink_msg_param_request_read_pack_chan(
    info->system_id,
    info->component_id,
    info->channel_id,
    &destination,
    source.target_system,
    source.target_component,
    source.param_id.c_str(),
    source.param_index
  );
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::SetPositionTargetLocalNed>::convertTo(
  const Storage &source,
  Converted &destination,
  const Mavlink::SystemInfo *info
)
{
  mavlink_msg_set_position_target_local_ned_pack_chan(
    info->system_id,
    info->component_id,
    info->channel_id,
    &destination,
    source.time_boot_ms,
    source.target_system,
    source.target_component,
    static_cast<u8>(source.coordinate_frame),
    static_cast<u16>(source.type_mask),
    source.x,
    source.y,
    source.z,
    source.vx,
    source.vy,
    source.vz,
    source.afx,
    source.afy,
    source.afz,
    source.yaw,
    source.yaw_rate
  );
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::CommandInt>::convertTo(
  const Storage &source,
  Converted &destination,
  const Mavlink::SystemInfo *info
)
{
  mavlink_msg_command_int_pack_chan(
    info->system_id,
    info->component_id,
    info->channel_id,
    &destination,
    source.target_system,
    source.target_component,
    static_cast<u8>(source.frame),
    static_cast<u16>(source.command),
    source.current,
    source.autocontinue,
    source.param1,
    source.param2,
    source.param3,
    source.param4,
    source.x,
    source.y,
    source.z
  );
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::CommandLong>::convertTo(
  const Storage &source,
  Converted &destination,
  const Mavlink::SystemInfo *info
)
{
  mavlink_msg_command_long_pack_chan(
    info->system_id,
    info->component_id,
    info->channel_id,
    &destination,
    source.target_system,
    source.target_component,
    static_cast<u16>(source.command),
    source.confirmation,
    source.param1,
    source.param2,
    source.param3,
    source.param4,
    source.param5,
    source.param6,
    source.param7
  );
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::Timesync>::convertTo(
  const Storage &source,
  Converted &destination,
  const Mavlink::SystemInfo *info
)
{
  mavlink_msg_timesync_pack_chan(
    info->system_id,
    info->component_id,
    info->channel_id,
    &destination,
    source.tc1,
    source.ts1,
    source.target_system,
    source.target_component
  );
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::SystemTime>::convertTo(
  const Storage &source,
  Converted &destination,
  const Mavlink::SystemInfo *info
)
{
  mavlink_msg_system_time_pack_chan(
    info->system_id, info->component_id, info->channel_id, &destination, source.time_unix_usec, source.time_boot_ms
  );
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::RcChannelsOverride>::convertTo(
  const Storage &source,
  Converted &destination,
  const Mavlink::SystemInfo *info
)
{
  mavlink_msg_rc_channels_override_pack_chan(
    info->system_id,
    info->component_id,
    info->channel_id,
    &destination,
    source.target_system,
    source.target_component,
    source.chan1_raw,
    source.chan2_raw,
    source.chan3_raw,
    source.chan4_raw,
    source.chan5_raw,
    source.chan6_raw,
    source.chan7_raw,
    source.chan8_raw,
    source.chan9_raw,
    source.chan10_raw,
    source.chan11_raw,
    source.chan12_raw,
    source.chan13_raw,
    source.chan14_raw,
    source.chan15_raw,
    source.chan16_raw,
    source.chan17_raw,
    source.chan18_raw
  );
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::MissionRequestList>::convertTo(
  const Storage &source,
  Converted &destination,
  const Mavlink::SystemInfo *info
)
{
  mavlink_msg_mission_request_list_pack_chan(
    info->system_id,
    info->component_id,
    info->channel_id,
    &destination,
    source.target_system,
    source.target_component,
    static_cast<u8>(source.mission_type)
  );
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::MissionRequestInt>::convertTo(
  const Storage &source,
  Converted &destination,
  const Mavlink::SystemInfo *info
)
{
  mavlink_msg_mission_request_int_pack_chan(
    info->system_id,
    info->component_id,
    info->channel_id,
    &destination,
    source.target_system,
    source.target_component,
    source.seq,
    static_cast<u8>(source.mission_type)
  );
}

template <>
void Mavlink::Node::MavlinkMessage<mavlink::common::MissionAck>::convertTo(
  const Storage &source,
  Converted &destination,
  const Mavlink::SystemInfo *info
)
{
  mavlink_msg_mission_ack_pack_chan(
    info->system_id,
    info->component_id,
    info->channel_id,
    &destination,
    source.target_system,
    source.target_component,
    static_cast<u8>(source.type),
    static_cast<u8>(source.mission_type),
    source.opaque_id
  );
}

}  // namespace lbot::plugins
}  // namespace labrat
