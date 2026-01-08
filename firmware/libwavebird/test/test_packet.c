#include <string.h>

#include "unity.h"

#include "wavebird/message.h"
#include "wavebird/packet.h"

#include "fixtures.h"

void setUp(void)
{
}

void tearDown(void)
{
}

static void test_deinterleave()
{
  uint32_t codewords[4] = {0};
  wavebird_packet_deinterleave(codewords, packet_input_state_resting);

  TEST_ASSERT_EQUAL_HEX32(codewords_input_state_resting[0], codewords[0]);
  TEST_ASSERT_EQUAL_HEX32(codewords_input_state_resting[1], codewords[1]);
  TEST_ASSERT_EQUAL_HEX32(codewords_input_state_resting[2], codewords[2]);
  TEST_ASSERT_EQUAL_HEX32(codewords_input_state_resting[3], codewords[3]);
}

static void test_interleave()
{
  uint8_t packet[WAVEBIRD_PACKET_BYTES];
  wavebird_packet_interleave(packet, codewords_input_state_resting);

  // Check 15.5 bytes are equal
  TEST_ASSERT_EQUAL_HEX8_ARRAY(packet_input_state_resting, packet, 15);
  TEST_ASSERT_EQUAL_HEX8(packet_input_state_resting[15] & 0xf0, packet[15] & 0xf0);
}

static void test_deinterleave_interleave()
{
  // Deinterleave a packet
  uint32_t codewords[4];
  wavebird_packet_deinterleave(codewords, packet_input_state_resting);

  // Interleave the resulting codewords
  uint8_t packet[WAVEBIRD_PACKET_BYTES];
  wavebird_packet_interleave(packet, codewords);

  // Check all 15.5 bytes are equal
  TEST_ASSERT_EQUAL_HEX8_ARRAY(packet_input_state_resting, packet, 15);
  TEST_ASSERT_EQUAL_HEX8(packet_input_state_resting[15] & 0xf0, packet[15] & 0xf0);
}

static void test_encode_input_state()
{
  // Encode the input state message
  uint8_t packet[WAVEBIRD_PACKET_BYTES];
  wavebird_packet_encode(packet, message_input_state_resting);

  // Check the crc is correct
  uint16_t expected_crc = wavebird_packet_get_crc(packet_input_state_resting);
  uint16_t actual_crc   = wavebird_packet_get_crc(packet);
  TEST_ASSERT_EQUAL_HEX16(expected_crc, actual_crc);

  // Check all 15.5 bytes of data are correct
  TEST_ASSERT_EQUAL_HEX8_ARRAY(packet_input_state_resting, packet, 15);
  TEST_ASSERT_EQUAL_HEX8(packet_input_state_resting[15] & 0xf0, packet[15] & 0xf0);
}

static void test_encode_origin()
{
  // Encode the origin message
  uint8_t packet[WAVEBIRD_PACKET_BYTES];
  wavebird_packet_encode(packet, message_origin);

  // Check the crc is correct
  uint16_t expected_crc = wavebird_packet_get_crc(packet_origin);
  uint16_t actual_crc   = wavebird_packet_get_crc(packet);
  TEST_ASSERT_EQUAL_HEX16(expected_crc, actual_crc);

  // Check all 15.5 bytes of data are correct
  TEST_ASSERT_EQUAL_HEX8_ARRAY(packet_origin, packet, 15);
  TEST_ASSERT_EQUAL_HEX8(packet_origin[15] & 0xf0, packet[15] & 0xf0);
}

static void test_decode_input_state()
{
  uint8_t message[WAVEBIRD_MESSAGE_BYTES];
  int rcode = wavebird_packet_decode(message, packet_input_state_resting);

  // Check decoding was successful
  TEST_ASSERT_EQUAL(0, rcode);

  // Check the message type
  TEST_ASSERT_EQUAL(WB_MESSAGE_TYPE_INPUT_STATE, wavebird_message_get_type(message));

  // Check the controller ID
  TEST_ASSERT_EQUAL_HEX16(0x2B1, wavebird_message_get_controller_id(message));

  // Check the buttons
  uint16_t buttons = wavebird_input_state_get_buttons(message);
  TEST_ASSERT_EQUAL_HEX16(0x0000, buttons);

  // Check the analog inputs
  TEST_ASSERT_EQUAL_HEX8(0x88, wavebird_input_state_get_stick_x(message));
  TEST_ASSERT_EQUAL_HEX8(0x7F, wavebird_input_state_get_stick_y(message));
  TEST_ASSERT_EQUAL_HEX8(0x88, wavebird_input_state_get_substick_x(message));
  TEST_ASSERT_EQUAL_HEX8(0x82, wavebird_input_state_get_substick_y(message));
  TEST_ASSERT_EQUAL_HEX8(0x1A, wavebird_input_state_get_trigger_left(message));
  TEST_ASSERT_EQUAL_HEX8(0x14, wavebird_input_state_get_trigger_right(message));
}

static void test_decode_origin()
{
  uint8_t message[WAVEBIRD_MESSAGE_BYTES];
  int rcode = wavebird_packet_decode(message, packet_origin);

  // Check decoding was successful
  TEST_ASSERT_EQUAL(0, rcode);

  // Check the message type
  TEST_ASSERT_EQUAL(WB_MESSAGE_TYPE_ORIGIN, wavebird_message_get_type(message));

  // Check the controller ID
  TEST_ASSERT_EQUAL_HEX16(0x2B1, wavebird_message_get_controller_id(message));

  // Check the origin values
  TEST_ASSERT_EQUAL_HEX8(0x86, wavebird_origin_get_stick_x(message));
  TEST_ASSERT_EQUAL_HEX8(0x7F, wavebird_origin_get_stick_y(message));
  TEST_ASSERT_EQUAL_HEX8(0x8B, wavebird_origin_get_substick_x(message));
  TEST_ASSERT_EQUAL_HEX8(0x83, wavebird_origin_get_substick_y(message));
  TEST_ASSERT_EQUAL_HEX8(0x1B, wavebird_origin_get_trigger_left(message));
  TEST_ASSERT_EQUAL_HEX8(0x13, wavebird_origin_get_trigger_right(message));
}

static void test_decode_single_error()
{
  uint8_t packet[WAVEBIRD_PACKET_BYTES];
  uint8_t message[WAVEBIRD_MESSAGE_BYTES];

  // Corrupt a single bit in the payload portion of the packet
  for (int i = 0; i < 124; i++) {
    // Start with a clean packet
    memcpy(packet, packet_input_state_resting, WAVEBIRD_PACKET_BYTES);

    // Corrupt a single bit in the packet
    packet[i / 8] ^= (1 << (7 - i % 8));

    // Attempt to decode the corrupted packet, check decoding was successful
    int rcode = wavebird_packet_decode(message, packet);
    TEST_ASSERT_EQUAL(0, rcode);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(message_input_state_resting, message, WAVEBIRD_MESSAGE_BYTES);
  }
}

static void test_decode_burst_error()
{
  uint8_t packet[WAVEBIRD_PACKET_BYTES];
  uint8_t message[WAVEBIRD_MESSAGE_BYTES];

  // Sweep through the payload portion of the packet a nibble at a time
  for (int i = 0; i < 30; i++) {
    // Start with a clean packet
    memcpy(packet, packet_input_state_resting, WAVEBIRD_PACKET_BYTES);

    // Corrupt a byte in the packet
    if (i % 2 == 0) {
      packet[i / 2] ^= 0xFF;
    } else {
      packet[i / 2] ^= 0x0F;
      packet[i / 2 + 1] ^= 0xF0;
    }

    // Attempt to decode the corrupted packet, check decoding was successful
    int rcode = wavebird_packet_decode(message, packet);
    TEST_ASSERT_EQUAL(0, rcode);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(message_input_state_resting, message, WAVEBIRD_MESSAGE_BYTES);
  }
}

static void test_decode_failure()
{
  uint8_t packet[WAVEBIRD_PACKET_BYTES];
  uint8_t message[WAVEBIRD_MESSAGE_BYTES];

  // Sweep through the payload portion of the packet a byte at a time
  for (int i = 0; i < 15; i++) {
    // Start with a clean packet
    memcpy(packet, packet_input_state_resting, WAVEBIRD_PACKET_BYTES);

    // Corrupt two bytes in the payload portion of the packet
    packet[i] ^= 0xFF;
    packet[i + 1] ^= 0xFF;

    // Attempt to decode the corrupted packet, check decoding failed
    int rcode = wavebird_packet_decode(message, packet);
    TEST_ASSERT_NOT_EQUAL(0, rcode);
  }
}

static void test_decode_crc_mismatch()
{
  uint8_t packet[WAVEBIRD_PACKET_BYTES];
  uint8_t message[WAVEBIRD_MESSAGE_BYTES];

  // Start with a clean packet
  memcpy(packet, packet_input_state_resting, WAVEBIRD_PACKET_BYTES);

  // Corrupt a bit in the CRC portion of the packet
  packet[15] ^= 0x01;

  // Attempt to decode the corrupted packet, check decoding failed
  int rcode = wavebird_packet_decode(message, packet);
  TEST_ASSERT_EQUAL(-WB_PACKET_ERR_CRC_MISMATCH, rcode);
}

static void test_encode_decode()
{
  uint8_t packet[WAVEBIRD_PACKET_BYTES]   = {0};
  uint8_t message[WAVEBIRD_MESSAGE_BYTES] = {0};

  // Encode a message into a packet
  wavebird_packet_encode(packet, message_input_state_resting);

  // Decode the packet back into a message
  int rc = wavebird_packet_decode(message, packet);

  // Check decoding was successful
  TEST_ASSERT_EQUAL(0, rc);

  // Check the decoded message is identical to the original
  TEST_ASSERT_EQUAL_HEX8_ARRAY(message_input_state_resting, message, WAVEBIRD_MESSAGE_BYTES);
}

int main(int argc, char **argv)
{
  UNITY_BEGIN();

  RUN_TEST(test_deinterleave);
  RUN_TEST(test_interleave);
  RUN_TEST(test_deinterleave_interleave);
  RUN_TEST(test_encode_input_state);
  RUN_TEST(test_encode_origin);
  RUN_TEST(test_decode_input_state);
  RUN_TEST(test_decode_origin);
  RUN_TEST(test_decode_single_error);
  RUN_TEST(test_decode_burst_error);
  RUN_TEST(test_decode_failure);
  RUN_TEST(test_decode_crc_mismatch);
  RUN_TEST(test_encode_decode);

  return UNITY_END();
}
