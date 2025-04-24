# WavePhoenix Receiver Firmware

This is the firmware for the WavePhoenix receiver, a replacement for the original WaveBird receiver. The WavePhoenix receiver is based on the EFR32BG22 SoC, and is compatible with the original WaveBird controller.

## Building

### Dependencies

- [CMake](https://cmake.org/)
- [Ninja](https://ninja-build.org/)
- [GNU Arm Embedded Toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm)
- [Silicon Labs Gecko SDK](https://github.com/SiliconLabs/Gecko_SDK) (requires Git LFS)
- [Simplicity Commander](https://www.silabs.com/developers/simplicity-studio/simplicity-commander?tab=downloads) (optional, to generate OTA binaries)
- Python 3.10 and packages `jinja2`, `pyyaml`, `numpy` and `scipy`

### Configuration

- The `GECKO_SDK_PATH` env variable should be set to the path of the Gecko SDK
- The `SIMPLICITY_COMMANDER_PATH` env variable should be set to the path of Simplicity Commander, if installed

### Building for RF-BM-BG22C3

From the `firmware` directory, run:

```bash
cmake --preset rf-bm-bg22c3 && cmake --build --preset rf-bm-bg22c3
```

This will generate:

- `build/rf-bm-bg22c3/receiver/receiver.hex` - The firmware binary for SWD flashing
- `build/rf-bm-bg22c3/receiver/receiver.gbl` - The firmware binary for OTA flashing

## Flashing

> [!NOTE]
> A bootloader **must** be installed for the receiver firmware to boot. See the `bootloader` directory for more information.

### Over-the-air (OTA) flashing

The easiest way to flash the firmware is to use the OTA update feature of the bootloader. If you have a computer with Bluetooth, you can use the [WavePhoenix CLI](https://github.com/loopj/wavephoenix-cli) to flash the firmware:

```bash
wavephoenix flash firmware.gbl
```

Alternatively, you can use the [Simplicity Connect mobile app](https://www.silabs.com/developer-tools/simplicity-connect-mobile-app?tab=downloads) to flash the firmware.

### Using OpenOCD

If you have a generic SWD debugger, you can use OpenOCD to flash the firmware. The [Raspberry Pi Debug Probe](https://www.raspberrypi.com/products/debug-probe/) is a good, affordable option. If you happen to have a Raspberry Pi Pico, you can install the [debugprobe firmware](https://github.com/raspberrypi/debugprobe) on it to turn it into an SWD debug probe!

- Download and install [OpenOCD pre-patched with EFM32 Series 2 support](https://github.com/loopj/openocd-efm32s2)
- Connect GND, SWDIO, SWCLK, and 3V3 between the debug probe and the SoC
- Flash the firmware:

    ```bash
    cmake --build --preset release --target receiver_flash_openocd
    ```

### Using JLink

If you have a JLink debugger, you can use the JLink Command Line Tools to flash the firmware.

- Install the [JLink Command Line Tools](https://www.segger.com/downloads/jlink/)
- Connect GND, SWDIO, SWCLK between the debug probe and the SoC
- Supply power to the device, and connect 3V3 to VTREF on the debugger
- Flash the firmware:

    ```bash
    echo -e "h\nloadfile receiver.hex\nr\ng\nqc" | JLinkExe -nogui 1 -device EFR32BG22C224F512GM32 -if SWD -speed 4000
    ```

## Customizing the firmware

The receiver firmware can be customized by setting various flags in a `board_config.h` file in a subdirectory of the
`config` directory. Setting the CMake variable `BOARD` to the name of the subdirectory will
include the `board_config.h` file in the build.

### SI data line

The SI data line is the communication line between the receiver and the console, and is **required** for
WavePhoenix to function.

Define the following:

- `SI_DATA_PORT` - The GPIO port for the SI data line
- `SI_DATA_PIN` - The GPIO pin for the SI data line

### Status LED

The WavePhoenix receiver firmware supports an optional status LED. The LED will blink on radio activity, just like
the original WaveBird receiver. The LED will also indicate pairing status in virtual pairing mode.

To enable the optional status LED, define the following:

- `HAS_STATUS_LED` - Set to `1` to enable status LED support
- `STATUS_LED_PORT` - The GPIO port for the status LED
- `STATUS_LED_PIN` - The GPIO pin for the status LED
- `STATUS_LED_INVERT` - Set to `1` to invert the LED logic (i.e. LED on when GPIO is low)

### Pairing button

The WavePhoenix receiver firmware supports a one-button "virtual pairing" mode. Connect one side of a switch to a
GPIO, and the other side to ground. When the button is pressed, the receiver will enter pairing mode. Pressing and
holding buttons on the controller will cause the receiver to pair with the controller. The exact behavior is
customizable by passing a pairing qualification function to `wavebird_radio_configure_qualification`.

To enable the pairing button, define the following:

- `HAS_PAIR_BTN` - Set to `1` to enable pairing button support
- `PAIR_BTN_PORT` - The GPIO port for the pairing button
- `PAIR_BTN_PIN` - The GPIO pin for the pairing button

### Channel wheel

The WavePhoenix receiver firmware supports an optional 16-position channel selection wheel, just like the original
WaveBird receiver. The channel wheel is a rotary DIP switch with 4 data pins, and one common pin. The common pin
should be connected to ground, and the data pins should be connected to GPIOs.

Due to the way interrupts work on EFR32 devices, the pin numbers must be different for each channel wheel pin,
even if they are on the different ports.

To enable the optional channel selection wheel, define the following:

- `HAS_CHANNEL_WHEEL` - Set to `1` to enable channel wheel support
- `CHANNEL_WHEEL_PORT_0` - The GPIO port for the first channel wheel pin
- `CHANNEL_WHEEL_PIN_0` - The GPIO pin for the first channel wheel pin
- `CHANNEL_WHEEL_PORT_1` - The GPIO port for the second channel wheel pin
- `CHANNEL_WHEEL_PIN_1` - The GPIO pin for the second channel wheel pin
- `CHANNEL_WHEEL_PORT_2` - The GPIO port for the third channel wheel pin
- `CHANNEL_WHEEL_PIN_2` - The GPIO pin for the third channel wheel pin
- `CHANNEL_WHEEL_PORT_3` - The GPIO port for the fourth channel wheel pin
- `CHANNEL_WHEEL_PIN_3` - The GPIO pin for the fourth channel wheel pin
