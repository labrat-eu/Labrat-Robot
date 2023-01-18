cmake_minimum_required(VERSION 3.25.1)

# Set C++20 standard.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set C11 standard.
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

project(test CXX)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(SKIP_STD SKIP_FPIC)

find_package(labrat_robot REQUIRED)
find_package(Protobuf MODULE REQUIRED)
find_package(GTest REQUIRED)

# Set discovery mode.
set(CMAKE_GTEST_DISCOVER_TESTS_DISCOVERY_MODE POST_BUILD)
# Enable testing if tests are enabled.
enable_testing()

include_directories(BEFORE ${CMAKE_CURRENT_LIST_DIR}/src)

set(TARGET_TEST_SOURCES
  src/helper.hpp
  src/setup.cpp
  src/performance.cpp
  src/stress.cpp
  src/mcap.cpp
)

# Create executable target for the unit test.
add_executable(testing ${TARGET_TEST_SOURCES})
# Link libraries to the test target.
target_link_libraries(testing PRIVATE GTest::gtest GTest::gtest_main pthread)
# Make tests available to ctest
gtest_discover_tests(testing DISCOVERY_MODE PRE_TEST)

target_include_directories(testing PRIVATE ${Protobuf_INCLUDE_DIRS})

target_link_libraries(testing PUBLIC labrat_robot::robot labrat_robot::plugins_foxglove-ws labrat_robot::plugins_mavlink labrat_robot::plugins_mcap ${Protobuf_LIBRARIES})
