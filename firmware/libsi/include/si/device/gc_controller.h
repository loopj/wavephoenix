/**
 * SI command handling for GameCube controllers.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

/**
 * Rumble motor states.
 */
enum si_device_gc_motor_state {
  SI_DEVICE_GC_MOTOR_STOP,
  SI_DEVICE_GC_MOTOR_RUMBLE,
  SI_DEVICE_GC_MOTOR_STOP_HARD,
};

/**
 * Analog modes.
 */
enum {
  SI_DEVICE_GC_ANALOG_MODE_0,
  SI_DEVICE_GC_ANALOG_MODE_1,
  SI_DEVICE_GC_ANALOG_MODE_2,
  SI_DEVICE_GC_ANALOG_MODE_3,
  SI_DEVICE_GC_ANALOG_MODE_4,
};

/**
 * GameCube controller input state.
 *
 * On the wire, the button state bits are sent in the following order:
 * Error, Error Latch, Need Origin, Start, Y, X, B, A, Use Origin, L, R, Z, Up, Down, Right, Left
 */
struct si_device_gc_input_state {
  union {
    struct {
      // Byte 0
      uint8_t a           : 1; // Bit 0
      uint8_t b           : 1; // Bit 1
      uint8_t x           : 1; // Bit 2
      uint8_t y           : 1; // Bit 3
      uint8_t start       : 1; // Bit 4
      uint8_t need_origin : 1; // Bit 5, should the host fetch the origin?
      uint8_t error_latch : 1; // Bit 6, error code, latched
      uint8_t error       : 1; // Bit 7, error code

      // Byte 1
      uint8_t left       : 1; // Bit 0
      uint8_t right      : 1; // Bit 1
      uint8_t down       : 1; // Bit 2
      uint8_t up         : 1; // Bit 3
      uint8_t z          : 1; // Bit 4
      uint8_t r          : 1; // Bit 5
      uint8_t l          : 1; // Bit 6
      uint8_t use_origin : 1; // Bit 7, should the host use the origin?
    } __attribute__((packed));

    uint8_t bytes[2];
  } buttons;

  uint8_t stick_x;
  uint8_t stick_y;
  uint8_t substick_x;
  uint8_t substick_y;
  uint8_t trigger_left;
  uint8_t trigger_right;
  uint8_t analog_a;
  uint8_t analog_b;
} __attribute__((packed));

/**
 * GameCube controller device state.
 */
struct si_device_gc_controller {
  uint8_t info[3];
  struct si_device_gc_input_state origin;
  struct si_device_gc_input_state input;
  bool input_valid;
};

/**
 * Initialize to present on the SI bus as a GameCube controller.
 *
 * This function sets up the initial state, and registers SI command
 * handlers for OEM GameCube controller, and WaveBird controller commands.
 *
 * @param device the device to initialize
 * @param type the device type flags
 * @param input_state pointer to an input state buffer
 */
void si_device_gc_init(struct si_device_gc_controller *device, uint8_t type);

/**
 * Set the wireless ID of the controller.
 *
 * Wireless IDs are 10-bit numbers used to identify a WaveBird controller.
 * Although these IDs aren’t globally unique, they are assumed to be distinct enough
 * so that it’s unlikely for a single user to have two controllers with the same ID.
 * The ID helps bind a controller to a specific port after data reception.
 *
 * @param device the device to set the wireless ID for
 * @param wireless_id the new 10-bit wireless ID
 */
void si_device_gc_set_wireless_id(struct si_device_gc_controller *device, uint16_t wireless_id);

/**
 * Get the current wireless ID of the controller.
 *
 * @param device the device to get the wireless ID from
 *
 * @return the current 10-bit wireless ID
 */
static inline uint16_t si_device_gc_get_wireless_id(struct si_device_gc_controller *device)
{
  return (device->info[1] & 0xC0) << 2 | device->info[2];
}

/**
 * Determine if the wireless ID has been fixed.
 *
 * Fixing the wireless ID is used to bind a WaveBird controller to a specific receiver.
 *
 * @param device the device to check
 *
 * @return true if the wireless ID is fixed
 */
static inline bool si_device_gc_wireless_id_fixed(struct si_device_gc_controller *device)
{
  return device->info[1] & SI_WIRELESS_FIX_ID;
}

/**
 * Mark the input state as valid.
 *
 * @param device the device to set the input state for
 * @param valid true if the input state is valid
 */
static inline void si_device_set_input_valid(struct si_device_gc_controller *device, bool valid)
{
  device->input_valid = valid;
}