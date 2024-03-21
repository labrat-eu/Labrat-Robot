cmake_minimum_required(VERSION 3.22.0)

# Include toolchain.
include(${CMAKE_TOOLCHAIN_FILE})

project(test CXX)

# Save the command line compile commands in the build output.
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

find_package(lbot MODULE REQUIRED)
find_package(GTest REQUIRED)
include(GoogleTest)

# Set discovery mode.
set(CMAKE_GTEST_DISCOVER_TESTS_DISCOVERY_MODE POST_BUILD)
# Enable testing if tests are enabled.
enable_testing()

include_directories(BEFORE ${CMAKE_CURRENT_LIST_DIR}/src)

set(TARGET_TEST_SOURCES
  src/helper.hpp
  src/main.cpp
  src/thread.cpp
  src/semantics.cpp
  src/setup.cpp
  src/manager.cpp
  src/performance.cpp
  src/stress.cpp
  src/mcap.cpp
  src/udp-bridge.cpp
  src/serial-bridge.cpp
)

# Create executable target for the unit test.
add_executable(testing ${TARGET_TEST_SOURCES})
# Link libraries to the test target.
target_link_libraries(testing PRIVATE lbot::core lbot::plugins GTest::gtest pthread)
# Make tests available to ctest
gtest_discover_tests(testing DISCOVERY_MODE PRE_TEST)
