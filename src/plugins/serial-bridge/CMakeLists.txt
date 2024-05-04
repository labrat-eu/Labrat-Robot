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

add_library(${TARGET_NAME} OBJECT ${TARGET_HEADERS} ${TARGET_SOURCES})
target_sources(lbot_plugins PRIVATE $<TARGET_OBJECTS:${TARGET_NAME}>)

# Set library properties.
set_target_properties(${TARGET_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Link external libraries to the library target.
target_link_libraries(${TARGET_NAME} PRIVATE crc_cpp::crc_cpp)
# Link internal libraries to the library target.
target_link_libraries(${TARGET_NAME} PUBLIC ${LOCAL_PROJECT_NAME}_core)

# Add a install targets.
install(FILES ${TARGET_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LOCAL_PROJECT_PATH_FULL}/${TARGET_RELATIVE_PATH})
