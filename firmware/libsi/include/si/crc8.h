#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * Calculate the CRC-8 checksum of a data buffer, using the polynomial 0x85.
 *
 * This is used with various SI commands that transfer blocks of data.
 *
 * @param data The data buffer
 * @param size The size of the data buffer
 */
uint8_t si_crc8(uint8_t *data, size_t size);