cmake_minimum_required(VERSION 3.20.0)


# Set the target name from the project name.
set(TARGET_NAME ${LOCAL_PROJECT_NAME_FULL})

# Get the install path.
file(RELATIVE_PATH TARGET_RELATIVE_PATH ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
set(TARGET_PATH ${LOCAL_PROJECT_PATH}/${TARGET_RELATIVE_PATH})

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
add_subdirectory(test)
add_subdirectory(examples)


# Link external libraries to the library target.
target_link_libraries(${TARGET_NAME} PRIVATE pthread)
# Link internal libraries to the library target.
target_link_libraries(${TARGET_NAME} PRIVATE robot_msg)


# Add public header install target.
install(FILES ${TARGET_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${TARGET_PATH})
