# Set minimum CMake version
cmake_minimum_required(VERSION "3.21")

# Configure project and languages
project(wavebird LANGUAGES C)

# Add CTest for running tests
include(CTest)

# Define the target and add the source files
add_library(wavebird STATIC "src/bch3121.c" "src/packet.c")

# Specify the include paths
target_include_directories(wavebird PRIVATE src/autogen PUBLIC include)

# Add the test targets
add_subdirectory(test)
