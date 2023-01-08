cmake_minimum_required(VERSION 3.20.0)


# Set the target name from the path.
get_filename_component(TARGET_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
get_filename_component(TARGET_NAME_PRIMARY ${TARGET_DIR} NAME)
get_filename_component(TARGET_NAME_SECONDARY ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TARGET_NAME ${TARGET_NAME_PRIMARY}_${TARGET_NAME_SECONDARY})

# Get the install path.
file(RELATIVE_PATH TARGET_RELATIVE_PATH ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
set(TARGET_PATH ${LOCAL_PROJECT_PATH}/${TARGET_RELATIVE_PATH})

set(TARGET_HEADERS
  atomic.hpp
  types.hpp
  fifo.hpp
  final_ptr.hpp
  thread.hpp
  concepts.hpp
)

# Add public header install target.
install(FILES ${TARGET_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${TARGET_PATH})
