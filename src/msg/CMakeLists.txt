cmake_minimum_required(VERSION 3.20.0)


find_package(Protobuf MODULE REQUIRED)

# Set the target name from the path.
get_filename_component(TARGET_NAME_SECONDARY ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TARGET_NAME ${LOCAL_PROJECT_NAME}_${TARGET_NAME_SECONDARY})

# Get the install path.
file(RELATIVE_PATH TARGET_RELATIVE_PATH ${PROJECT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})

set(TARGET_MESSAGES
  test.proto
  log.proto
)

# Generate protobug code.
protobuf_generate_cpp(TARGET_SOURCES TARGET_HEADERS ${TARGET_MESSAGES})

add_library(${TARGET_NAME} SHARED ${TARGET_HEADERS} ${TARGET_SOURCES})

# Set shared object properties.
set_target_properties(${TARGET_NAME} PROPERTIES VERSION ${GIT_VERSION})
set_target_properties(${TARGET_NAME} PROPERTIES SOVERSION ${GIT_VERSION_MAJOR})

target_include_directories(${TARGET_NAME} PRIVATE ${Protobuf_INCLUDE_DIRS})
target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(${TARGET_NAME} PUBLIC ${Protobuf_LIBRARIES})

# Add public header install target.
install(FILES ${TARGET_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LOCAL_PROJECT_PATH_FULL}/${TARGET_RELATIVE_PATH})
