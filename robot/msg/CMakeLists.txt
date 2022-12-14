cmake_minimum_required(VERSION 3.20.0)


# Set the target name from the path.
get_filename_component(TARGET_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

# Get the install path.
file(RELATIVE_PATH TARGET_RELATIVE_PATH ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
set(TARGET_PATH ${LOCAL_PROJECT_PATH}/${TARGET_RELATIVE_PATH})


set(TARGET_SOURCES_NETWORK
  network.cpp
)

set(TARGET_HEADERS_NETWORK
  network.hpp
)

# Create library target.
add_library(${TARGET_NAME}-network STATIC ${TARGET_HEADERS_NETWORK} ${TARGET_SOURCES_NETWORK})

# Add public header install target.
install(FILES ${TARGET_HEADERS_NETWORK} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${TARGET_PATH})
