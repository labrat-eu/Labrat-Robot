cmake_minimum_required(VERSION 3.25.1)


# Set the target name from the path.
get_filename_component(TARGET_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
get_filename_component(TARGET_NAME_PRIMARY ${TARGET_DIR} NAME)
get_filename_component(TARGET_NAME_SECONDARY ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TARGET_NAME ${TARGET_NAME_PRIMARY}_${TARGET_NAME_SECONDARY})

# Get the install path.
file(RELATIVE_PATH TARGET_RELATIVE_PATH ${PROJECT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})

set(TARGET_HEADERS
  node.hpp
  connection.hpp
  serial_connection.hpp
  udp_connection.hpp
)

set(TARGET_SOURCES
  node.cpp
  serial_connection.cpp
  udp_connection.cpp
)

add_subdirectory(msg)

add_library(${TARGET_NAME} SHARED ${TARGET_HEADERS} ${TARGET_SOURCES})
add_dependencies(robot_plugins ${TARGET_NAME})

# Set shared object properties.
set_target_properties(${TARGET_NAME} PROPERTIES VERSION ${GIT_VERSION})
set_target_properties(${TARGET_NAME} PROPERTIES SOVERSION ${GIT_VERSION_MAJOR})

# Link internal libraries to the library target.
target_link_libraries(${TARGET_NAME} PUBLIC ${LOCAL_PROJECT_NAME} mavlink_msg)

# Add public header install target.
install(FILES ${TARGET_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LOCAL_PROJECT_PATH_FULL}/${TARGET_RELATIVE_PATH})
