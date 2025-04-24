# Set minimum CMake version
cmake_minimum_required(VERSION "3.21")

# Configure project and languages
project(si LANGUAGES C)

# Define the library
add_library(si STATIC "src/crc8.c" "src/commands.c" "src/device/gc_controller.c")

# Specify the include paths
target_include_directories(si PUBLIC include)

# EFR32 platform specific settings
if(CMAKE_CROSSCOMPILING)
  # Download and make the GeckoSDK CMake targets available
  if(NOT GeckoSDK_FOUND)
    include(FetchContent)
    FetchContent_Declare(
      GeckoSDK
      GIT_REPOSITORY https://github.com/loopj/gecko-sdk-cmake.git
      GIT_TAG main
    )
    FetchContent_MakeAvailable(GeckoSDK)
  endif()

  # Add the platform-specific source files
  target_sources(si PRIVATE "src/platform/efr32/si_efr32.c")

  # Depend on emlib from the Gecko SDK
  target_link_libraries(si GeckoSDK::emlib GeckoSDK::emdrv::dmadrv)
endif()

# Add the test target
if(NOT CMAKE_CROSSCOMPILING)
  add_subdirectory(test)
endif()
