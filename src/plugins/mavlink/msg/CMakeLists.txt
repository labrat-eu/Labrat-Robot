cmake_minimum_required(VERSION 3.25.1)


find_package(Protobuf MODULE REQUIRED)

# Set the target name from the path.
get_filename_component(TARGET_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
get_filename_component(TARGET_NAME_PRIMARY ${TARGET_DIR} NAME)
get_filename_component(TARGET_NAME_SECONDARY ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TARGET_NAME ${TARGET_NAME_PRIMARY}_${TARGET_NAME_SECONDARY})

# Get the install path.
file(RELATIVE_PATH TARGET_RELATIVE_PATH ${PROJECT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})

set(TARGET_MESSAGES
  heartbeat.proto
  protocol_version.proto
  actuator_control_target.proto
  actuator_output_status.proto
  adsb_vehicle.proto
  ais_vessel.proto
  altitude.proto
  attitude.proto
  attitude_quaternion_cov.proto
  attitude_quaternion.proto
  attitude_target.proto
  att_pos_mocap.proto
  auth_key.proto
  autopilot_state_for_gimbal_device.proto
  autopilot_version.proto
  battery_status.proto
  button_change.proto
  camera_capture_status.proto
  camera_fov_status.proto
  camera_image_captured.proto
  camera_information.proto
  camera_settings.proto
  camera_tracking_geo_status.proto
  camera_tracking_image_status.proto
  camera_trigger.proto
  canfd_frame.proto
  can_filter_modify.proto
  can_frame.proto
  cellular_config.proto
  cellular_status.proto
  change_operator_control_ack.proto
  change_operator_control.proto
  collision.proto
  command_ack.proto
  command_cancel.proto
  command_int.proto
  command_long.proto
  component_information.proto
  component_metadata.proto
  control_system_state.proto
  current_event_sequence.proto
  data_stream.proto
  data_transmission_handshake.proto
  debug_float_array.proto
  debug.proto
  debug_vect.proto
  distance_sensor.proto
  efi_status.proto
  encapsulated_data.proto
  esc_info.proto
  esc_status.proto
  estimator_status.proto
  event.proto
  extended_sys_state.proto
  fence_status.proto
  file_transfer_protocol.proto
  flight_information.proto
  follow_target.proto
  generator_status.proto
  gimbal_device_attitude_status.proto
  gimbal_device_information.proto
  gimbal_device_set_attitude.proto
  gimbal_manager_information.proto
  gimbal_manager_set_attitude.proto
  gimbal_manager_set_manual_control.proto
  gimbal_manager_set_pitchyaw.proto
  gimbal_manager_status.proto
  global_position_int_cov.proto
  global_position_int.proto
  global_vision_position_estimate.proto
  gps2_raw.proto
  gps2_rtk.proto
  gps_global_origin.proto
  gps_inject_data.proto
  gps_input.proto
  gps_raw_int.proto
  gps_rtcm_data.proto
  gps_rtk.proto
  gps_status.proto
  high_latency2.proto
  high_latency.proto
  highres_imu.proto
  hil_actuator_controls.proto
  hil_controls.proto
  hil_gps.proto
  hil_optical_flow.proto
  hil_rc_inputs_raw.proto
  hil_sensor.proto
  hil_state.proto
  hil_state_quaternion.proto
  home_position.proto
  hygrometer_sensor.proto
  isbd_link_status.proto
  landing_target.proto
  link_node_status.proto
  local_position_ned_cov.proto
  local_position_ned.proto
  local_position_ned_system_global_offset.proto
  log_data.proto
  log_entry.proto
  log_erase.proto
  logging_ack.proto
  logging_data_acked.proto
  logging_data.proto
  log_request_data.proto
  log_request_end.proto
  log_request_list.proto
  mag_cal_report.proto
  manual_control.proto
  manual_setpoint.proto
  memory_vect.proto
  message_interval.proto
  mission_ack.proto
  mission_clear_all.proto
  mission_count.proto
  mission_current.proto
  mission_item_int.proto
  mission_item.proto
  mission_item_reached.proto
  mission_request_int.proto
  mission_request_list.proto
  mission_request_partial_list.proto
  mission_request.proto
  mission_set_current.proto
  mission_write_partial_list.proto
  mount_orientation.proto
  named_value_float.proto
  named_value_int.proto
  nav_controller_output.proto
  obstacle_distance.proto
  odometry.proto
  onboard_computer_status.proto
  open_drone_id_arm_status.proto
  open_drone_id_authentication.proto
  open_drone_id_basic_id.proto
  open_drone_id_location.proto
  open_drone_id_message_pack.proto
  open_drone_id_operator_id.proto
  open_drone_id_self_id.proto
  open_drone_id_system.proto
  open_drone_id_system_update.proto
  optical_flow.proto
  optical_flow_rad.proto
  orbit_execution_status.proto
  param_ext_ack.proto
  param_ext_request_list.proto
  param_ext_request_read.proto
  param_ext_set.proto
  param_ext_value.proto
  param_map_rc.proto
  param_request_list.proto
  param_request_read.proto
  param_set.proto
  param_value.proto
  ping.proto
  play_tune.proto
  play_tune_v2.proto
  position_target_global_int.proto
  position_target_local_ned.proto
  power_status.proto
  radio_status.proto
  raw_imu.proto
  raw_pressure.proto
  raw_rpm.proto
  rc_channels_override.proto
  rc_channels.proto
  rc_channels_raw.proto
  rc_channels_scaled.proto
  request_data_stream.proto
  request_event.proto
  resource_request.proto
  response_event_error.proto
  safety_allowed_area.proto
  safety_set_allowed_area.proto
  scaled_imu2.proto
  scaled_imu3.proto
  scaled_imu.proto
  scaled_pressure2.proto
  scaled_pressure3.proto
  scaled_pressure.proto
  serial_control.proto
  servo_output_raw.proto
  set_actuator_control_target.proto
  set_attitude_target.proto
  set_gps_global_origin.proto
  set_home_position.proto
  set_mode.proto
  set_position_target_global_int.proto
  set_position_target_local_ned.proto
  setup_signing.proto
  sim_state.proto
  smart_battery_info.proto
  statustext.proto
  storage_information.proto
  supported_tunes.proto
  sys_status.proto
  system_time.proto
  terrain_check.proto
  terrain_data.proto
  terrain_report.proto
  terrain_request.proto
  time_estimate_to_target.proto
  timesync.proto
  trajectory_representation_bezier.proto
  trajectory_representation_waypoints.proto
  tunnel.proto
  uavcan_node_info.proto
  uavcan_node_status.proto
  utm_global_position.proto
  v2_extension.proto
  vfr_hud.proto
  vibration.proto
  vicon_position_estimate.proto
  video_stream_information.proto
  video_stream_status.proto
  vision_position_estimate.proto
  vision_speed_estimate.proto
  wheel_distance.proto
  wifi_config_ap.proto
  winch_status.proto
  wind_cov.proto
)

# Generate protobug code.
protobuf_generate_cpp(TARGET_SOURCES TARGET_HEADERS ${TARGET_MESSAGES})

add_library(${TARGET_NAME} SHARED ${TARGET_HEADERS} ${TARGET_SOURCES})

# Set shared object properties.
set_target_properties(${TARGET_NAME} PROPERTIES VERSION ${GIT_VERSION})
set_target_properties(${TARGET_NAME} PROPERTIES SOVERSION ${GIT_VERSION_MAJOR})

target_include_directories(${TARGET_NAME} PRIVATE ${Protobuf_INCLUDE_DIRS})
target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(${TARGET_NAME} PRIVATE ${Protobuf_LIBRARIES})

# Add public header install target.
install(FILES ${TARGET_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LOCAL_PROJECT_PATH_FULL}/${TARGET_RELATIVE_PATH})
