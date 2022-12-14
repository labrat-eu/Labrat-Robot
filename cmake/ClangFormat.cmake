cmake_minimum_required(VERSION 3.20.0)

function(prj_add_clang_format_targets)
  set(options)
  set(oneValueArgs SOURCEPATH)
  set(multiValueArgs)
  cmake_parse_arguments(INT "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  set(ARGS)

  # Handle source path argument.
  if(NOT DEFINED INT_SOURCEPATH)
    message(FATAL_ERROR "No source path given.")
  else()
    list(APPEND ARGS "-p ${INT_SOURCEPATH}")
  endif()

  # Use verbose mode.
  list(APPEND ARGS "-v")

  # Handle llvm version variable.
  if(DEFINED LLVM_VERSION)
    list(APPEND ARGS "-c ${LLVM_VERSION}")
  endif()

  # Configure .clang-format file and format checking scripts.
  configure_file(${PROJECT_SOURCE_DIR}/.scripts/pre/check-format.sh.in ${PROJECT_SOURCE_DIR}/.scripts/check-format.sh @ONLY)
  configure_file(${PROJECT_SOURCE_DIR}/.scripts/pre/format-code.sh.in ${PROJECT_SOURCE_DIR}/.scripts/format-code.sh @ONLY)
  configure_file(${PROJECT_SOURCE_DIR}/.clang/.clang-format.in ${PROJECT_SOURCE_DIR}/.clang-format @ONLY)

  # Add format script targets.
  add_custom_target(format-code COMMAND ${PROJECT_SOURCE_DIR}/.scripts/format-code.sh ${ARGS})
  add_custom_target(check-format COMMAND ${PROJECT_SOURCE_DIR}/.scripts/check-format.sh ${ARGS})
endfunction(prj_add_clang_format_targets)
