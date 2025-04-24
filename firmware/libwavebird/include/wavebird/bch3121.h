/**
 * BCH(31,21) encoder and decoder, with error correction.
 *
 * WaveBird input states are comprised of 4, 31-bit BCH codewords, which are
 * interleaved to protect against burst errors.
 *
 * The BCH coding is (31,21), which means that 10 bits are used for error
 * correction, allowing for up to 2 errors to be corrected per codeword.
 *
 * We are using non-systematic BCH encoding.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#define BCH3121_CODEWORD_LEN  31
#define BCH3121_MESSAGE_LEN   21
#define BCH3121_ORDER         (1 << 10)

// Error codes
enum {
  BCH3121_ERR_DETECTED = 1,
  BCH3121_ERR_UNCORRECTABLE,
};

/**
 * Encode a 21-bit input into a 31-bit BCH codeword.
 *
 * @param message 21-bit input message
 *
 * @return 31-bit BCH codeword
 */
uint32_t bch3121_encode(uint32_t message);

/**
 * Decode a 31-bit BCH codeword.
 *
 * @param message 21-bit decoded message, which may contain errors
 * @param codeword 31-bit BCH codeword
 *
 * @return the syndrome of the codeword
 */
uint32_t bch3121_decode(uint32_t *message, uint32_t codeword);

/**
 * Decode a 31-bit BCH codeword, applying error correction if possible.
 *
 * @param message 21-bit decoded message, if successful
 * @param codeword 31-bit BCH codeword
 *
 * @return successful decodes will return the number of corrected errors,
 *         otherwise a negative error code will be returned
 */
int bch3121_decode_and_correct(uint32_t *message, uint32_t codeword);

/**
 * Generate a syndrome table for BCH(31,21) error correction.
 *
 * The syndrome table is a lookup table that maps syndromes to error patterns.
 *
 * @param syndrome_table 1024-element table to populate
 */
void bch3121_generate_syndrome_table(uint16_t *syndrome_table);