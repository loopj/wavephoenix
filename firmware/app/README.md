# WavePhoenix App Firmware

## Dependencies

- [GNU Arm Embedded Toolchain](https://developer.arm.com/Tools%20and%20Software/GNU%20Toolchain)
- [Silicon Labs Simplicity SDK](https://github.com/SiliconLabs/simplicity_sdk)
- [Silicon Labs Configurator (SLC-CLI)](https://www.silabs.com/software-and-tools/simplicity-studio/configurator-command-line-interface) (requires Java)
- [Simplicity Commander](https://www.silabs.com/developers/simplicity-studio/simplicity-commander?tab=downloads) (optional, required to generate OTA binaries)

## Configuration

You'll need to tell SLC-CLI where your Simplicity SDK and ARM toolchain are located:

```bash
slc configuration --sdk path/to/simplicity_sdk
slc configuration --gcc-toolchain path/to/gcc-arm-none-eabi
slc signature trust --sdk path/to/simplicity_sdk
```

## Building

To build the WavePhoenix app, run the following script, replacing `rf-bm-bg22c3` with your target board:

```bash
BOARD=rf-bm-bg22c3 ./build.sh
```

This will generate:

- `target/rf-bm-bg22c3/build/release/wavephoenix.hex` - The firmware binary for SWD flashing
- `target/rf-bm-bg22c3/build/release/wavephoenix.gbl` - The firmware binary for OTA flashing

## Flashing

> [!IMPORTANT]
> A bootloader **must** be installed before flashing the app firmware. See the [`bootloader`](../bootloader) directory for more information.

### Over-the-air (OTA) flashing

If you have a computer or phone with Bluetooth, you can use [WavePhoenix Web](https://web.wavephoenix.com) to flash the firmware.

### Using Simplicity Commander

If you are using a dev board with a built-in J-Link debugger, you can use Simplicity Commander to flash the firmware:

```bash
commander flash wavephoenix.hex
```

### Using OpenOCD

If you are using a generic SWD debug probe, you can use OpenOCD to flash the firmware:

1. Download and install Arduino's fork of OpenOCD ([Windows](https://downloads.arduino.cc/tools/openocd-0.12.0-arduino1-static-i686-w64-mingw32.tar.bz2), [macOS](https://downloads.arduino.cc/tools/openocd-0.12.0-arduino1-static-x86_64-apple-darwin19.tar.bz2), [Linux](https://downloads.arduino.cc/tools/openocd-0.12.0-arduino1-static-x86_64-ubuntu16.04-linux-gnu.tar.bz2))

2. Connect GND, SWDIO, SWCLK, and 3V3 between the debug probe and the SoC
3. Flash the firmware, replacing `wavephoenix.hex` with the path to your firmware file:

    ```bash
    openocd -f interface/cmsis-dap.cfg \
          -c "transport select swd" \
          -f target/efm32s2.cfg \
          -c "init; halt; flash write_image erase wavephoenix.hex; exit"
    ```

## Customizing the firmware

The WavePhoenix firmware can be customized by setting various flags in board configuration files. See the [`boards`](../boards) folder for examples.

### Joybus Configuration

The Joybus GPIO is the communication line between the WavePhoenix and the console, and is **required** for WavePhoenix to function.

Define the following:

- `JOYBUS_PORT` - The GPIO port for the SI data line (e.g. `gpioPortD`)
- `JOYBUS_PIN` - The GPIO pin for the SI data line (e.g. `3`)
- `JOYBUS_TIMER` - The timer peripheral to use in `libjoybus` (e.g. `TIMER0`)
- `JOYBUS_USART` - The USART peripheral to use in `libjoybus` (e.g. `USART0`)

### Status LED

The WavePhoenix receiver firmware supports an optional status LED. The LED will blink on radio activity, just like the original WaveBird receiver. The LED will also indicate pairing status in virtual pairing mode.

To enable the optional status LED, define the following:

- `HAS_STATUS_LED` - Set to `1` to enable status LED support
- `STATUS_LED_PORT` - The GPIO port for the status LED
- `STATUS_LED_PIN` - The GPIO pin for the status LED
- `STATUS_LED_INVERT` - Set to `1` to invert the LED logic (i.e. LED on when GPIO is low)

### Pairing button

The WavePhoenix receiver firmware supports a one-button "virtual pairing" mode. Connect one side of a switch to a GPIO, and the other side to ground. When the button is pressed, the receiver will enter pairing mode. Pressing and holding buttons on the controller will cause the receiver to pair with the controller. The exact behavior is
customizable by passing a pairing qualification function to `wavebird_radio_configure_qualification`.

To enable the pairing button, define the following:

- `HAS_PAIR_BTN` - Set to `1` to enable pairing button support
- `PAIR_BTN_PORT` - The GPIO port for the pairing button
- `PAIR_BTN_PIN` - The GPIO pin for the pairing button

### Channel wheel

The WavePhoenix receiver firmware supports an optional 16-position channel selection wheel, just like the original WaveBird receiver. The channel wheel is a rotary DIP switch with 4 data pins, and one common pin. The common pin should be connected to ground, and the data pins should be connected to GPIOs.

Due to the way interrupts work on EFR32 devices, the pin numbers must be different for each channel wheel pin, even if they are on the different ports.

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
