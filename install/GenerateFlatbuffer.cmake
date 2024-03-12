cmake_minimum_required(VERSION 3.22.0)

find_package(flatbuffers MODULE REQUIRED)

function(lbot_generate_flatbuffer)
  set(options)
  set(oneValueArgs TARGET TARGET_PATH INCLUDE_PREFIX)
  set(multiValueArgs SCHEMAS INCLUDE_DIRS FLAGS)
  cmake_parse_arguments(INT "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT DEFINED INT_TARGET)
    message(FATAL_ERROR "No target provided.")
  endif()

  # Test if including from FindFlatBuffers
  if(FLATBUFFERS_FLATC_EXECUTABLE)
    set(FLATC_TARGET "")
    set(FLATC ${FLATBUFFERS_FLATC_EXECUTABLE})
  else()
    set(FLATC_TARGET flatc)
    set(FLATC flatc)
  endif()

  set(working_dir "${CMAKE_CURRENT_SOURCE_DIR}")

  # Create a directory to place the generated code.
  set(generated_target_dir "${CMAKE_BINARY_DIR}/flatbuffer")
  set(generated_include_dir "${generated_target_dir}/include/${INT_TARGET_PATH}")
  set(generated_schema_dir "${generated_target_dir}/schema/${INT_TARGET_PATH}")

  # Generate the include files parameters.
  set(include_params -I ${generated_target_dir}/include)
  foreach (include_dir ${INT_INCLUDE})
    set(include_params -I ${include_dir} ${include_params})
  endforeach()

  foreach (include_dir ${lbot_INCLUDE_DIRS})
    set(include_params -I ${include_dir} ${include_params})
  endforeach()

  if (NOT ${INT_INCLUDE_PREFIX} STREQUAL "")
    list(APPEND INT_FLAGS "--include-prefix" ${INT_INCLUDE_PREFIX})
  endif()

  list(APPEND INT_FLAGS --gen-object-api)
  list(APPEND INT_FLAGS --gen-name-strings)
  list(APPEND INT_FLAGS --gen-compare)
  list(APPEND INT_FLAGS --gen-object-api)
  list(APPEND INT_FLAGS --object-suffix Native)
  list(APPEND INT_FLAGS --no-prefix)
  list(APPEND INT_FLAGS --scoped-enums)
  list(APPEND INT_FLAGS --keep-prefix)
  list(APPEND INT_FLAGS --cpp-std=c++17)

  set(suffix ".fb")
  set(extension "hpp")

  # Create rules to generate the code for each schema.
  foreach(schema ${INT_SCHEMAS})
    get_filename_component(filename ${schema} NAME_WLE)
    set(generated_include "${generated_include_dir}/${filename}${suffix}.${extension}")

    add_custom_command(
      OUTPUT ${generated_include}
      COMMAND ${FLATC} ${FLATC_ARGS}
      -o ${generated_include_dir}
      ${include_params}
      -c ${schema}
      --filename-suffix ${suffix}
      --filename-ext ${extension}
      ${INT_FLAGS}
      DEPENDS ${FLATC_TARGET} ${schema}
      WORKING_DIRECTORY "${working_dir}"
      COMMENT "Building ${schema} flatbuffers...")
    list(APPEND all_generated_header_files ${generated_include})

    # Geneate the binary flatbuffers schemas.
    set(binary_schema
        "${generated_schema_dir}/${filename}.bfbs")
    add_custom_command(
      OUTPUT ${binary_schema}
      COMMAND ${FLATC} -b --schema
      -o ${generated_schema_dir}
      ${include_params}
      ${schema}
      DEPENDS ${FLATC_TARGET} ${schema}
      WORKING_DIRECTORY "${working_dir}")
    list(APPEND all_generated_binary_files ${binary_schema})
  endforeach()

  # Set up interface library
  add_library(${INT_TARGET} INTERFACE)
  target_sources(
    ${INT_TARGET}
    INTERFACE
      ${all_generated_header_files}
      ${all_generated_binary_files}
      ${INT_SCHEMAS})
  add_dependencies(
    ${INT_TARGET}
    ${FLATC}
    ${INT_SCHEMAS})
  target_include_directories(
    ${INT_TARGET}
    INTERFACE ${generated_target_dir}/include)

  install(FILES ${INT_SCHEMAS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${INT_TARGET_PATH})
  install(FILES ${all_generated_header_files} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${INT_TARGET_PATH})
  install(FILES ${all_generated_binary_files} DESTINATION ${CMAKE_INSTALL_RUNSTATEDIR}/${INT_TARGET_PATH})
endfunction(lbot_generate_flatbuffer)
