cmake_minimum_required(VERSION 3.20.0)

function(prj_add_doxygen_targets)
  set(DOXYFILE ${PROJECT_SOURCE_DIR}/.doxygen/doxyfile)
  set(DOXYFILE_IN ${PROJECT_SOURCE_DIR}/.doxygen/pre/doxyfile.in)

  # Configure doxyfile.
  configure_file(${DOXYFILE_IN} ${DOXYFILE} @ONLY)

  # Add doc generation target.
  if(${PRJ_OPT_INSTALL_DOCS})
    # Install man pages.
    add_custom_target(generate-docs ALL COMMAND doxygen ${ARGS} ${DOXYFILE} OUTPUTS ${PROJECT_SOURCE_DIR}/docs/ COMMENT "Generating documentation")
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/docs/man/man3 DESTINATION ${CMAKE_INSTALL_MANDIR})
  else()
    add_custom_target(generate-docs COMMAND doxygen ${ARGS} ${DOXYFILE} OUTPUTS ${PROJECT_SOURCE_DIR}/docs/ COMMENT "Generating documentation")
  endif()
endfunction(prj_add_doxygen_targets)
