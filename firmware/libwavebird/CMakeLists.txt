# Set minimum CMake version
cmake_minimum_required(VERSION "3.21")

# Configure project and languages
project(wavebird LANGUAGES C)

# Define the target and add the source files
add_library(wavebird STATIC "src/bch3121.c" "src/packet.c")

# Specify the include paths
target_include_directories(wavebird PRIVATE src/autogen PUBLIC include)

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

  # Configure automatic generation of rail_config.c and rail_config.h files
  set_rail_config_paths("config/rail/radio_settings_${GECKO_SDK_RAIL_LIB_NAME}.radioconf" "src/autogen")

  # Add platform-specific source files
  target_sources(wavebird PRIVATE "src/platform/efr32/radio_efr32.c" "src/autogen/rail_config.c")

  # Depend on emlib and rail_lib from the Gecko SDK
  target_link_libraries(wavebird GeckoSDK::emlib GeckoSDK::rail_lib)
endif()

# Add the test target
if(NOT CMAKE_CROSSCOMPILING)
  add_subdirectory(test)
endif()
