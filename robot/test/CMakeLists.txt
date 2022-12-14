cmake_minimum_required(VERSION 3.20.0)

find_package(Protobuf MODULE REQUIRED)


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
  network.cpp
)

# Build unit tests if they are enabled.
if(${PRJ_OPT_ENABLE_TESTS})
  # Create executable target for the unit test.
  add_executable(${TARGET_NAME} ${TARGET_TEST_SOURCES})
  # Link libraries to the test target.
  target_link_libraries(${TARGET_NAME} PRIVATE ${LOCAL_PROJECT_NAME_FULL} gtest gtest_main pthread)
  # Make tests available to ctest
  gtest_discover_tests(${TARGET_NAME} DISCOVERY_MODE PRE_TEST)

  target_include_directories(${TARGET_NAME} PRIVATE ${Protobuf_INCLUDE_DIRS})
  protobuf_generate_cpp(PROTO_SRC PROTO_HEADER protobuf/test.proto)
  add_library(${TARGET_NAME}_protobuf ${PROTO_HEADER} ${PROTO_SRC})
  target_include_directories(${TARGET_NAME}_protobuf PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(${TARGET_NAME}_protobuf PRIVATE ${Protobuf_LIBRARIES})

  target_link_libraries(${TARGET_NAME} PRIVATE ${TARGET_NAME}_protobuf)
endif()
