#include <string.h>

#include "si/commands.h"
#include "si/device/gc_controller.h"

/*
 * Pack an "full" input state into a "short" input state, depending on the analog mode.
 *
 * The "short poll" command used by games expects 8-byte responses, this is presumably
 * so it fit in a nice round multiple of 32-bit words.
 *
 * The full input state is 10 bytes long, so there are various ways to "pack" the input
 * state into 8 bytes. Depending on the analog mode, either one pair of analog inputs
 * can be omitted, or two pairs of analog inputs can be truncated to 4 bits.
 *
 * All production games, with the exception of Luigi's Mansion, use analog mode 3. This
 * mode omits the analog A/B inputs, and sends the substick X/Y and triggers at full
 * precision. Analog A/B buttons were only present in pre-production GameCube
 * controllers.
 */
static uint8_t *pack_input_state(struct si_device_gc_input_state *src, uint8_t analog_mode)
{
  // Buffer for packed input state
  static uint8_t packed_state[8];

  // Copy the button and stick data
  memcpy(packed_state, src, 4);

  // Pack the remaining analog input data
  switch (analog_mode) {
    default:
      // Substick X/Y full precision, triggers and analog A/B truncated to 4 bits
      packed_state[4] = src->substick_x;
      packed_state[5] = src->substick_y;
      packed_state[6] = (src->trigger_left & 0xF0) | (src->trigger_right >> 4);
      packed_state[7] = (src->analog_a & 0xF0) | (src->analog_b >> 4);
      break;
    case SI_DEVICE_GC_ANALOG_MODE_1:
      // Triggers full precision, substick X/Y and analog A/B truncated to 4 bits
      packed_state[4] = (src->substick_x & 0xF0) | (src->substick_y >> 4);
      packed_state[5] = src->trigger_left;
      packed_state[6] = src->trigger_right;
      packed_state[7] = (src->analog_a & 0xF0) | (src->analog_b >> 4);
      break;
    case SI_DEVICE_GC_ANALOG_MODE_2:
      // Analog A/B full precision, substick X/Y and triggers truncated to 4 bits
      packed_state[4] = (src->substick_x & 0xF0) | (src->substick_y >> 4);
      packed_state[5] = (src->trigger_left & 0xF0) | (src->trigger_right >> 4);
      packed_state[6] = src->analog_a;
      packed_state[7] = src->analog_b;
      break;
    case SI_DEVICE_GC_ANALOG_MODE_3:
      // Substick X/Y and triggers full precision, analog A/B omitted
      packed_state[4] = src->substick_x;
      packed_state[5] = src->substick_y;
      packed_state[6] = src->trigger_left;
      packed_state[7] = src->trigger_right;
      break;
    case SI_DEVICE_GC_ANALOG_MODE_4:
      // Substick X/Y and analog A/B full precision, triggers omitted
      packed_state[4] = src->substick_x;
      packed_state[5] = src->substick_y;
      packed_state[6] = src->analog_a;
      packed_state[7] = src->analog_b;
      break;
  }

  return packed_state;
}

/**
 * Handle "info" commands.
 *
 * Command:         {0x00}
 * Response:        A 3-byte device info.
 */
static int handle_info(const uint8_t *command, si_callback_fn callback, void *context)
{
  struct si_device_gc_controller *device = (struct si_device_gc_controller *)context;

  // Respond with the device info
  si_write_bytes(device->info, SI_CMD_INFO_RESP, callback);

  return SI_CMD_INFO_RESP;
}

/**
 * Handle "reset" commands.
 *
 * Command:         {0xFF}
 * Response:        A 3-byte device info.
 */
static int handle_reset(const uint8_t *command, si_callback_fn callback, void *context)
{
  struct si_device_gc_controller *device = (struct si_device_gc_controller *)context;

  // TODO: Stop the rumble motor, if active

  // Respond with the device type and status
  si_write_bytes(device->info, SI_CMD_RESET_RESP, callback);

  return SI_CMD_RESET_RESP;
}

/**
 * Handle "short poll" commands, to fetch the current input state.
 *
 * Command:         {0x40, analog_mode, motor_state}
 * Response:        An 8-byte packed input state, see `pack_input_state` for details
 */
static int handle_short_poll(const uint8_t *command, si_callback_fn callback, void *context)
{
  struct si_device_gc_controller *device = (struct si_device_gc_controller *)context;

  // Extract the analog mode and motor state from the command
  uint8_t analog_mode = command[1] & 0x07;
  uint8_t motor_state = command[2] & 0x03;

  if (!(device->info[0] & SI_GC_WIRELESS)) {
    // Update the origin flags
    device->input.buttons.need_origin = (device->info[2] & SI_NEED_ORIGIN) != 0;
    device->input.buttons.use_origin  = true;

    // Save the analog mode and motor state
    device->info[2] &= ~(SI_MOTOR_STATE_MASK | SI_ANALOG_MODE_MASK);
    device->info[2] |= motor_state << 3 | analog_mode;
  }

  // If the input state is valid, use that for the response, otherwise use the origin
  struct si_device_gc_input_state *state = device->input_valid ? &device->input : &device->origin;

  // Most games use analog mode 3, which is just the first 8 bytes of the full input state
  // Otherwise, pack the input state based on the analog mode
  uint8_t *short_state;
  if (analog_mode == SI_DEVICE_GC_ANALOG_MODE_3) {
    short_state = (uint8_t *)state;
  } else {
    short_state = pack_input_state(state, analog_mode);
  }

  // Respond with the 8-byte "short" input state
  si_write_bytes(short_state, SI_CMD_GC_SHORT_POLL_RESP, callback);

  return SI_CMD_GC_SHORT_POLL_RESP;
}

/**
 * Handle "read origin" commands.
 *
 * Command:         {0x41}
 * Response:        A 10-byte input state representing the current origin.
 */
static int handle_read_origin(const uint8_t *command, si_callback_fn callback, void *context)
{
  struct si_device_gc_controller *device = (struct si_device_gc_controller *)context;

  // Tell the host it no longer needs to fetch the origin
  if (!(device->info[0] & SI_GC_WIRELESS)) {
    device->info[2] &= ~SI_NEED_ORIGIN;
  }

  // Clear the "need origin" flag
  device->input.buttons.need_origin = false;

  // Respond with the origin
  si_write_bytes((uint8_t *)(&device->origin), SI_CMD_GC_READ_ORIGIN_RESP, callback);

  return SI_CMD_GC_READ_ORIGIN_RESP;
}

/**
 * Handle "calibrate" commands.
 *
 * Command:         {0x42, 0x00, 0x00}
 * Response:        A 10-byte input state representing the current origin.
 */
static int handle_calibrate(const uint8_t *command, si_callback_fn callback, void *context)
{
  struct si_device_gc_controller *device = (struct si_device_gc_controller *)context;

  // Set current analog input state as the origin
  device->origin.stick_x       = device->input.stick_x;
  device->origin.stick_y       = device->input.stick_y;
  device->origin.substick_x    = device->input.substick_x;
  device->origin.substick_y    = device->input.substick_y;
  device->origin.trigger_left  = device->input.trigger_left;
  device->origin.trigger_right = device->input.trigger_right;

  // Tell the host it no longer needs to fetch the origin
  if (!(device->info[0] & SI_GC_WIRELESS)) {
    device->info[2] &= ~SI_NEED_ORIGIN;
  }

  // Respond with the new origin
  si_write_bytes((uint8_t *)(&device->origin), SI_CMD_GC_CALIBRATE_RESP, callback);

  return SI_CMD_GC_CALIBRATE_RESP;
}

/**
 * Handle "long poll" commands, to fetch the current input state with full precision.
 *
 * Command Format:  {0x43, analog_mode, motor_state}
 * Response:        A 10-byte input state.
 *
 * NOTE: This command is not used by any games, but is included for completeness.
 */
static int handle_long_poll(const uint8_t *command, si_callback_fn callback, void *context)
{
  struct si_device_gc_controller *device = (struct si_device_gc_controller *)context;

  // Extract the analog mode and motor state from the command
  uint8_t analog_mode = command[1] & 0x07;
  uint8_t motor_state = command[2] & 0x03;

  // Update the origin flags before responding
  device->input.buttons.need_origin = (device->info[2] & SI_NEED_ORIGIN) != 0;
  device->input.buttons.use_origin  = true;

  // Save the analog mode and motor state
  if (!(device->info[0] & SI_GC_WIRELESS)) {
    device->info[2] &= ~(SI_MOTOR_STATE_MASK | SI_ANALOG_MODE_MASK);
    device->info[2] |= motor_state << 3 | analog_mode;
  }

  // Respond with the current input state
  si_write_bytes((uint8_t *)&device->input, SI_CMD_GC_LONG_POLL_RESP, callback);

  return SI_CMD_GC_LONG_POLL_RESP;
}

/**
 * Handle "fix device" commands, to "fix" the receiver ID to a specific controller ID.
 *
 * This is used to pair a WaveBird controller with a specific receiver.
 *
 * Command:         {0x4E, wireless_id_h | SI_WIRELESS_FIX_ID, wireless_id_l}
 * Response:        A 3-byte device info.
 */
static int handle_fix_device(const uint8_t *command, si_callback_fn callback, void *context)
{
  struct si_device_gc_controller *device = (struct si_device_gc_controller *)context;

  // Extract the wireless ID from the command
  uint16_t wireless_id = ((command[1] & 0xC0) << 2) | command[2];

  // Set the wireless id in the device info
  device->info[1] = (device->info[1] & ~0xC0) | ((wireless_id >> 2) & 0xC0);
  device->info[2] = wireless_id & 0xFF;

  // Update other device info flags
  device->info[0] |= SI_WIRELESS_STATE;
  device->info[1] |= SI_WIRELESS_FIX_ID;

  // Respond with the new device info
  si_write_bytes(device->info, SI_CMD_GC_FIX_DEVICE_RESP, callback);

  return SI_CMD_GC_FIX_DEVICE_RESP;
}

void si_device_gc_init(struct si_device_gc_controller *device, uint8_t type)
{
  // Set the initial device info flags
  device->info[0] = type;
  device->info[1] = 0x00;
  device->info[2] = 0x00;

  // Set the initial origin
  memset(&device->origin, 0, sizeof(device->origin));
  device->origin.stick_x    = 0x80;
  device->origin.stick_y    = 0x80;
  device->origin.substick_x = 0x80;
  device->origin.substick_y = 0x80;

  // Set the initial input state
  memcpy(&device->input, &device->origin, sizeof(device->input));

  // Mark the input as valid initially
  device->input_valid = true;

  // Request the origin on non-wireless controllers
  if (!(type & SI_GC_WIRELESS))
    device->info[2] = SI_NEED_ORIGIN;

  // Register the SI commands handled by GameCube controllers
  si_command_register(SI_CMD_INFO, SI_CMD_INFO_LEN, handle_info, device);
  si_command_register(SI_CMD_GC_SHORT_POLL, SI_CMD_GC_SHORT_POLL_LEN, handle_short_poll, device);
  si_command_register(SI_CMD_GC_READ_ORIGIN, SI_CMD_GC_READ_ORIGIN_LEN, handle_read_origin, device);
  si_command_register(SI_CMD_GC_CALIBRATE, SI_CMD_GC_CALIBRATE_LEN, handle_calibrate, device);
  si_command_register(SI_CMD_GC_LONG_POLL, SI_CMD_GC_LONG_POLL_LEN, handle_long_poll, device);
  si_command_register(SI_CMD_RESET, SI_CMD_RESET_LEN, handle_reset, device);

  // Register additional commands handled by WaveBird receivers
  if (type & SI_GC_WIRELESS) {
    si_command_register(SI_CMD_GC_FIX_DEVICE, SI_CMD_GC_FIX_DEVICE_LEN, handle_fix_device, device);
  }
}

void si_device_gc_set_wireless_id(struct si_device_gc_controller *device, uint16_t wireless_id)
{
  if (si_device_gc_wireless_id_fixed(device))
    return;

  // Set the wireless ID in the device info
  device->info[1] = (device->info[1] & ~0xC0) | ((wireless_id >> 2) & 0xC0);
  device->info[2] = wireless_id & 0xFF;

  // Update other device info flags
  device->info[0] |= SI_GC_STANDARD | SI_WIRELESS_RECEIVED;
  device->info[1] |= SI_WIRELESS_ORIGIN;
}