cmake_minimum_required(VERSION 3.22.0)

function(lbot_add_launcher)
  set(options)
  set(oneValueArgs TARGET)
  set(multiValueArgs)
  cmake_parse_arguments(INT "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT DEFINED INT_TARGET)
    message(FATAL_ERROR "No target provided.")
  endif()

  set(LBOT_LAUNCHER_TARGET $<TARGET_FILE:${INT_TARGET}>)

  configure_file(${CMAKE_CURRENT_FUNCTION_LIST_DIR}/launcher.sh.in ${CMAKE_BINARY_DIR}/launch_${INT_TARGET}.tmp)
  file(GENERATE OUTPUT ${CMAKE_BINARY_DIR}/launch_${INT_TARGET}.sh INPUT ${CMAKE_BINARY_DIR}/launch_${INT_TARGET}.tmp FILE_PERMISSIONS OWNER_READ OWNER_EXECUTE)
endfunction(lbot_add_launcher)
