cmake_minimum_required(VERSION 3.20.0)


# Set the target name from the path.
set(TARGET_NAME ${LOCAL_PROJECT_NAME_FULL}_test)

# Get the install path.
file(RELATIVE_PATH TARGET_RELATIVE_PATH ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
set(TARGET_PATH ${LOCAL_PROJECT_PATH}/${TARGET_RELATIVE_PATH})

set(TARGET_TEST_SOURCES
  helper.hpp
  setup.cpp
  performance.cpp
  stress.cpp
  mcap.cpp
)

# Build unit tests if they are enabled.
if(${PRJ_OPT_ENABLE_TESTS})
  # Create executable target for the unit test.
  add_executable(${TARGET_NAME} ${TARGET_TEST_SOURCES})
  # Link libraries to the test target.
  target_link_libraries(${TARGET_NAME} PRIVATE ${LOCAL_PROJECT_NAME_FULL} gtest gtest_main pthread)
  # Make tests available to ctest
  gtest_discover_tests(${TARGET_NAME} DISCOVERY_MODE PRE_TEST)

  target_link_libraries(${TARGET_NAME} PRIVATE msg mcap)
endif()
