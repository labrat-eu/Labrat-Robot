cmake_minimum_required(VERSION 3.22.0)

# Set the target name from the path.
get_filename_component(TARGET_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
get_filename_component(TARGET_NAME_PRIMARY ${TARGET_DIR} NAME)
get_filename_component(TARGET_NAME_SECONDARY ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TARGET_NAME ${TARGET_NAME_PRIMARY}_${TARGET_NAME_SECONDARY})

# Get the install path.
file(RELATIVE_PATH TARGET_RELATIVE_PATH ${PROJECT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})

set(TARGET_MESSAGES
  disks.fbs
  cpu.fbs
  memory.fbs
  process.fbs
)

lbot_generate_flatbuffer(TARGET ${TARGET_NAME} SCHEMAS ${TARGET_MESSAGES} TARGET_PATH ${LOCAL_PROJECT_PATH_FULL}/${TARGET_RELATIVE_PATH})
