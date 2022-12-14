cmake_minimum_required(VERSION 3.20.0)

function(prj_add_include_symlink)
  set(options)
  set(oneValueArgs TARGET NAME NAMESPACE)
  set(multiValueArgs)
  cmake_parse_arguments(INT "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  # Create a symbolic link from the include directory path to the passed target.
  file(MAKE_DIRECTORY ${PROJECT_SOURCE_DIR}/include/${INT_NAMESPACE})
  file(CREATE_LINK ${PROJECT_SOURCE_DIR}/${INT_TARGET} ${PROJECT_SOURCE_DIR}/include/${INT_NAMESPACE}/${INT_NAME} SYMBOLIC)
endfunction(prj_add_include_symlink)
