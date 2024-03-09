cmake_minimum_required(VERSION 3.28.1)


find_package(labrat-robot MODULE REQUIRED)

# Set the target name.
set(TARGET_NAME conversions)

set(TARGET_SOURCES
  main.cpp
)

set(TARGET_MESSAGES
  msg/number.fbs
)

# Create executable target.
add_executable(${TARGET_NAME} ${TARGET_HEADERS} ${TARGET_SOURCES})

# Generate flatbuffer messages.
set(TARGET_NAME_MESSAGE ${TARGET_NAME}_msg)
labrat_robot_generate_flatbuffer(TARGET ${TARGET_NAME_MESSAGE} SCHEMAS ${TARGET_MESSAGES} TARGET_PATH examples/msg)

# Link external libraries target.
target_link_libraries(${TARGET_NAME} PRIVATE labrat-robot::core pthread)
target_link_libraries(${TARGET_NAME} PRIVATE ${TARGET_NAME_MESSAGE})

# Create launch script
labrat_robot_add_launcher(TARGET ${TARGET_NAME})
