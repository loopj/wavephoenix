/**
 * SI command handling for N64 controllers.
 */

#pragma once

// N64 controller SI commands
#define SI_CMD_N64_POLL             0x01
#define SI_CMD_N64_POLL_LEN         1
#define SI_CMD_N64_POLL_RESP        4

/**
 * N64 controller input state.
 *
 * On the wire, the button state bits are sent in the following order:
 * A, B, Z, Start, Up, Down, Left, Right, Reset, 0, L, R, C-Up, C-Down, C-Left, C-Right
 */
struct si_device_n64_input_state {
  union {
    struct {
      // Byte 0
      uint8_t right : 1; // Bit 0
      uint8_t left  : 1; // Bit 1
      uint8_t down  : 1; // Bit 2
      uint8_t up    : 1; // Bit 3
      uint8_t start : 1; // Bit 4
      uint8_t z     : 1; // Bit 5
      uint8_t b     : 1; // Bit 6
      uint8_t a     : 1; // Bit 7

      // Byte 1
      uint8_t c_right : 1; // Bit 0
      uint8_t c_left  : 1; // Bit 1
      uint8_t c_down  : 1; // Bit 2
      uint8_t c_up    : 1; // Bit 3
      uint8_t r       : 1; // Bit 4
      uint8_t l       : 1; // Bit 5
      uint8_t         : 1; // Bit 6
      uint8_t rst     : 1; // Bit 7

    } __attribute__((packed));
    uint8_t bytes[2];
  } buttons;

  uint8_t stick_x;
  uint8_t stick_y;
} __attribute__((packed));

/**
 * N64 controller device state.
 */
struct si_device_n64_controller {
  uint8_t info[3];
  struct si_device_n64_input_state input;
};

/**
 * Initialize to present on the SI bus as an N64 controller.
 *
 * This function sets up the initial state, and registers SI command
 * handlers for OEM N64 controllers.
 *
 * @param device the device to initialize
 */
void si_device_n64_init(struct si_device_n64_controller *device);