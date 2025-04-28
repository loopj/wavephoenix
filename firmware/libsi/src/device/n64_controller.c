#include <string.h>

#include "si/si.h"

#include "si/commands.h"
#include "si/device/n64_controller.h"

/**
 * Handle "info" commands.
 *
 * Command:         {0x00}
 * Response:        A 3-byte device info.
 */
static int handle_info(const uint8_t *command, si_callback_fn callback, void *context)
{
  struct si_device_n64_controller *device = (struct si_device_n64_controller *)context;

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
  struct si_device_n64_controller *device = (struct si_device_n64_controller *)context;

  // Respond with the device info
  si_write_bytes(device->info, SI_CMD_INFO_RESP, callback);

  return SI_CMD_RESET_RESP;
}

/**
 * Handle "poll" commands.
 *
 * Command:         {0x01}
 * Response:        A 4-byte input state.
 */
static int handle_poll(const uint8_t *command, si_callback_fn callback, void *context)
{
  struct si_device_n64_controller *device = (struct si_device_n64_controller *)context;

  si_write_bytes((uint8_t *)&device->input, SI_CMD_N64_POLL_RESP, callback);

  return SI_CMD_N64_POLL_RESP;
}

void si_device_n64_init(struct si_device_n64_controller *device)
{
  // Present as a wired N64 controller, with no accessory
  device->info[0] = 0x05;
  device->info[1] = 0x00;
  device->info[2] = 0x02;

  // Resting state on the N64 controller is all zeros
  memset(&device->input, 0, sizeof(device->input));

  si_command_register(SI_CMD_INFO, SI_CMD_INFO_LEN, handle_info, device);
  si_command_register(SI_CMD_RESET, SI_CMD_RESET_LEN, handle_reset, device);
  si_command_register(SI_CMD_N64_POLL, SI_CMD_N64_POLL_LEN, handle_poll, device);
}