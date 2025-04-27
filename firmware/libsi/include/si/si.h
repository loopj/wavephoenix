/**
 * SI (Serial Interface) protocol.
 *
 * SI is a half-duplex, asynchronous serial protocol using a single, open-drain
 * line with an external pull-up resistor.
 *
 * The operating voltage is 3.3V.
 *
 * OEM GameCube controllers send SI pulses at 250 kHz (a 4µs period):
 * - Logic 0 pulse:   3µs low, 1µs high
 * - Logic 1 pulse:   1µs low, 3µs high
 * - Stop bit:        2µs low, 2µs high
 *
 * WaveBird receivers send SI pulses at 225 kHz (a 4.44µs period):
 * - Logic 0 pulse:   3.33µs low, 1.11µs high
 * - Logic 1 pulse:   1.11µs low, 3.33µs high
 * - Stop bit:        2.22µs low, 2.22µs high
 *
 * A GameCube/Wii console sends SI pulses at 200 kHz (a 5µs period):
 * - Logic 0 pulse:   3.75µs low, 1.25µs high
 * - Logic 1 pulse:   1.25µs low, 3.75µs high
 * - Stop bit:        1.25µs low, 3.75µs high (same as logic 1)
 *
 * Communication:
 * - Host (console) sends a 1-3 byte command to a device (controller).
 * - Device responds with a multi-byte response.
 * - Command and responses are terminated with a stop bit.
 *
 * Commands:
 * - 0x00 - Get device type and status
 * - 0xFF - Reset device
 * - Other commands are device-specific
 *
 * Implementation:
 * - Implementations need to clock in and out pulses on the SI line quickly and accurately.
 * - Bit-banging is not feasible due to the tight timing requirements.
 * - Recommended to use timer peripherals to capture pulses.
 * - Recommended to use uart/timer peripherals to transmit pulses.
 * - Recommended to use DMA to transfer data between the uart/timer and memory.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

// SI transfers are max 64 bytes
#define SI_BLOCK_SIZE               64

// Common commands
#define SI_CMD_RESET                0xFF
#define SI_CMD_RESET_LEN            1
#define SI_CMD_RESET_RESP           3

#define SI_CMD_INFO                 0x00
#define SI_CMD_INFO_LEN             1
#define SI_CMD_INFO_RESP            3

// SI device info flags
// On wireless controllers 0x00C0FF is reserved for the controller ID

// Byte 0
#define SI_GC_STANDARD              0x01
#define SI_WIRELESS_STATE           0x02
#define SI_TYPE_GC                  0x08
#define SI_GC_NOMOTOR               0x20
#define SI_WIRELESS_RECEIVED        0x40
#define SI_GC_WIRELESS              0x80

// Byte 1
#define SI_WIRELESS_FIX_ID          0x10
#define SI_WIRELESS_ORIGIN          0x20

// Byte 2
#define SI_HAS_ERROR                0x80
#define SI_HAS_LATCHED_ERROR        0x40
#define SI_NEED_ORIGIN              0x20
#define SI_MOTOR_STATE_MASK         0x18
#define SI_ANALOG_MODE_MASK         0x07

// SI modes
enum {
  SI_MODE_HOST,
  SI_MODE_DEVICE,
};

// Error codes
enum {
  SI_ERR_NOT_READY = 1,
  SI_ERR_UNKNOWN_COMMAND,
  SI_ERR_INVALID_COMMAND,
  SI_ERR_TRANSFER_FAILED,
  SI_ERR_TRANSFER_TIMEOUT,
};

/**
 * Function type for transfer completion callbacks.
 *
 * @param result 0 on success, negative error code on failure
 */
typedef void (*si_callback_fn)(int result);

/**
 * Initialize the SI bus.
 *
 * @param port the GPIO port to use
 * @param pin the GPIO pin to use
 * @param mode the SI mode (host or device)
 * @param rx_freq the receive frequency, in Hz
 * @param tx_freq the transmit frequency, in Hz
 */
void si_init(uint8_t port, uint8_t pin, uint8_t mode, uint32_t rx_freq, uint32_t tx_freq);

/**
 * Write data to the SI bus.
 *
 * @param data the data to send
 * @param length the length of the data
 * @param callback function to call when the transfer is complete
 */
void si_write_bytes(const uint8_t *data, uint8_t length, si_callback_fn callback);

/**
 * Read data from the SI bus.
 *
 * @param buffer the buffer to read into
 * @param length the number of bytes expected
 * @param callback function to call when the transfer is complete
 */
void si_read_bytes(uint8_t *buffer, uint8_t length, si_callback_fn callback);

/**
 * Read a single command from the SI bus.
 *
 * @param buffer the buffer to read into
 * @param callback function to call when the command has been read
 */
void si_read_command(uint8_t *buffer, si_callback_fn callback);

/**
 * Wait for the SI bus to be idle.
 *
 * This function will block until the SI bus is idle.
 */
void si_await_bus_idle(void);