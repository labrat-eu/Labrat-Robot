cmake_minimum_required(VERSION 3.20.0)

# Set the target name from the path.
get_filename_component(TARGET_NAME_SECONDARY ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TARGET_NAME ${LOCAL_PROJECT_NAME}_${TARGET_NAME_SECONDARY})

add_custom_target(${TARGET_NAME})

add_subdirectory(mcap)
add_subdirectory(foxglove-ws)
add_subdirectory(mavlink)
