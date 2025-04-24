# libsi

A partial implementation of the SI (Serial Interface) protocol used by N64 and GameCube controllers.

## Platforms

- EFR32 Series 1 and Series 2 SoCs

## Running tests

- Build the test suite

    ```bash
    cmake -Bbuild && cmake --build build --target test_si
    ```

- Run the tests

    ```bash
    ./build/test/test_si
    ```
