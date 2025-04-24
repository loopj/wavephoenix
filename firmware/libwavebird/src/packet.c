#include <stddef.h>

#include "wavebird/bch3121.h"
#include "wavebird/packet.h"

#define PACKET_DATA_BITS  124
#define PACKET_DATA_START 28
#define CODEWORD_COUNT    4
#define CRC_FINAL_XOR     0xCE98

// Internal state to avoid repeated allocations
static uint8_t crc_state[WAVEBIRD_MESSAGE_BYTES];
static uint32_t codewords[CODEWORD_COUNT];

// CRC function pointer to allow for hardware CRC calculation
static uint16_t crc_ccitt(const uint8_t *data, size_t length);
static wavebird_packet_crc_fn_t crc_fn = crc_ccitt;

// A "good enough" CRC-CCITT implementation for systems without hardware CRC
static uint16_t crc_ccitt(const uint8_t *data, size_t length)
{
  uint16_t crc = 0x0000;

  while (length--) {
    crc ^= (uint16_t)(*data++) << 8;

    // Process each bit in the byte
    for (int i = 0; i < 8; i++) {
      crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);
    }
  }

  return crc;
}

// Set the Nth bit in a big-endian byte array
static inline void set_bit(uint8_t *data, uint8_t length, uint8_t bit, uint8_t value)
{
  uint8_t byte_index = length - 1 - bit / 8;
  uint8_t bit_mask   = 1 << (bit % 8);

  if (value) {
    data[byte_index] |= bit_mask;
  } else {
    data[byte_index] &= ~bit_mask;
  }
}

// Get the Nth bit from a big-endian byte array
static inline uint8_t get_bit(const uint8_t *data, uint8_t length, uint8_t bit)
{
  uint8_t byte_index = length - 1 - bit / 8;
  uint8_t bit_mask   = 1 << (bit % 8);

  return (data[byte_index] & bit_mask) ? 1 : 0;
}

void wavebird_packet_set_crc_fn(wavebird_packet_crc_fn_t _crc_fn)
{
  crc_fn = _crc_fn;
}

void wavebird_packet_deinterleave(uint32_t *codewords, const uint8_t *packet)
{
  // Step through each bit of the interleaved packet data
  for (uint8_t i = 0; i < PACKET_DATA_BITS; i++) {
    // Extract the bit from the packet
    bool bit = get_bit(packet, WAVEBIRD_PACKET_BYTES, i + PACKET_DATA_START);

    // Set the bit in the appropriate codeword
    if (bit) {
      codewords[i % CODEWORD_COUNT] |= 1 << (i / CODEWORD_COUNT);
    } else {
      codewords[i % CODEWORD_COUNT] &= ~(1 << (i / CODEWORD_COUNT));
    }
  }
}

void wavebird_packet_interleave(uint8_t *packet, const uint32_t *codewords)
{
  // Step through each bit of each codeword
  for (uint8_t i = 0; i < PACKET_DATA_BITS; i++) {
    // Extract the bit from the codeword
    bool bit = (codewords[i % CODEWORD_COUNT] >> (i / 4)) & 1;

    // Set the bit in the packet
    set_bit(packet, WAVEBIRD_PACKET_BYTES, i + PACKET_DATA_START, bit);
  }
}

int wavebird_packet_decode(uint8_t *message, const uint8_t *packet)
{
  // Deinterleave the input data into 4, 31-bit codewords
  wavebird_packet_deinterleave(codewords, packet);

  // Explicitly zero the first 4 bits of the message. We don't actually
  // care about these, but we want the complete message to be consistent
  // every time is is generated.
  message[0] = 0x00;

  // Decode each codeword, and pack them into the message
  for (int i = 0; i < CODEWORD_COUNT; i++) {
    // Attempt to decode the codeword
    uint32_t codeword;
    if (bch3121_decode_and_correct(&codeword, codewords[i]) < 0)
      return -WB_PACKET_ERR_DECODE_FAILED;

    // Pack the decoded codeword into the message
    for (int j = 0; j < BCH3121_MESSAGE_LEN; j++) {
      // Extract bit from codeword
      uint8_t bit = codeword & 1;

      // Set the bit in message
      set_bit(message, WAVEBIRD_MESSAGE_BYTES, i * BCH3121_MESSAGE_LEN + j, bit);

      // Set the bit in the CRC state (transposed)
      set_bit(crc_state, WAVEBIRD_MESSAGE_BYTES, j * CODEWORD_COUNT + i, bit);

      // Shift the codeword right
      codeword >>= 1;
    }
  }

  // Extract the expected CRC from the packet, and calculate the actual CRC
  uint16_t expected_crc = wavebird_packet_get_crc(packet);
  uint16_t actual_crc   = crc_fn(crc_state, WAVEBIRD_MESSAGE_BYTES) ^ CRC_FINAL_XOR;

  // Return error code if CRCs do not match
  if (expected_crc != actual_crc)
    return -WB_PACKET_ERR_CRC_MISMATCH;

  return 0;
}

void wavebird_packet_encode(uint8_t *packet, const uint8_t *message)
{
  // Construct and encode the codewords
  for (int i = 0; i < CODEWORD_COUNT; i++) {
    for (int j = 0; j < BCH3121_MESSAGE_LEN; j++) {
      // Extract the bit from the message
      uint8_t bit = get_bit(message, WAVEBIRD_MESSAGE_BYTES, i * BCH3121_MESSAGE_LEN + j);

      // Set the bit in the codeword
      if (bit) {
        codewords[i] |= (1 << j);
      } else {
        codewords[i] &= ~(1 << j);
      }

      // Set the bit in the CRC state (transposed)
      set_bit(crc_state, WAVEBIRD_MESSAGE_BYTES, j * CODEWORD_COUNT + i, bit);
    }

    // Encode into a BCH(31,21) codeword
    codewords[i] = bch3121_encode(codewords[i]);
  }

  // Interleave the codewords
  wavebird_packet_interleave(packet, codewords);

  // Calculate and set the CRC
  uint16_t crc = crc_fn(crc_state, WAVEBIRD_MESSAGE_BYTES) ^ CRC_FINAL_XOR;
  wavebird_packet_set_crc(packet, crc);

  // Set the footer
  wavebird_packet_set_footer(packet, 0x000);
}