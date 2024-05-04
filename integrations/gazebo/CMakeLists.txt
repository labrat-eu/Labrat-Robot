cmake_minimum_required(VERSION 3.25.1)

# Include toolchain.
include(${CMAKE_TOOLCHAIN_FILE})

# Load cmake modules.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

# Include tools.
include(Config)
include(IncludeSymlinks)

project(${LOCAL_PROJECT_NAME_FULL} CXX)

# Add include symlinks.
prj_add_include_symlink(TARGET ${CMAKE_CURRENT_LIST_DIR}/src NAME ${LOCAL_PROJECT_NAME} NAMESPACE ${LOCAL_PROJECT_NAMESPACE})

# Add the project root and build directory to the include path.
include_directories(BEFORE ${CMAKE_CURRENT_LIST_DIR}/include)
include_directories(BEFORE ${CMAKE_BINARY_DIR})

add_subdirectory(src)
