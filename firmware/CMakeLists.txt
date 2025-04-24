# Set minimum CMake version
cmake_minimum_required(VERSION "3.21")

# Configure project and languages
project(wavephoenix LANGUAGES C ASM)

# Export compile commands for use with clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Include build targets for the common libraries
add_subdirectory(libsi)
add_subdirectory(libwavebird)

# Include build targets for the applications
if(CMAKE_CROSSCOMPILING)
  # Include openocd flashing utils
  include(${CMAKE_SOURCE_DIR}/cmake/openocd.cmake)
  include(${CMAKE_SOURCE_DIR}/cmake/ota.cmake)

  # Include build targets for the applications
  add_subdirectory(receiver)
endif()
