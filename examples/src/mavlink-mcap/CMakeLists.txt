cmake_minimum_required(VERSION 3.20.0)


# Set the target name from the path.
get_filename_component(TARGET_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

# Get the install path.
file(RELATIVE_PATH TARGET_RELATIVE_PATH ${PROJECT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})

set(TARGET_SOURCES
  main.cpp
)

# Create executable target for the unit test.
add_executable(${TARGET_NAME} ${TARGET_SOURCES})

target_link_libraries(${TARGET_NAME} PRIVATE labrat_robot::plugins_foxglove-ws labrat_robot::plugins_mavlink labrat_robot::plugins_mcap)
