cmake_minimum_required(VERSION 3.25.1)

find_package(gz-cmake3 REQUIRED)

gz_find_package(gz-common5 REQUIRED)
gz_find_package(gz-sim8 REQUIRED)

# Set the target name.
set(TARGET_NAME gz-lbot-bridge)

set(TARGET_HEADERS
  bridge.hpp
)

set(TARGET_SOURCES
  bridge.cpp
)

# Create library target.
add_library(${TARGET_NAME} SHARED ${TARGET_HEADERS} ${TARGET_SOURCES})

# Set shared object properties.
set_target_properties(${TARGET_NAME} PROPERTIES VERSION 0)
set_target_properties(${TARGET_NAME} PROPERTIES SOVERSION 0)

# Link external libraries to the library target.
target_link_libraries(${TARGET_NAME} PRIVATE gz-common5::gz-common5 gz-sim8::gz-sim8 pthread)

# Add a install targets.
install(TARGETS ${TARGET_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
