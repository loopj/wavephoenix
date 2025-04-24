# CMake target for the WavePhoenix receiver application
set(TARGET receiver)

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

# Create the target and set the sources
add_executable(
  ${TARGET}
  "src/app_properties.c"
  "src/button.c"
  "src/channel_wheel.c"
  "src/led.c"
  "src/main.c"
  "src/serial.c"
  "src/settings.c"
)

# Set dependencies
target_link_libraries(
  ${TARGET}
  PUBLIC GeckoSDK::bootloader::interface GeckoSDK::emlib GeckoSDK::emdrv::gpioint si wavebird
)

# Set the linker script
target_link_options(${TARGET} PRIVATE "-T${LINKER_SCRIPT}")

# Include the board configuration
target_include_directories(${TARGET} PRIVATE "config/${BOARD}")

# Enable debug logging if DEBUG is set
if(DEBUG)
  target_compile_definitions(${TARGET} PUBLIC DEBUG)
endif()

# Generate firmware files after building the target
gecko_sdk_generate_hex(${TARGET})
gecko_sdk_generate_gbl(${TARGET})

# Create OpenOCD flash target
add_openocd_flash_target(${TARGET})
add_ota_flash_target(${TARGET})
