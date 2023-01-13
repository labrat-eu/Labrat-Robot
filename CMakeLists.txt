cmake_minimum_required(VERSION 3.20.0)


# Set used languages.
set(USED_LANGUAGES CXX C ASM)

# Load cmake modules.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

# Include tools.
include(Config)
include(Options)
include(VersionFromGit)
include(ClangFormat)
include(ClangTidy)
include(Doxygen)
include(IncludeSymlinks)

# Include global packages.
find_package(PkgConfig)

# Set C++20 standard.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set C11 standard.
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)



# Select default toolchain file.
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE OR CMAKE_TOOLCHAIN_FILE STREQUAL "")
  set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/toolchains/clang/native.cmake)
  include(${CMAKE_TOOLCHAIN_FILE})
endif()

# Inform of the toolchain file used
message(STATUS "Toolchain file: ${CMAKE_TOOLCHAIN_FILE}")

# Set build types.
set(CMAKE_CONFIGURATION_TYPES Debug Release)

# Default to Debug build type if none is set.
if(NOT DEFINED CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
  set(CMAKE_BUILD_TYPE Debug)
endif()

# Set build type compile definitions
string(TOUPPER ${CMAKE_BUILD_TYPE} PRJ_BUILD_TYPE_CAPS)
add_compile_definitions(PRJ_BUILD_TYPE=${CMAKE_BUILD_TYPE} "PRJ_BUILD_TYPE_${PRJ_BUILD_TYPE_CAPS}")
unset(PRJ_BUILD_TYPE_CAPS)
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Set name definitions.
add_compile_definitions(
  PRJ_LOCAL_PROJECT_NAME="${LOCAL_PROJECT_NAME}"
  PRJ_LOCAL_PROJECT_DESCRIPTION="${LOCAL_PROJECT_DESCRIPTION}"
)


# Get a version string from git.
prj_version_from_git()

# Set version definitions.
add_compile_definitions(
  GIT_VERSION_MAJOR=${GIT_VERSION_MAJOR}
  GIT_VERSION_MINOR=${GIT_VERSION_MINOR}
  GIT_VERSION_PATCH=${GIT_VERSION_PATCH}
  GIT_SEMVER="${GIT_SEMVER}"
  GIT_VERSION="${GIT_VERSION}"
  GIT_TAG="${GIT_TAG}"
  GIT_HASH="${GIT_HASH}"
  GIT_HASH_SHORT="${GIT_HASH_SHORT}"
  GIT_BRANCH="${GIT_BRANCH}"
)


# Create the root project.
project(${LOCAL_PROJECT_NAME} VERSION ${GIT_VERSION} DESCRIPTION ${LOCAL_PROJECT_DESCRIPTION} LANGUAGES ${USED_LANGUAGES})


# Define variables and options here.
prj_register_var(DOCKER_IMAGE "Name of the docker image to be built." ${PROJECT_NAME})
prj_register_option(ENABLE_TESTS "Enable the building of unit tests." ON)
prj_register_option(ENABLE_COVERAGE "Enable generation of code coverage information once the application is run." OFF)
prj_register_option(ENABLE_PROFILING "Enable generation of profiling information once the application is run." OFF)
prj_register_option(ENABLE_LINTING "Enable the use of various linters (e.g. clang-tidy etc.)." OFF)
prj_register_option(INSTALL_DOCS "Enable installation of man pages on make install." OFF)

# Print enabled variables and options.
list(JOIN PRJ_CUSTOM_VARIABLES ", " PRJ_PRJ_CUSTOM_VARIABLES_STRING)
list(JOIN PRJ_ENABLED_OPTIONS ", " PRJ_ENABLED_OPTIONS_STRING)
message(STATUS "Custom variables: " ${PRJ_PRJ_CUSTOM_VARIABLES_STRING})
message(STATUS "Enabled options: " ${PRJ_ENABLED_OPTIONS_STRING})


# Handle enabled tests option.
if(${PRJ_OPT_ENABLE_TESTS})
  find_package(GTest REQUIRED)
  # Set discovery mode.
  set(CMAKE_GTEST_DISCOVER_TESTS_DISCOVERY_MODE POST_BUILD)
  # Enable testing if tests are enabled.
  enable_testing()
endif()

# Handle enabled coverage option.
if(${PRJ_OPT_ENABLE_COVERAGE})
  if(CMAKE_BUILD_TYPE MATCHES Debug)
    list(APPEND PRJ_C_FLAGS "-fprofile-arcs" "-ftest-coverage")
    list(APPEND PRJ_CXX_FLAGS "-fprofile-arcs" "-ftest-coverage")
    list(APPEND PRJ_ASM_FLAGS "-fprofile-arcs" "-ftest-coverage")
  else()
    message(WARNING "Invalid configuration: Option ENABLE_COVERAGE requires build type 'Debug'.")
  endif()
endif()

# Handle enabled profiling option.
if(${PRJ_OPT_ENABLE_PROFILING})
  list(APPEND PRJ_C_FLAGS "-pg")
  list(APPEND PRJ_CXX_FLAGS "-pg")
  list(APPEND PRJ_ASM_FLAGS "-pg")
endif()

# Handle enabled linting option.
if(${PRJ_OPT_ENABLE_LINTING})
  if(CMAKE_BUILD_TYPE MATCHES Debug)
    prj_add_clang_tidy()
  else()
    prj_add_clang_tidy(FAIL_ON_WARNING)
  endif()
endif()

# Apply all set compiler flags to the CMAKE variable.
list(JOIN PRJ_C_FLAGS " " PRJ_C_FLAGS)
list(JOIN PRJ_CXX_FLAGS " " PRJ_CXX_FLAGS)
list(JOIN PRJ_ASM_FLAGS " " PRJ_ASM_FLAGS)
if(CMAKE_BUILD_TYPE MATCHES Debug)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS_DEBUG} ${PRJ_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_DEBUG} ${PRJ_CXX_FLAGS}")
  set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS_DEBUG} ${PRJ_ASM_FLAGS}")
else(CMAKE_BUILD_TYPE MATCHES Release)
  set(CMAKE_C_FLAGS "${CMAKE_C_RELEASE} ${PRJ_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_RELEASE} ${PRJ_CXX_FLAGS}")
  set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS_RELEASE} ${PRJ_ASM_FLAGS}")
endif()

# Save the command line compile commands in the build output.
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)


# Set correct linking path.
include(GNUInstallDirs)
set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib)


# TODO: add description
prj_add_include_symlink(TARGET src NAME ${LOCAL_PROJECT_NAME} NAMESPACE ${LOCAL_PROJECT_NAMESPACE})

# Add the project root and build directory to the include path.
include_directories(BEFORE ${CMAKE_CURRENT_LIST_DIR}/include)
include_directories(BEFORE ${CMAKE_BINARY_DIR})

# Add header libraries to the include path.
include_directories(BEFORE ${CMAKE_CURRENT_LIST_DIR}/submodules)
include_directories(BEFORE ${CMAKE_CURRENT_LIST_DIR}/submodules/cpp-utils)
include_directories(BEFORE ${CMAKE_CURRENT_LIST_DIR}/submodules/mcap/cpp/mcap/include)
include_directories(BEFORE ${CMAKE_CURRENT_LIST_DIR}/submodules/foxglove-ws/cpp/foxglove-websocket/include)

# Add main source directory.
add_subdirectory(src)

# Add clang format targets.
prj_add_clang_format_targets(SOURCEPATH ${CMAKE_CURRENT_LIST_DIR}/src)

# Add doxygen targets.
prj_add_doxygen_targets()

# Add a install target for additional configurations.
install(TARGETS ${LOCAL_PROJECT_NAME} robot_msg mavlink_msg plugins_foxglove-ws plugins_mavlink plugins_mcap EXPORT ${LOCAL_PROJECT_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${LOCAL_PROJECT_NAME_FULL} ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/${LOCAL_PROJECT_NAME_FULL} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT ${LOCAL_PROJECT_NAME} FILE ${LOCAL_PROJECT_NAME_FULL}.cmake NAMESPACE ${LOCAL_PROJECT_NAME_FULL}:: DESTINATION ${CMAKE_INSTALL_DATADIR}/${LOCAL_PROJECT_NAME_FULL})

include(CMakePackageConfigHelpers)
configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/install/Config.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/${LOCAL_PROJECT_NAME_FULL}Config.cmake INSTALL_DESTINATION ${CMAKE_INSTALL_DATADIR}/${LOCAL_PROJECT_NAME_FULL} PATH_VARS CMAKE_INSTALL_DATADIR)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${LOCAL_PROJECT_NAME_FULL}Config.cmake DESTINATION ${CMAKE_INSTALL_DATADIR}/${LOCAL_PROJECT_NAME_FULL})
