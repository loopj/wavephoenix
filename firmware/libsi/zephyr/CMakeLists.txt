# Define the library
zephyr_library_named(si)

# Set the library path
set(LIBSI_DIR ${CMAKE_CURRENT_LIST_DIR}/..)

# Add the common source files
zephyr_library_sources(
  ${LIBSI_DIR}/src/crc8.c
  ${LIBSI_DIR}/src/device/commands.c
  ${LIBSI_DIR}/src/device/gc_controller.c
)

# Specify the include paths
zephyr_include_directories(${LIBSI_DIR}/include)

# Link against Zephyr
target_link_libraries(si PUBLIC zephyr_interface)

# EFR32-specific settings
if(DEFINED SI_RX_TIMER_IDX)
  target_compile_definitions(si PUBLIC SI_RX_TIMER_IDX=${SI_RX_TIMER_IDX})
endif()

if(DEFINED SI_TX_USART_IDX)
  target_compile_definitions(si PUBLIC SI_TX_USART_IDX=${SI_TX_USART_IDX})
endif()

zephyr_library_sources(${LIBSI_DIR}/src/platform/efr32/si_efr32_emlib.c)

zephyr_library_sources(
  ${ZEPHYR_HAL_SILABS_MODULE_DIR}/simplicity_sdk/platform/emlib/src/em_timer.c
  ${ZEPHYR_HAL_SILABS_MODULE_DIR}/simplicity_sdk/platform/emlib/src/em_ldma.c
  ${ZEPHYR_HAL_SILABS_MODULE_DIR}/simplicity_sdk/platform/emdrv/dmadrv/src/dmadrv.c
)
