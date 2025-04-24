# Bootloader

Gecko Bootloader with OTA update support.

## Building

Generate a cmake project from the .slcp file (replace rf-bm-bg22c with your board):

```sh
slc generate --project-file bootloader-rf-bm-bg22c.slcp --export-destination bootloader_project --output-type cmake
```

Build the project:

```sh
cd bootloader_project/bootloader_cmake
cmake --workflow --preset project
cmake --preset project && cmake --build --preset default_config
```

## Flashing

### Using OpenOCD

```bash
openocd -f "interface/cmsis-dap.cfg" -c "transport select swd" -f "target/efm32s2.cfg" -c "init; halt; flash write_image erase bootloader_project/bootloader_cmake/build/default_config/bootloader.hex; reset run; exit"
```

### Using probe-rs

```bash
probe-rs download --chip EFR32BG22C224F512 --binary-format hex bootloader_project/bootloader_cmake/build/default_config/bootloader.hex
```
