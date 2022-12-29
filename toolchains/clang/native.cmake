cmake_minimum_required(VERSION 3.20.0)

# Set LLVM version.
set(LLVM_VERSION 14)

# Set compiler target architecture.
set(TRIPLE native)

# Set the binary output paths.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${TRIPLE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lib)

# Set compiler to use.
set(CMAKE_C_COMPILER clang-${LLVM_VERSION})
set(CMAKE_CXX_COMPILER clang++-${LLVM_VERSION})
set(CMAKE_ASM_COMPILER clang-${LLVM_VERSION})
set(CMAKE_LINKER lld-link-${LLVM_VERSION})
set(CMAKE_OBJCOPY llvm-objcopy-${LLVM_VERSION})
set(CMAKE_SIZE_UTIL llvm-size-${LLVM_VERSION})

# Default C compiler flags.

# Enable most warnings, treat warnings as errors and set highest optimization.
set(CMAKE_C_FLAGS_RELEASE "-Wall -Wextra -Werror -O3" CACHE STRING "" FORCE)
# Enable most warnings, enable debug symbols and disable optimization.
set(CMAKE_C_FLAGS_DEBUG "-Wall -Wextra -g -O0 -fstandalone-debug" CACHE STRING "" FORCE)

# Default C++ compiler flags.

# Enable most warnings, treat warnings as errors and set highest optimization.
set(CMAKE_CXX_FLAGS_RELEASE "-Wall -Wextra -Werror -O3" CACHE STRING "" FORCE)
# Enable most warnings, enable debug symbols and disable optimization.
set(CMAKE_CXX_FLAGS_DEBUG "-Wall -Wextra -g -O0 -fstandalone-debug" CACHE STRING "" FORCE)

# Default ASM compiler flags.

# Enable most warnings, treat warnings as errors and set highest optimization.
set(CMAKE_ASM_FLAGS_RELEASE "-Wall -Wextra -Werror -O3" CACHE STRING "" FORCE)
# Enable most warnings, enable debug symbols and disable optimization.
set(CMAKE_ASM_FLAGS_DEBUG "-Wall -Wextra -g -O0 -fstandalone-debug" CACHE STRING "" FORCE)

# Add library locations.
link_directories("/usr/local/lib")
