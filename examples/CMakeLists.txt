cmake_minimum_required(VERSION 3.15.0)

# Set C++20 standard.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set C11 standard.
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

project(examples CXX)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(SKIP_STD SKIP_FPIC KEEP_RPATHS)

find_package(labrat_robot REQUIRED)
find_package(Protobuf MODULE REQUIRED)

include_directories(BEFORE ${CMAKE_CURRENT_LIST_DIR}/src)

add_subdirectory(src)

install(TARGETS mavlink-mcap RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
