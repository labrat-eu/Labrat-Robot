cmake_minimum_required(VERSION 3.20.0)

function(prj_add_clang_tidy)
  set(options FAIL_ON_WARNING)
  set(oneValueArgs)
  set(multiValueArgs)
  cmake_parse_arguments(INT "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT DEFINED LLVM_VERSION)
    message(FATAL_ERROR "No clang-toolchain used.")
  endif()

  set(ARGS "")
  list(APPEND ARGS "clang-tidy-${LLVM_VERSION}")

  # Handle fail on warning option.
  if(INT_FAIL_ON_WARNING)
    list(APPEND ARGS "--warnings-as-errors=*")
  endif()

  # Enable clang-tidy.
  set(CMAKE_C_CLANG_TIDY ${ARGS} PARENT_SCOPE)
  set(CMAKE_CXX_CLANG_TIDY ${ARGS} PARENT_SCOPE)

  # Configure .clang-tidy file.
  configure_file(${PROJECT_SOURCE_DIR}/.clang/.clang-tidy.in ${PROJECT_SOURCE_DIR}/.clang-tidy @ONLY)
endfunction(prj_add_clang_tidy)
