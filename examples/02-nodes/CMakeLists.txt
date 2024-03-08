cmake_minimum_required(VERSION 3.28.1)


find_package(labrat-robot MODULE REQUIRED)

# Set the target name.
set(TARGET_NAME nodes)

set(TARGET_SOURCES
  main.cpp
)

# Create executable target.
add_executable(${TARGET_NAME} ${TARGET_HEADERS} ${TARGET_SOURCES})

# Link external libraries target.
target_link_libraries(${TARGET_NAME} PRIVATE labrat-robot::core pthread)

# Create launch script
labrat_robot_add_launcher(TARGET ${TARGET_NAME})
