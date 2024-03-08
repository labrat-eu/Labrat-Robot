cmake_minimum_required(VERSION 3.28.1)

# Include toolchain.
include(${CMAKE_TOOLCHAIN_FILE})

project(labrat-robot-example CXX)

# Add examples
add_subdirectory(00-hello-world)
add_subdirectory(01-logging)
add_subdirectory(02-nodes)
add_subdirectory(03-threads)
add_subdirectory(04-topics)
add_subdirectory(05-conversions)
add_subdirectory(06-services)
add_subdirectory(07-plugins)
