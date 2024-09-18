cmake_minimum_required(VERSION 3.22.0)


# Set used languages.
set(USED_LANGUAGES CXX C ASM)

# Load cmake modules.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/install)

# Include tools.
include(Config)
include(Options)

# Set build types.
set(CMAKE_CONFIGURATION_TYPES Debug Release RelWithDebInfo)

# Default to Debug build type if none is set.
if(NOT DEFINED CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
  set(CMAKE_BUILD_TYPE Debug)
endif()

# Set build type compile definitions
string(TOUPPER ${CMAKE_BUILD_TYPE} PRJ_BUILD_TYPE_CAPS)
add_compile_definitions(PRJ_BUILD_TYPE=${CMAKE_BUILD_TYPE} "PRJ_BUILD_TYPE_${PRJ_BUILD_TYPE_CAPS}")
unset(PRJ_BUILD_TYPE_CAPS)
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Create the root project.
project(${LOCAL_PROJECT_NAME} VERSION ${GIT_VERSION} DESCRIPTION ${LOCAL_PROJECT_DESCRIPTION} LANGUAGES ${USED_LANGUAGES})

include(ClangFormat)
include(Doxygen)
include(IncludeSymlinks)
include(GenerateFlatbuffer)

# Set name definitions.
add_compile_definitions(
  PRJ_LOCAL_PROJECT_NAME="${LOCAL_PROJECT_NAME}"
  PRJ_LOCAL_PROJECT_DESCRIPTION="${LOCAL_PROJECT_DESCRIPTION}"
)

# Set version definitions.
add_compile_definitions(
  GIT_VERSION_MAJOR=${GIT_VERSION_MAJOR}
  GIT_VERSION_MINOR=${GIT_VERSION_MINOR}
  GIT_VERSION_PATCH=${GIT_VERSION_PATCH}
  GIT_SEMVER=${GIT_SEMVER}
  GIT_VERSION=${GIT_VERSION}
  GIT_TAG=${GIT_TAG}
  GIT_HASH=${GIT_HASH}
  GIT_HASH_SHORT=${GIT_HASH_SHORT}
  GIT_REF=${GIT_REF}
  GIT_BRANCH=${GIT_BRANCH}
)

# Define variables and options here.
prj_register_option(ENABLE_PLUGINS "Enable building of plugins." ON)
prj_register_option(ENABLE_FORMAT "Enable code formatting targets." OFF)
prj_register_option(ENABLE_DOCS "Enable documnetation generation targets." OFF)

# Print enabled variables and options.
list(JOIN PRJ_CUSTOM_VARIABLES ", " PRJ_PRJ_CUSTOM_VARIABLES_STRING)
list(JOIN PRJ_ENABLED_OPTIONS ", " PRJ_ENABLED_OPTIONS_STRING)
message(STATUS "Custom variables: " ${PRJ_PRJ_CUSTOM_VARIABLES_STRING})
message(STATUS "Enabled options: " ${PRJ_ENABLED_OPTIONS_STRING})

# Save the command line compile commands in the build output.
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif()

# Set correct linking path.
include(GNUInstallDirs)
list(APPEND CMAKE_INSTALL_RPATH "$ORIGIN")


# Add include symlinks.
prj_add_include_symlink(TARGET ${CMAKE_CURRENT_LIST_DIR}/src NAME ${LOCAL_PROJECT_NAME} NAMESPACE ${LOCAL_PROJECT_NAMESPACE})
prj_add_include_symlink(TARGET ${CMAKE_CURRENT_LIST_DIR}/submodules/mavlink NAME mavlink)

# Add the project root and build directory to the include path.
include_directories(BEFORE ${CMAKE_BINARY_DIR}/include)

# Add main source directory.
add_subdirectory(src)

prj_configure_clang_format()
prj_configure_doxygen()

install(FILES install/GenerateFlatbuffer.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake RENAME LbotGenerateFlatbuffer.cmake)
