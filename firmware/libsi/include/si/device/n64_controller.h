/**
 * SI command handling for N64 controllers.
 */

#pragma once

// N64 controller SI commands
#define SI_CMD_N64_POLL             0x01
#define SI_CMD_N64_POLL_LEN         1
#define SI_CMD_N64_POLL_RESP        4

struct si_device_n64_controller {
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