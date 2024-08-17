cmake_minimum_required(VERSION 3.22.0)


find_package(lbot MODULE REQUIRED)

# Set the target name.
set(TARGET_NAME plugins)

set(TARGET_SOURCES
  main.cpp
)

set(TARGET_MESSAGES
  msg/vector.fbs
)

# Create executable target.
add_executable(${TARGET_NAME} ${TARGET_HEADERS} ${TARGET_SOURCES})

# Generate flatbuffer messages.
set(TARGET_NAME_MESSAGE ${TARGET_NAME}_msg)
lbot_generate_flatbuffer(TARGET ${TARGET_NAME_MESSAGE} SCHEMAS ${TARGET_MESSAGES} TARGET_PATH examples/msg)

# Link external libraries target.
target_link_libraries(${TARGET_NAME} PRIVATE lbot::core lbot::plugins pthread)
target_link_libraries(${TARGET_NAME} PRIVATE ${TARGET_NAME_MESSAGE})
