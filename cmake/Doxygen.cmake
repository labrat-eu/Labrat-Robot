cmake_minimum_required(VERSION 3.22.0)

function(prj_configure_doxygen)
  set(DOXYFILE ${CMAKE_BINARY_DIR}/doxyfile)
  set(DOXYFILE_IN ${PROJECT_SOURCE_DIR}/docs/doxyfile.in)

  if(NOT EXISTS ${DOXYFILE_IN})
    return()
  endif()

  # Configure doxyfile.
  configure_file(${DOXYFILE_IN} ${DOXYFILE} @ONLY)

  # Copy headers.
  file(COPY ${PROJECT_SOURCE_DIR}/src DESTINATION ${CMAKE_BINARY_DIR}/docs/include/${LOCAL_PROJECT_NAMESPACE} FILES_MATCHING PATTERN *.hpp)
  file(REMOVE_RECURSE ${CMAKE_BINARY_DIR}/docs/include/${LOCAL_PROJECT_PATH_FULL})
  file(RENAME ${CMAKE_BINARY_DIR}/docs/include/${LOCAL_PROJECT_NAMESPACE}/src ${CMAKE_BINARY_DIR}/docs/include/${LOCAL_PROJECT_PATH_FULL})
endfunction(prj_add_doxygen_targets)
