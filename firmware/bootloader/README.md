# WavePhoenix Bootloader

## Dependencies

- [GNU Arm Embedded Toolchain](https://developer.arm.com/Tools%20and%20Software/GNU%20Toolchain)
- [Silicon Labs Simplicity SDK](https://github.com/SiliconLabs/simplicity_sdk)
- [Silicon Labs Configurator (SLC-CLI)](https://www.silabs.com/software-and-tools/simplicity-studio/configurator-command-line-interface) (requires Java)

## Configuration

You'll need to tell SLC-CLI where your Simplicity SDK and ARM toolchain are located:

```bash
slc configuration --sdk path/to/simplicity_sdk
slc configuration --gcc-toolchain path/to/gcc-arm-none-eabi
slc signature trust --sdk path/to/simplicity_sdk
```

## Building

To build the WavePhoenix bootloader, run the following script, replacing `rf-bm-bg22c3` with your target board:

```bash
BOARD=rf-bm-bg22c3 ./build.sh
```

This will generate `target/rf-bm-bg22c3/build/release/bootloader.hex`.

## Flashing

### Using Simplicity Commander

If you are using a dev board with a built-in J-Link debugger, you can use Simplicity Commander to flash the bootloader:

```bash
commander flash bootloader.hex
```

### Using OpenOCD

If you are using a generic SWD debug probe, you can use OpenOCD to flash the bootloader:

1. Download and install Arduino's fork of OpenOCD ([Windows](https://downloads.arduino.cc/tools/openocd-0.12.0-arduino1-static-i686-w64-mingw32.tar.bz2), [macOS](https://downloads.arduino.cc/tools/openocd-0.12.0-arduino1-static-x86_64-apple-darwin19.tar.bz2), [Linux](https://downloads.arduino.cc/tools/openocd-0.12.0-arduino1-static-x86_64-ubuntu16.04-linux-gnu.tar.bz2))

2. Connect GND, SWDIO, SWCLK, and 3V3 between the debug probe and the SoC.

3. If this is your first time flashing this chip, you'll need to do a full device erase to unlock the debug interface:

      ```bash
      openocd -f "interface/cmsis-dap.cfg" \
              -f "target/efm32s2.cfg" \
              -c "init; efm32s2_dci_device_erase; shutdown"
      ```

4. Disconnect then reconnect the debug probe.

5. Then, flash the bootloader, replacing `bootloader.hex` with the path to your bootloader file:

    ```bash
    openocd -f interface/cmsis-dap.cfg \
          -c "transport select swd" \
          -f target/efm32s2.cfg \
          -c "init; halt; flash write_image erase bootloader.hex; exit"
    ```