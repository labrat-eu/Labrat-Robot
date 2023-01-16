cmake_minimum_required(VERSION 3.20.0)

function(prj_add_coverage_report_targets)
  set(options)
  set(oneValueArgs TARGET)
  set(multiValueArgs)
  cmake_parse_arguments(INT "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  set(ARGS)

  # Handle source path argument.
  if(NOT DEFINED INT_TARGET)
    message(FATAL_ERROR "No target given.")
  else()
    list(APPEND ARGS "-p ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${INT_TARGET}")
    list(APPEND ARGS "-o ${PROJECT_SOURCE_DIR}/report_${INT_TARGET}")

    get_target_property(TARGET_DEPENDENCIES ${INT_TARGET} LINK_LIBRARIES)
    foreach(DEPENDENCY IN LISTS TARGET_DEPENDENCIES)
      list(APPEND ARGS "-l $<TARGET_FILE:$<IF:$<TARGET_EXISTS:${DEPENDENCY}>,${DEPENDENCY},${INT_TARGET}>>")
    endforeach()
  endif()

  # Use verbose mode.
  list(APPEND ARGS "-v")

  # Handle llvm version variable.
  if(DEFINED LLVM_VERSION)
    list(APPEND ARGS "-c ${LLVM_VERSION}")
  endif()

  if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Configure script.
    configure_file(${PROJECT_SOURCE_DIR}/.scripts/pre/coverage-report.sh.in ${PROJECT_SOURCE_DIR}/.scripts/coverage-report.sh @ONLY)

    # Add format script targets.
    add_custom_target(coverage_${INT_TARGET} COMMAND ${PROJECT_SOURCE_DIR}/.scripts/coverage-report.sh ${ARGS} DEPENDS ${INT_TARGET} COMMENT "Generating coverage report")
  endif()
endfunction(prj_add_coverage_report_targets)
