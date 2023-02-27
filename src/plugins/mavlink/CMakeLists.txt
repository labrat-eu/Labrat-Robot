cmake_minimum_required(VERSION 3.22.0)


# Set the target name from the path.
get_filename_component(TARGET_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
get_filename_component(TARGET_NAME_PRIMARY ${TARGET_DIR} NAME)
get_filename_component(TARGET_NAME_SECONDARY ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TARGET_NAME ${TARGET_NAME_PRIMARY}_${TARGET_NAME_SECONDARY})

# Get the install path.
file(RELATIVE_PATH TARGET_RELATIVE_PATH ${PROJECT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})

set(TARGET_HEADERS
  node.hpp
  connection.hpp
  serial_connection.hpp
  udp_connection.hpp
)

set(TARGET_SOURCES
  node.cpp
  serial_connection.cpp
  udp_connection.cpp
)

set(TARGET_MESSAGES
  msg/actuator_control_target.fbs
  msg/actuator_output_status.fbs
  msg/adsb_vehicle.fbs
  msg/ais_vessel.fbs
  msg/altitude.fbs
  msg/attitude.fbs
  msg/attitude_quaternion_cov.fbs
  msg/attitude_quaternion.fbs
  msg/attitude_target.fbs
  msg/att_pos_mocap.fbs
  msg/auth_key.fbs
  msg/autopilot_state_for_gimbal_device.fbs
  msg/autopilot_version.fbs
  msg/battery_status.fbs
  msg/button_change.fbs
  msg/camera_capture_status.fbs
  msg/camera_fov_status.fbs
  msg/camera_image_captured.fbs
  msg/camera_information.fbs
  msg/camera_settings.fbs
  msg/camera_tracking_geo_status.fbs
  msg/camera_tracking_image_status.fbs
  msg/camera_trigger.fbs
  msg/canfd_frame.fbs
  msg/can_filter_modify.fbs
  msg/can_frame.fbs
  msg/cellular_config.fbs
  msg/cellular_status.fbs
  msg/change_operator_control_ack.fbs
  msg/change_operator_control.fbs
  msg/collision.fbs
  msg/command_ack.fbs
  msg/command_cancel.fbs
  msg/command_int.fbs
  msg/command_long.fbs
  msg/component_information.fbs
  msg/component_metadata.fbs
  msg/control_system_state.fbs
  msg/current_event_sequence.fbs
  msg/data_stream.fbs
  msg/data_transmission_handshake.fbs
  msg/debug.fbs
  msg/debug_float_array.fbs
  msg/debug_vect.fbs
  msg/distance_sensor.fbs
  msg/efi_status.fbs
  msg/encapsulated_data.fbs
  msg/esc_info.fbs
  msg/esc_status.fbs
  msg/estimator_status.fbs
  msg/event.fbs
  msg/extended_sys_state.fbs
  msg/fence_status.fbs
  msg/file_transfer_protocol.fbs
  msg/flight_information.fbs
  msg/follow_target.fbs
  msg/generator_status.fbs
  msg/gimbal_device_attitude_status.fbs
  msg/gimbal_device_information.fbs
  msg/gimbal_device_set_attitude.fbs
  msg/gimbal_manager_information.fbs
  msg/gimbal_manager_set_attitude.fbs
  msg/gimbal_manager_set_manual_control.fbs
  msg/gimbal_manager_set_pitchyaw.fbs
  msg/gimbal_manager_status.fbs
  msg/global_position_int_cov.fbs
  msg/global_position_int.fbs
  msg/global_vision_position_estimate.fbs
  msg/gps2_raw.fbs
  msg/gps2_rtk.fbs
  msg/gps_global_origin.fbs
  msg/gps_inject_data.fbs
  msg/gps_input.fbs
  msg/gps_raw_int.fbs
  msg/gps_rtcm_data.fbs
  msg/gps_rtk.fbs
  msg/gps_status.fbs
  msg/heartbeat.fbs
  msg/high_latency2.fbs
  msg/high_latency.fbs
  msg/highres_imu.fbs
  msg/hil_actuator_controls.fbs
  msg/hil_controls.fbs
  msg/hil_gps.fbs
  msg/hil_optical_flow.fbs
  msg/hil_rc_inputs_raw.fbs
  msg/hil_sensor.fbs
  msg/hil_state.fbs
  msg/hil_state_quaternion.fbs
  msg/home_position.fbs
  msg/hygrometer_sensor.fbs
  msg/isbd_link_status.fbs
  msg/landing_target.fbs
  msg/link_node_status.fbs
  msg/local_position_ned_cov.fbs
  msg/local_position_ned.fbs
  msg/local_position_ned_system_global_offset.fbs
  msg/log_data.fbs
  msg/log_entry.fbs
  msg/log_erase.fbs
  msg/logging_ack.fbs
  msg/logging_data_acked.fbs
  msg/logging_data.fbs
  msg/log_request_data.fbs
  msg/log_request_end.fbs
  msg/log_request_list.fbs
  msg/mag_cal_report.fbs
  msg/manual_control.fbs
  msg/manual_setpoint.fbs
  msg/memory_vect.fbs
  msg/message_interval.fbs
  msg/mission_ack.fbs
  msg/mission_clear_all.fbs
  msg/mission_count.fbs
  msg/mission_current.fbs
  msg/mission_item.fbs
  msg/mission_item_int.fbs
  msg/mission_item_reached.fbs
  msg/mission_request.fbs
  msg/mission_request_int.fbs
  msg/mission_request_list.fbs
  msg/mission_request_partial_list.fbs
  msg/mission_set_current.fbs
  msg/mission_write_partial_list.fbs
  msg/mount_orientation.fbs
  msg/named_value_float.fbs
  msg/named_value_int.fbs
  msg/nav_controller_output.fbs
  msg/obstacle_distance.fbs
  msg/odometry.fbs
  msg/onboard_computer_status.fbs
  msg/open_drone_id_arm_status.fbs
  msg/open_drone_id_authentication.fbs
  msg/open_drone_id_basic_id.fbs
  msg/open_drone_id_location.fbs
  msg/open_drone_id_message_pack.fbs
  msg/open_drone_id_operator_id.fbs
  msg/open_drone_id_self_id.fbs
  msg/open_drone_id_system.fbs
  msg/open_drone_id_system_update.fbs
  msg/optical_flow.fbs
  msg/optical_flow_rad.fbs
  msg/orbit_execution_status.fbs
  msg/param_ext_ack.fbs
  msg/param_ext_request_list.fbs
  msg/param_ext_request_read.fbs
  msg/param_ext_set.fbs
  msg/param_ext_value.fbs
  msg/param_map_rc.fbs
  msg/param_request_list.fbs
  msg/param_request_read.fbs
  msg/param_set.fbs
  msg/param_value.fbs
  msg/ping.fbs
  msg/play_tune.fbs
  msg/play_tune_v2.fbs
  msg/position_target_global_int.fbs
  msg/position_target_local_ned.fbs
  msg/power_status.fbs
  msg/protocol_version.fbs
  msg/radio_status.fbs
  msg/raw_imu.fbs
  msg/raw_pressure.fbs
  msg/raw_rpm.fbs
  msg/rc_channels.fbs
  msg/rc_channels_override.fbs
  msg/rc_channels_raw.fbs
  msg/rc_channels_scaled.fbs
  msg/request_data_stream.fbs
  msg/request_event.fbs
  msg/resource_request.fbs
  msg/response_event_error.fbs
  msg/safety_allowed_area.fbs
  msg/safety_set_allowed_area.fbs
  msg/scaled_imu2.fbs
  msg/scaled_imu3.fbs
  msg/scaled_imu.fbs
  msg/scaled_pressure2.fbs
  msg/scaled_pressure3.fbs
  msg/scaled_pressure.fbs
  msg/serial_control.fbs
  msg/servo_output_raw.fbs
  msg/set_actuator_control_target.fbs
  msg/set_attitude_target.fbs
  msg/set_gps_global_origin.fbs
  msg/set_home_position.fbs
  msg/set_mode.fbs
  msg/set_position_target_global_int.fbs
  msg/set_position_target_local_ned.fbs
  msg/setup_signing.fbs
  msg/sim_state.fbs
  msg/smart_battery_info.fbs
  msg/statustext.fbs
  msg/storage_information.fbs
  msg/supported_tunes.fbs
  msg/sys_status.fbs
  msg/system_time.fbs
  msg/terrain_check.fbs
  msg/terrain_data.fbs
  msg/terrain_report.fbs
  msg/terrain_request.fbs
  msg/time_estimate_to_target.fbs
  msg/timesync.fbs
  msg/trajectory_representation_bezier.fbs
  msg/trajectory_representation_waypoints.fbs
  msg/tunnel.fbs
  msg/uavcan_node_info.fbs
  msg/uavcan_node_status.fbs
  msg/utm_global_position.fbs
  msg/v2_extension.fbs
  msg/vfr_hud.fbs
  msg/vibration.fbs
  msg/vicon_position_estimate.fbs
  msg/video_stream_information.fbs
  msg/video_stream_status.fbs
  msg/vision_position_estimate.fbs
  msg/vision_speed_estimate.fbs
  msg/wheel_distance.fbs
  msg/wifi_config_ap.fbs
  msg/winch_status.fbs
  msg/wind_cov.fbs
)

add_library(${TARGET_NAME} SHARED ${TARGET_HEADERS} ${TARGET_SOURCES})
add_dependencies(robot_plugins ${TARGET_NAME})

# Set shared object properties.
set_target_properties(${TARGET_NAME} PROPERTIES VERSION ${GIT_VERSION})
set_target_properties(${TARGET_NAME} PROPERTIES SOVERSION ${GIT_VERSION_MAJOR})

set(TARGET_NAME_MESSAGE ${TARGET_NAME}_msg)
set(TARGET_MESSAGE_INCLUDE_PREFIX ${LOCAL_PROJECT_PATH_FULL}/${TARGET_RELATIVE_PATH}/msg)
set(TARGET_MESSAGE_NAMESPACE_PATH mavlink/msg/common)
set(TARGET_MESSAGE_BINARY_SCHEMAS_DIR ${CMAKE_CURRENT_BINARY_DIR}/schemas/${TARGET_MESSAGE_NAMESPACE_PATH})
flatbuffers_generate_headers(TARGET ${TARGET_NAME_MESSAGE} SCHEMAS ${TARGET_MESSAGES} INCLUDE_PREFIX ${TARGET_MESSAGE_INCLUDE_PREFIX} BINARY_SCHEMAS_DIR ${TARGET_MESSAGE_BINARY_SCHEMAS_DIR} FLAGS --gen-object-api --gen-name-strings --gen-compare --object-suffix "Native")

# Link external libraries to the library target.
target_link_libraries(${TARGET_NAME} PRIVATE ${TARGET_NAME}_msg)
# Link internal libraries to the library target.
target_link_libraries(${TARGET_NAME} PUBLIC ${LOCAL_PROJECT_NAME})

# Add a install targets.
install(TARGETS ${TARGET_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${TARGET_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LOCAL_PROJECT_PATH_FULL}/${TARGET_RELATIVE_PATH})
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME_MESSAGE}/${TARGET_MESSAGE_INCLUDE_PREFIX} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LOCAL_PROJECT_PATH_FULL}/${TARGET_RELATIVE_PATH})
install(DIRECTORY ${TARGET_MESSAGE_BINARY_SCHEMAS_DIR} DESTINATION ${CMAKE_INSTALL_RUNSTATEDIR}/${TARGET_MESSAGE_NAMESPACE_PATH}/..)
