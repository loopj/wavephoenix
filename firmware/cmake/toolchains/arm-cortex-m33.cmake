# Toolchain file for ARM Cortex-M33 Gecko SoCs

# Configure for a generic ARM embedded build
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Point to the arm-none-eabi toolchain
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-g++)

# Determine the ARM sysroot
execute_process(
  COMMAND arm-none-eabi-gcc --print-sysroot
  OUTPUT_VARIABLE ARM_SYSROOT
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
set(CMAKE_SYSROOT ${ARM_SYSROOT})

# Set the (Cortex-M33 specific) compiler and linker flags
set(CMAKE_C_FLAGS "-mcpu=cortex-m33 -mthumb -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mcmse -Wall -ffunction-sections -fdata-sections -fomit-frame-pointer -fno-builtin")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS} -x assembler-with-cpp")
set(CMAKE_EXE_LINKER_FLAGS "-mcpu=cortex-m33 -mthumb -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mcmse -Wl,--gc-sections,--print-memory-usage,--no-warn-rwx-segments -specs=nano.specs")

# Set flags based on the build type
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Og -g -DDEBUG")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -DNDEBUG")
endif()