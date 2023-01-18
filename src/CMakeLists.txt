cmake_minimum_required(VERSION 3.15.0)


# Set the target name from the project name.
set(TARGET_NAME ${LOCAL_PROJECT_NAME})

# Get the install path.
file(RELATIVE_PATH TARGET_RELATIVE_PATH ${PROJECT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})

set(TARGET_SOURCES
  manager.cpp
  topic.cpp
  service.cpp
  logger.cpp
  exception.cpp
)

set(TARGET_HEADERS
  manager.hpp
  message.hpp
  node.hpp
  topic.hpp
  service.hpp
  logger.hpp
  exception.hpp
  plugin.hpp
)

# Create library target for the project.
add_library(${TARGET_NAME} SHARED ${TARGET_HEADERS} ${TARGET_SOURCES})

# Set shared object properties.
set_target_properties(${TARGET_NAME} PROPERTIES VERSION ${GIT_VERSION})
set_target_properties(${TARGET_NAME} PROPERTIES SOVERSION ${GIT_VERSION_MAJOR})


# Add subtarget source directory.
add_subdirectory(msg)
add_subdirectory(plugins)
add_subdirectory(utils)


# Link external libraries to the library target.
target_link_libraries(${TARGET_NAME} PRIVATE pthread)
# Link internal libraries to the library target.
target_link_libraries(${TARGET_NAME} PUBLIC robot_msg)


# Add public header install target.
install(FILES ${TARGET_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LOCAL_PROJECT_PATH_FULL}/${TARGET_RELATIVE_PATH})
