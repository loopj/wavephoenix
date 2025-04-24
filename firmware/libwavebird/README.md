# libwavebird

An open source implementation of the WaveBird protocol.

Currently supports Silicon Labs Gecko Series 1 and Series 2 SoCs. Tested with the EFR32FG1, EFR32FG14, EFR32MG22, and EFR32BG22 SoCs.

## Adding support for additional Gecko SoCs

In theory this library should work with any Gecko Series 1 or Series 2 SoC that has proprietary 2.4GHz support. A `.radioconf` file for that platform will need to be created in `config/rail`.

## Adding support for additional platforms

The BCH(31,21) logic and packet decoding logic is platform agnostic.

I currently don't know of any other SoCs which support the WaveBird's FSK+DSSS 15-chip modulation, but in case they do exist, I've tried to keep the code modular. The Silicon Labs Gecko specific code is restricted to `radio_efr32.c`. A new platform would need to provide implementations for the functions defined in `radio.h`.

## Running tests

- Build the test suite

    ```bash
    cmake -Bbuild && cmake --build build --target test_wavebird
    ```

- Run the tests

    ```bash
    ./build/test/test_wavebird
    ```
