cmake_minimum_required(VERSION 3.25.1)

# Include toolchain.
include(${CMAKE_TOOLCHAIN_FILE})

# Load cmake modules.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

project(gz-lbot-test CXX)

add_subdirectory(src)
