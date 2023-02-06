cmake_minimum_required(VERSION 3.22.0)


find_package(flatbuffers MODULE REQUIRED)

# Set the target name from the project name.
set(TARGET_NAME ${LOCAL_PROJECT_NAME})

# Get the install path.
file(RELATIVE_PATH TARGET_RELATIVE_PATH ${PROJECT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})

set(TARGET_SOURCES
  manager.cpp
  message.cpp
  topic.cpp
  service.cpp
  logger.cpp
  exception.cpp
)

set(TARGET_HEADERS
  base.hpp
  manager.hpp
  message.hpp
  node.hpp
  topic.hpp
  service.hpp
  logger.hpp
  exception.hpp
  plugin.hpp
)

set(TARGET_MESSAGES
  msg/test.fbs
  msg/log.fbs
)

# Create library target for the project.
add_library(${TARGET_NAME} SHARED ${TARGET_HEADERS} ${TARGET_SOURCES})

# Set shared object properties.
set_target_properties(${TARGET_NAME} PROPERTIES VERSION ${GIT_VERSION})
set_target_properties(${TARGET_NAME} PROPERTIES SOVERSION ${GIT_VERSION_MAJOR})


# Add subtarget source directory.
add_subdirectory(plugins)
add_subdirectory(utils)


set(TARGET_NAME_MESSAGE ${TARGET_NAME}_msg)
set(TARGET_MESSAGE_INCLUDE_PREFIX ${LOCAL_PROJECT_PATH_FULL}/${TARGET_RELATIVE_PATH}/msg)
set(TARGET_MESSAGE_NAMESPACE_PATH ${LOCAL_PROJECT_PATH_FULL}/${TARGET_RELATIVE_PATH}/msg)
set(TARGET_MESSAGE_BINARY_SCHEMAS_DIR ${CMAKE_CURRENT_BINARY_DIR}/schemas/${TARGET_MESSAGE_NAMESPACE_PATH})
flatbuffers_generate_headers(TARGET ${TARGET_NAME_MESSAGE} SCHEMAS ${TARGET_MESSAGES} INCLUDE_PREFIX ${TARGET_MESSAGE_INCLUDE_PREFIX} BINARY_SCHEMAS_DIR ${TARGET_MESSAGE_BINARY_SCHEMAS_DIR} FLAGS --gen-object-api --gen-name-strings --gen-compare --object-suffix "Native")

# Link external libraries to the library target.
target_link_libraries(${TARGET_NAME} PRIVATE ${TARGET_NAME}_msg pthread)
# Link internal libraries to the library target.
target_link_libraries(${TARGET_NAME} PUBLIC flatbuffers::flatbuffers)

# Add a install targets.
install(TARGETS ${TARGET_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${TARGET_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LOCAL_PROJECT_PATH_FULL}/${TARGET_RELATIVE_PATH})
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME_MESSAGE}/${TARGET_MESSAGE_INCLUDE_PREFIX} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LOCAL_PROJECT_PATH_FULL}/${TARGET_RELATIVE_PATH})
install(DIRECTORY ${TARGET_MESSAGE_BINARY_SCHEMAS_DIR} DESTINATION ${CMAKE_INSTALL_RUNSTATEDIR}/${TARGET_MESSAGE_NAMESPACE_PATH}/..)
