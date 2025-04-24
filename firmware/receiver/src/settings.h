#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * Initialize the settings structure.
 *
 * @param settings pointer to the settings structure to initialize
 * @param size size of the settings structure
 * @param signature signature to detect if the settings have been initialized
 * @param defaults pointer to the default settings
 */
void settings_init(void *settings, size_t size, uint32_t signature, const void *defaults);

/**
 * Save the persistent settings to USERDATA.
 *
 * @param settings pointer to the settings structure to save
 * @param size size of the settings structure
 */
void settings_save(void *settings, size_t size);