# Set minimum CMake version
cmake_minimum_required(VERSION "3.21")

# Configure project and languages
project(si LANGUAGES C)

# Define the library
add_library(si STATIC src/crc8.c src/device/commands.c src/device/gc_controller.c)

# Specify the include paths
target_include_directories(si PUBLIC include)

# EFR32 platform specific settings
if(CMAKE_CROSSCOMPILING)
  # Building for EFR32 using gecko-sdk-cmake
  if(NOT GeckoSDK_FOUND)
    include(FetchContent)
    FetchContent_Declare(
      GeckoSDK
      GIT_REPOSITORY https://github.com/loopj/gecko-sdk-cmake.git
      GIT_TAG main
    )
    FetchContent_MakeAvailable(GeckoSDK)
  endif()

  # Configure SI peripheral selection
  if(DEFINED SI_RX_TIMER_IDX)
    target_compile_definitions(si PUBLIC SI_RX_TIMER_IDX=${SI_RX_TIMER_IDX})
  endif()

  if(DEFINED SI_TX_USART_IDX)
    target_compile_definitions(si PUBLIC SI_TX_USART_IDX=${SI_TX_USART_IDX})
  endif()

  # Add the platform-specific source files
  target_sources(si PRIVATE "src/platform/efr32/si_efr32_emlib.c")

  # Depend on emlib from the Gecko SDK
  target_link_libraries(si GeckoSDK::emlib GeckoSDK::emdrv::dmadrv)
endif()

# Add the test target
if(NOT CMAKE_CROSSCOMPILING)
  add_subdirectory(test)
endif()
