cmake_minimum_required(VERSION 3.25.1)

project(examples CXX)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(SKIP_FPIC KEEP_RPATHS)

find_package(labrat_robot REQUIRED)
find_package(Protobuf MODULE REQUIRED)

include_directories(BEFORE ${CMAKE_CURRENT_LIST_DIR}/src)

add_subdirectory(src)

install(TARGETS mavlink-mcap RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
