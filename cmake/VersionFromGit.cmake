cmake_minimum_required(VERSION 3.20.0)

function(prj_version_from_git)

  # Find Git executable.
  find_package(Git REQUIRED)

  # Git describe current commit. If the current commit is at a tag git will
  # return just the version without the hash.
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" describe --tags
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    RESULT_VARIABLE INT_GIT_RESULT
    OUTPUT_VARIABLE INT_GIT_DESCRIBE
    ERROR_VARIABLE INT_GIT_ERROR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_STRIP_TRAILING_WHITESPACE
  )
  if(NOT INT_GIT_RESULT EQUAL 0)
    set(INT_VERSION_MAJOR 0)
    set(INT_VERSION_MINOR 0)
    set(INT_VERSION_PATCH 0)
    set(INT_GIT_AT_TAG FALSE)
  else()

    # Get the latest git tag.
    execute_process(
      COMMAND "${GIT_EXECUTABLE}" describe --tags --abbrev=0
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      RESULT_VARIABLE INT_GIT_RESULT
      OUTPUT_VARIABLE INT_GIT_TAG
      ERROR_VARIABLE INT_GIT_ERROR
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_STRIP_TRAILING_WHITESPACE
    )
    if(NOT INT_GIT_RESULT EQUAL 0)
      message(FATAL_ERROR "Failed to execute Git: ${INT_GIT_ERROR}")
    endif()

    # Extract the version numbers from the semantic version string.
    if(INT_GIT_TAG MATCHES "^v(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)$")
      set(INT_VERSION_MAJOR "${CMAKE_MATCH_1}")
      set(INT_VERSION_MINOR "${CMAKE_MATCH_2}")
      set(INT_VERSION_PATCH "${CMAKE_MATCH_3}")
    else()
      message(FATAL_ERROR "Git tag isn't valid semantic version: [${INT_GIT_TAG}]")
    endif()

    # Check if the current commit is tagged.
    if("${INT_GIT_TAG}" STREQUAL "${INT_GIT_DESCRIBE}")
      set(INT_GIT_AT_TAG ON)
    endif()
  endif()

  # Get the hash for the current commit.
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    RESULT_VARIABLE INT_GIT_RESULT
    OUTPUT_VARIABLE INT_GIT_HASH
    ERROR_VARIABLE INT_GIT_ERROR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_STRIP_TRAILING_WHITESPACE
  )
  if(NOT INT_GIT_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to execute Git: ${INT_GIT_ERROR}")
  endif()
  # Create a short hash string.
  string(SUBSTRING ${INT_GIT_HASH} 0 7 INT_GIT_HASH_SHORT)

  # Get the current branch.
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    RESULT_VARIABLE INT_GIT_RESULT
    OUTPUT_VARIABLE INT_GIT_BRANCH
    ERROR_VARIABLE INT_GIT_ERROR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_STRIP_TRAILING_WHITESPACE
  )
  if(NOT INT_GIT_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to execute Git: ${INT_GIT_ERROR}")
  endif()

  # Construct the version variables.
  set(INT_VERSION ${INT_VERSION_MAJOR}.${INT_VERSION_MINOR}.${INT_VERSION_PATCH})
  set(INT_SEMVER ${INT_VERSION})

  if(NOT INT_GIT_AT_TAG)
    set(INT_SEMVER ${INT_SEMVER}+${INT_GIT_HASH_SHORT})
  endif()

  # Set parent scope variables.
  set(GIT_VERSION_MAJOR ${INT_VERSION_MAJOR} PARENT_SCOPE)
  set(GIT_VERSION_MINOR ${INT_VERSION_MINOR} PARENT_SCOPE)
  set(GIT_VERSION_PATCH ${INT_VERSION_PATCH} PARENT_SCOPE)
  set(GIT_SEMVER ${INT_SEMVER} PARENT_SCOPE)
  set(GIT_VERSION ${INT_VERSION} PARENT_SCOPE)
  set(GIT_TAG ${INT_GIT_TAG} PARENT_SCOPE)
  set(GIT_HASH ${INT_GIT_HASH} PARENT_SCOPE)
  set(GIT_HASH_SHORT ${INT_GIT_HASH_SHORT} PARENT_SCOPE)
  set(GIT_BRANCH ${INT_GIT_BRANCH} PARENT_SCOPE)

endfunction(prj_version_from_git)
