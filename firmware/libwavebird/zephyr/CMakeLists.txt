# Define the library
zephyr_library_named(wavebird)

# Set the library path
set(LIBWAVEBIRD_DIR ${CMAKE_CURRENT_LIST_DIR}/..)

# Add the source files
zephyr_library_sources(
  ${LIBWAVEBIRD_DIR}/src/bch3121.c
  ${LIBWAVEBIRD_DIR}/src/packet.c
)

# Specify the include paths
zephyr_include_directories(${LIBWAVEBIRD_DIR}/include)

# Link against Zephyr
target_link_libraries(wavebird PUBLIC zephyr_interface)

# EFR32-specific settings
zephyr_library_sources(
  ${LIBWAVEBIRD_DIR}/src/autogen/rail_config.c
  ${LIBWAVEBIRD_DIR}/src/platform/efr32/radio_efr32.c
)

zephyr_include_directories(${LIBWAVEBIRD_DIR}/src/autogen)
