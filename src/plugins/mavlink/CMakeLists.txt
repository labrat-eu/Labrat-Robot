cmake_minimum_required(VERSION 3.22.0)


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
  conversions.cpp
  node.cpp
  serial_connection.cpp
  udp_connection.cpp
)

add_library(${TARGET_NAME} OBJECT ${TARGET_HEADERS} ${TARGET_SOURCES})
target_link_libraries(lbot_plugins PUBLIC ${TARGET_NAME})

# Set library properties.
set_target_properties(${TARGET_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_subdirectory(msg)

# Link external libraries to the library target.
target_link_libraries(${TARGET_NAME} PRIVATE mavlink_msg)
# Link internal libraries to the library target.
target_link_libraries(${TARGET_NAME} PUBLIC ${LOCAL_PROJECT_NAME}_core)

target_compile_options(${TARGET_NAME} PRIVATE "-Wno-address-of-packed-member")

# Add a install targets.
install(FILES ${TARGET_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LOCAL_PROJECT_PATH_FULL}/${TARGET_RELATIVE_PATH})
