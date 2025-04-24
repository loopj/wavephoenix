/**
 * WaveBird packet decoding functions.
 *
 * After FSK-DSSS demodulation, a WaveBird frame is 25 bytes / 200 bits long,
 * and is structured as follows:
 *
 * 0xFAAAAAAA1234XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXYYYYZZZ
 *
 * The first 6 bytes are the preamble (FAAAAAAA) and sync word (1234). These
 * are removed by the PHY layer (radio).
 *
 * The remaining 19 bytes are the WaveBird packet, structured as follows:
 *
 * 0xXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXYYYYZZZ
 * - X:     encoded payload (124 bits)
 * - YYYY:  CRC (16 bits)
 * - ZZZ:   footer - seems to be a fixed value, but varies between controllers
 *                   seen values 0x000, 0x010, 0x110, 0x120
 *
 * The 124-bit encoded payload is comprised of 4 BCH(31,21) codewords, interleaved to
 * protect against burst errors.
 *
 * After deinterleaving and decoding, we are left with an 84-bit message.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

#define WAVEBIRD_PACKET_BYTES 19
#define WAVEBIRD_MESSAGE_BYTES 11

/**
 * Packet decoding error codes
 */
enum {
  WB_PACKET_ERR_CRC_MISMATCH = 1,
  WB_PACKET_ERR_DECODE_FAILED,
};

// CRC function prototype, to allow for hardware CRC calculation
typedef uint16_t (*wavebird_packet_crc_fn_t)(const uint8_t *data, size_t length);

/**
 * Get the CRC value from a WaveBird packet.
 *
 * @param packet the 19-byte packet from the radio
 *
 * @return the CRC value
 */
static inline uint16_t wavebird_packet_get_crc(const uint8_t *packet)
{
  return (packet[15] & 0x0F) << 12 | packet[16] << 4 | (packet[17] & 0xF0) >> 4;
}

/**
 * Set the CRC value in a WaveBird packet.
 *
 * @param packet the 19-byte packet
 * @param crc the CRC value to set
 */
static inline void wavebird_packet_set_crc(uint8_t *packet, uint16_t crc)
{
  packet[15] &= 0xF0;
  packet[15] |= (crc >> 12) & 0x0F;
  packet[16] = (crc >> 4) & 0xFF;
  packet[17] &= 0x0F;
  packet[17] |= (crc << 4) & 0xF0;
}

/**
 * Get the footer from a WaveBird packet.
 *
 * @param packet the 19-byte packet from the radio
 * @return the footer value
 */
static inline uint16_t wavebird_packet_get_footer(const uint8_t *packet)
{
  return (packet[17] & 0x0F) << 8 | packet[18];
}

/**
 * Set the footer in a WaveBird packet.
 *
 * @param packet the 19-byte packet
 * @param footer the footer value to set
 */
static inline void wavebird_packet_set_footer(uint8_t *packet, uint16_t footer)
{
  packet[17] &= 0xF0;
  packet[17] |= (footer >> 8) & 0x0F;
  packet[18] = footer & 0xFF;
}

/**
 * Set the CRC function to use for packet encoding and decoding, to allow for
 * hardware CRC calculation when available.
 *
 * Functions must provide a CRC-CCITT implementation, with polynomial 0x1021,
 * and initial value 0x0000.
 *
 * @param crc_fn function to calculate the CRC
 */
void wavebird_packet_set_crc_fn(wavebird_packet_crc_fn_t crc_fn);

/**
 * Deinterleave the payload from a WaveBird packet into 4 BCH(31,21) codewords.
 *
 * @param codewords array to store the 4, 31-bit codewords
 * @param packet the 19-byte packet from the radio
 */
void wavebird_packet_deinterleave(uint32_t *codewords, const uint8_t *packet);

/**
 * Interleave 4 BCH(31,21) codewords, and add them to a WaveBird packet.
 *
 * @param packet byte array to store the 19-byte packet
 * @param codewords the 4, 31-bit codewords
 */
void wavebird_packet_interleave(uint8_t *packet, const uint32_t *codewords);

/**
 * Decode a WaveBird packet into an 84-bit message.
 *
 * @param message byte array to store the decoded message
 * @param packet the 19-byte packet from the radio
 * @param crc_fn function to calculate the CRC
 *
 * @return negative error code on failure, or the packet type on success
 * @retval WB_PACKET_INPUT_STATE if the packet is a valid "input state" packet
 * @retval WB_PACKET_ORIGIN if the packet is a valid "origin" packet
 * @retval -WB_PACKET_ERR_CRC_MISMATCH if CRC check failed
 * @retval -WB_PACKET_ERR_DECODE_FAILED if BCH decoding failed
 */
int wavebird_packet_decode(uint8_t *message, const uint8_t *packet);

/**
 * Encode a 84-bit message into a WaveBird packet.
 *
 * @param packet byte array to store the 19-byte packet
 * @param message the 84-bit message to encode
 */
void wavebird_packet_encode(uint8_t *packet, const uint8_t *message);