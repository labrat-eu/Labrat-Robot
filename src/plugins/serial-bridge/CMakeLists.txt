cmake_minimum_required(VERSION 3.22.0)


find_package(crc_cpp REQUIRED)

# Set the target name from the path.
get_filename_component(TARGET_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
get_filename_component(TARGET_NAME_PRIMARY ${TARGET_DIR} NAME)
get_filename_component(TARGET_NAME_SECONDARY ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TARGET_NAME ${TARGET_NAME_PRIMARY}_${TARGET_NAME_SECONDARY})

# Get the install path.
file(RELATIVE_PATH TARGET_RELATIVE_PATH ${PROJECT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})

set(TARGET_HEADERS
  node.hpp
)

set(TARGET_SOURCES
  node.cpp
)

add_library(${TARGET_NAME} SHARED ${TARGET_HEADERS} ${TARGET_SOURCES})
add_dependencies(robot_plugins ${TARGET_NAME})

# Set shared object properties.
set_target_properties(${TARGET_NAME} PROPERTIES VERSION ${GIT_VERSION})
set_target_properties(${TARGET_NAME} PROPERTIES SOVERSION ${GIT_VERSION_MAJOR})

# Link external libraries to the library target.
target_link_libraries(${TARGET_NAME} PRIVATE ${LOCAL_PROJECT_NAME}_msg crc_cpp::crc_cpp)
# Link internal libraries to the library target.
target_link_libraries(${TARGET_NAME} PUBLIC ${LOCAL_PROJECT_NAME})

# Add a install targets.
install(TARGETS ${TARGET_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${TARGET_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LOCAL_PROJECT_PATH_FULL}/${TARGET_RELATIVE_PATH})
