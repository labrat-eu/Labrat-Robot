cmake_minimum_required(VERSION 3.22.0)


find_package(flatbuffers MODULE REQUIRED)
find_package(yaml-cpp REQUIRED)

if (NOT TARGET flatbuffers::flatbuffers)
  add_library(flatbuffers::flatbuffers ALIAS flatbuffers::flatbuffers_shared)
endif()

# Set the target name from the project name.
set(TARGET_NAME ${LOCAL_PROJECT_NAME}_core)

# Get the install path.
file(RELATIVE_PATH TARGET_RELATIVE_PATH ${PROJECT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})

set(TARGET_SOURCES
  clock.cpp
  manager.cpp
  topic.cpp
  service.cpp
  logger.cpp
  exception.cpp
  config.cpp
)

set(TARGET_HEADERS
  base.hpp
  clock.hpp
  manager.hpp
  message.hpp
  node.hpp
  topic.hpp
  service.hpp
  logger.hpp
  exception.hpp
  plugin.hpp
  config.hpp
  info.hpp
  filter.hpp
)

# Create library target for the project.
add_library(${TARGET_NAME} ${TARGET_HEADERS} ${TARGET_SOURCES})

# Add subtarget source directory.
add_subdirectory(msg)
add_subdirectory(utils)
add_subdirectory(plugins)

# Link external libraries to the library target.
target_link_libraries(${TARGET_NAME} PRIVATE pthread yaml-cpp::yaml-cpp)
# Link internal libraries to the library target.
target_link_libraries(${TARGET_NAME} PUBLIC lbot_msg lbot_msg_external flatbuffers::flatbuffers)

# Add a install targets.
install(TARGETS ${TARGET_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${TARGET_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LOCAL_PROJECT_PATH_FULL}/${TARGET_RELATIVE_PATH})
install(FILES ${TARGET_HEADERS_UTILS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LOCAL_PROJECT_PATH_FULL}/utils/${TARGET_RELATIVE_PATH})
