#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "si/si.h"

/**
 * Function type for command handlers.
 *
 * @param command the command to handle
 * @param callback function to call when the command is complete
 * @param context user-defined context
 *
 * @return 0 on success, negative error code on failure
 */
typedef int (*si_command_handler_fn)(const uint8_t *command, si_complete_cb_t callback, void *context);

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
 * Process a single SI command on the bus.
 *
 * This will read a command from the SI bus and call the registered handler.
 */
void si_command_process();

/**
 * Enable automatic command processing.
 *
 * After each command is processed, the SI bus will automatically
 * transition back to RX mode and wait for the next command.
 */
void si_command_processing_enable();

/**
 * Disable automatic command processing.
 *
 * If a command is currently being processed, it will complete,
 * but no further commands will be processed until
 * `si_command_processing_enable()` is called again.
 */
void si_command_processing_disable();