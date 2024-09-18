cmake_minimum_required(VERSION 3.22.0)

function(prj_configure_clang_format)
  set(CLANNG_FORMAT ${CMAKE_BINARY_DIR}/.clang-format)
  set(CLANNG_FORMAT_IN ${PROJECT_SOURCE_DIR}/.clang/.clang-format.in)

  if(NOT EXISTS ${CLANNG_FORMAT_IN})
    return()
  endif()

  # Configure .clang-format file and format checking scripts.
  configure_file(${CLANNG_FORMAT_IN} ${CLANNG_FORMAT} @ONLY)
endfunction(prj_configure_clang_format)
