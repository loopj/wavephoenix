#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "si.h"

/**
 * Function type for command handlers.
 *
 * @param command the command to handle
 * @param callback function to call when the command is complete
 * @param context user-defined context
 *
 * @return 0 on success, negative error code on failure
 */
typedef int (*si_command_handler_fn)(const uint8_t *command, si_callback_fn callback, void *context);

/**
 * Register a command handler for commands from an SI host.
 *
 * @param command the command to handle
 * @param command_length the length of the command
 * @param handler the command handler function
 *
 */
void si_command_register(uint8_t command, uint8_t length, si_command_handler_fn handler, void *context);

/**
 * Get the expected length of an SI command.
 *
 * @param command the command to check
 *
 * @return the expected length of the command, in bytes, or 0 if the command is unknown
 */
uint8_t si_command_get_length(uint8_t command);

/**
 * Get the command handler for an SI command.
 *
 * @param command the command to check
 *
 * @return the command handler function, or NULL if the command is unknown
 */
si_command_handler_fn si_command_get_handler(uint8_t command);

/**
 * Process incoming SI commands.
 *
 * This function should be called periodically to check for incoming commands
 * and handle them as needed.
 */
void si_command_process();