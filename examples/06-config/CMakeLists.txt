cmake_minimum_required(VERSION 3.22.0)


find_package(lbot MODULE REQUIRED)

# Set the target name.
set(TARGET_NAME config)

set(TARGET_SOURCES
  main.cpp
)

# Create executable target.
add_executable(${TARGET_NAME} ${TARGET_HEADERS} ${TARGET_SOURCES})

# Link external libraries target.
target_link_libraries(${TARGET_NAME} PRIVATE lbot::core lbot::plugins pthread)
