cmake_minimum_required(VERSION 3.22.0)

if(${PRJ_OPT_ENABLE_PLUGINS})
  # Set the target name from the path.
  get_filename_component(TARGET_NAME_SECONDARY ${CMAKE_CURRENT_SOURCE_DIR} NAME)
  set(TARGET_NAME ${LOCAL_PROJECT_NAME}_${TARGET_NAME_SECONDARY})

  add_library(${TARGET_NAME})

  add_subdirectory(linux)
  add_subdirectory(udp-bridge)
  add_subdirectory(serial-bridge)
  add_subdirectory(mcap)
  add_subdirectory(foxglove-ws)
  add_subdirectory(mavlink)

  if(${PRJ_OPT_ENABLE_PLUGINS_EXPERIMENTAL})
    add_subdirectory(gazebo-time)
  endif()

  install(TARGETS ${TARGET_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()
