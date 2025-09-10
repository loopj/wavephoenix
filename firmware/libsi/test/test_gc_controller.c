#include <stdio.h>
#include <string.h>

#include "unity.h"

#include "si/commands.h"
#include "si/device/gc_controller.h"
#include "si/si.h"

// Mock SI implementation
static uint8_t response_buf[SI_BLOCK_SIZE] = {0};
static uint8_t response_len                = 0;

void si_write_bytes(const uint8_t *data, uint8_t length, si_callback_fn callback)
{
  memcpy(response_buf, data, length);
  response_len = length;
}

void si_read_command(uint8_t *data, si_callback_fn callback)
{
}

// Simulate receiving a command
static int simulate_command(struct si_device_gc_controller *device, uint8_t *command)
{
  si_command_handler_fn handler = si_command_get_handler(command[0]);
  return handler(command, NULL, device);
}

// Test that the device info response is correct for a standard GameCube controller
static void test_gcc_info()
{
  // Initialize as a standard GameCube controller
  struct si_device_gc_controller device;
  si_device_gc_init(&device, SI_TYPE_GC | SI_GC_STANDARD);

  // Send an info command
  uint8_t info_command[] = {SI_CMD_INFO};
  simulate_command(&device, info_command);

  // Test device info response is as expected
  uint8_t expected_response[] = {0x09, 0x00, 0x00};
  TEST_ASSERT_EQUAL(3, response_len);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_response, response_buf, 3);
}

// Test that the "need origin" flag is cleared after a "read origin" command
static void test_gcc_info_after_read_origin()
{
  // Initialize as a standard GameCube controller
  struct si_device_gc_controller device;
  si_device_gc_init(&device, SI_TYPE_GC | SI_GC_STANDARD);

  // Set an origin
  struct si_device_gc_input_state new_origin = {
      .stick_x       = 0x81,
      .stick_y       = 0x82,
      .substick_x    = 0x83,
      .substick_y    = 0x84,
      .trigger_left  = 0x11,
      .trigger_right = 0x12,
  };
  si_device_gc_set_origin(&device, &new_origin);

  // Send an info command
  uint8_t info_command[] = {SI_CMD_INFO};
  simulate_command(&device, info_command);

  // Test device info response is as expected
  uint8_t expected_response[] = {0x09, 0x00, 0x20};
  TEST_ASSERT_EQUAL(3, response_len);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_response, response_buf, 3);

  // Send a read origin command
  uint8_t read_origin_command[] = {SI_CMD_GC_READ_ORIGIN};
  simulate_command(&device, read_origin_command);

  // Send another info command
  simulate_command(&device, info_command);

  // Verify the "need_origin" flag is not present
  uint8_t expected_response_2[] = {0x09, 0x00, 0x00};
  TEST_ASSERT_EQUAL(3, response_len);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_response_2, response_buf, 3);
}

// Test that "analog mode" and "motor state" are saved after a "poll" command
static void test_gcc_info_after_poll()
{
  // Initialize as a standard GameCube controller
  struct si_device_gc_controller device;
  si_device_gc_init(&device, SI_TYPE_GC | SI_GC_STANDARD);

  // Send a read origin command
  uint8_t read_origin_command[] = {SI_CMD_GC_READ_ORIGIN};
  simulate_command(&device, read_origin_command);

  // Send a poll command, analog_mode = 3, motor_state = 1
  uint8_t poll_command[] = {SI_CMD_GC_SHORT_POLL, 3, 1};
  simulate_command(&device, poll_command);

  // Send an info command
  uint8_t info_command[] = {SI_CMD_INFO};
  simulate_command(&device, info_command);

  // Verify the "analog_mode" and "motor_state" are present
  uint8_t expected_response[] = {0x09, 0x00, 0x0B};
  TEST_ASSERT_EQUAL(3, response_len);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_response, response_buf, 3);
}

// Test that the device info response is correct for a WaveBird receiver
static void test_wavebird_info()
{
  // Initialize as a WaveBird receiver
  struct si_device_gc_controller device;
  si_device_gc_init(&device, SI_TYPE_GC | SI_GC_WIRELESS | SI_GC_NOMOTOR);

  // Send an info command
  uint8_t info_command[] = {SI_CMD_INFO};
  simulate_command(&device, info_command);

  // Test device info response is as expected
  uint8_t expected_response[] = {0xA8, 0x00, 0x00};
  TEST_ASSERT_EQUAL(3, response_len);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_response, response_buf, 3);
}

// Test that the device info response is correct when setting the wireless ID
static void test_wavebird_info_after_set_wireless_id()
{
  // Initialize as a WaveBird receiver
  struct si_device_gc_controller device;
  si_device_gc_init(&device, SI_TYPE_GC | SI_GC_WIRELESS | SI_GC_NOMOTOR);

  // Set a 10-bit wireless ID
  si_device_gc_set_wireless_id(&device, 0x2B1);
  TEST_ASSERT_EQUAL(0x2B1, si_device_gc_get_wireless_id(&device));

  // Send an info command
  uint8_t info_command[] = {SI_CMD_INFO};
  simulate_command(&device, info_command);

  // Test device info response includes the controller ID
  uint8_t expected_response[] = {0xE9, 0x80, 0xB1};
  TEST_ASSERT_EQUAL(3, response_len);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_response, response_buf, 3);
}

// Test that the device info response is correct when setting the wireless ID multiple times
static void test_wavebird_info_after_set_wireless_id_multiple()
{
  // Initialize as a WaveBird receiver
  struct si_device_gc_controller device;
  si_device_gc_init(&device, SI_TYPE_GC | SI_GC_WIRELESS | SI_GC_NOMOTOR);

  // Set a 10-bit wireless ID
  si_device_gc_set_wireless_id(&device, 0x2B1);
  TEST_ASSERT_EQUAL(0x2B1, si_device_gc_get_wireless_id(&device));

  // Set a different 10-bit wireless ID
  si_device_gc_set_wireless_id(&device, 0x32F);
  TEST_ASSERT_EQUAL(0x32F, si_device_gc_get_wireless_id(&device));

  // Send an info command
  uint8_t info_command[] = {SI_CMD_INFO};
  simulate_command(&device, info_command);

  // Test device info response includes the most recent controller ID
  uint8_t expected_response[] = {0xE9, 0xC0, 0x2F};
  TEST_ASSERT_EQUAL(3, response_len);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_response, response_buf, 3);
}

// Test that the device info response is correct when fixing the wireless ID
static void test_wavebird_info_after_fix_device()
{
  uint8_t info_command[] = {SI_CMD_INFO};

  // Initialize as a WaveBird receiver
  struct si_device_gc_controller device;
  si_device_gc_init(&device, SI_TYPE_GC | SI_GC_WIRELESS | SI_GC_NOMOTOR);

  // Send an info command
  simulate_command(&device, info_command);

  // Test device info response is as expected
  uint8_t expected_info_response[] = {0xA8, 0x00, 0x00};
  TEST_ASSERT_EQUAL(3, response_len);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_info_response, response_buf, 3);

  // Set the wireless ID (e.g. after packet reception)
  si_device_gc_set_wireless_id(&device, 0x2B1);

  // Send an info command
  simulate_command(&device, info_command);

  // Test device info response is as expected
  uint8_t expected_info_response_2[] = {0xE9, 0x80, 0xB1};
  TEST_ASSERT_EQUAL(3, response_len);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_info_response_2, response_buf, 3);

  // Send a fix device command
  uint8_t fix_device_command[] = {SI_CMD_GC_FIX_DEVICE, 0x90, 0xB1};
  simulate_command(&device, fix_device_command);

  // Check fix device response is as expected
  uint8_t expected_fix_response[] = {0xEB, 0x90, 0xB1};
  TEST_ASSERT_EQUAL(3, response_len);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_fix_response, response_buf, 3);

  // Send an info command
  simulate_command(&device, info_command);

  // Test device info response includes the fixed controller ID
  uint8_t expected_response[] = {0xEB, 0x90, 0xB1};
  TEST_ASSERT_EQUAL(3, response_len);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_response, response_buf, 3);
}

// Test that the device info is correct when the console fixes the wireless ID,
// but we have not yet received a packet from the controller
static void test_wavebird_fix_device_without_wireless_id()
{
  uint8_t info_command[] = {SI_CMD_INFO};

  // Initialize as a WaveBird receiver
  struct si_device_gc_controller device;
  si_device_gc_init(&device, SI_TYPE_GC | SI_GC_WIRELESS | SI_GC_NOMOTOR);

  // Send an info command
  simulate_command(&device, info_command);

  // Test device info response is as expected
  uint8_t expected_info_response[] = {0xA8, 0x00, 0x00};
  TEST_ASSERT_EQUAL(3, response_len);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_info_response, response_buf, 3);

  // Send a fix device command
  uint8_t fix_device_command[] = {SI_CMD_GC_FIX_DEVICE, 0x90, 0xB1};
  simulate_command(&device, fix_device_command);

  // Check fix device response is as expected
  uint8_t expected_fix_response[] = {0xAB, 0x90, 0xB1};
  TEST_ASSERT_EQUAL(3, response_len);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_fix_response, response_buf, 3);

  // Send an info command
  simulate_command(&device, info_command);

  // Test device info response includes the fixed controller ID
  uint8_t expected_info_response_2[] = {0xAB, 0x90, 0xB1};
  TEST_ASSERT_EQUAL(3, response_len);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_info_response_2, response_buf, 3);
}

// Test that setting wireless ID fails when the controller ID has already been fixed
static void test_set_wireless_id_when_fixed(void)
{
  // Initialize as a WaveBird receiver
  struct si_device_gc_controller device;
  si_device_gc_init(&device, SI_TYPE_GC | SI_GC_WIRELESS | SI_GC_NOMOTOR);
  si_device_gc_set_wireless_id(&device, 0x2B1);

  // Send a fix device command
  uint8_t fix_device_command[] = {SI_CMD_GC_FIX_DEVICE, 0x90, 0xB1};
  simulate_command(&device, fix_device_command);

  // Try to set a different 10-bit wireless ID
  si_device_gc_set_wireless_id(&device, 0x123);
  TEST_ASSERT_EQUAL_HEX16(0x2B1, si_device_gc_get_wireless_id(&device));
}

static void test_set_wireless_origin(void)
{
  uint8_t info_command[] = {SI_CMD_INFO};

  // Initialize as a WaveBird receiver
  struct si_device_gc_controller device;
  si_device_gc_init(&device, SI_TYPE_GC | SI_GC_WIRELESS | SI_GC_NOMOTOR);
  si_device_gc_set_wireless_id(&device, 0x2B1);

  // Send an info command
  simulate_command(&device, info_command);

  // Test device info response is as expected
  uint8_t expected_info_response[] = {0xE9, 0x80, 0xB1};
  TEST_ASSERT_EQUAL(3, response_len);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_info_response, response_buf, 3);

  // Send a fix device command
  uint8_t fix_device_command[] = {SI_CMD_GC_FIX_DEVICE, 0x90, 0xB1};
  simulate_command(&device, fix_device_command);

  // Send an info command
  simulate_command(&device, info_command);

  // Test device info is as expected
  uint8_t expected_info_response_2[] = {0xEB, 0x90, 0xB1};
  TEST_ASSERT_EQUAL(3, response_len);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_info_response_2, response_buf, 3);

  // Set the wireless origin
  struct si_device_gc_input_state origin = {
      .stick_x       = 0x85,
      .stick_y       = 0x86,
      .substick_x    = 0x87,
      .substick_y    = 0x88,
      .trigger_left  = 0x11,
      .trigger_right = 0x12,
  };
  si_device_gc_set_origin(&device, &origin);

  // Check the origin state is set correctly
  TEST_ASSERT_EQUAL_UINT8(0x85, device.origin.stick_x);
  TEST_ASSERT_EQUAL_UINT8(0x86, device.origin.stick_y);
  TEST_ASSERT_EQUAL_UINT8(0x87, device.origin.substick_x);
  TEST_ASSERT_EQUAL_UINT8(0x88, device.origin.substick_y);
  TEST_ASSERT_EQUAL_UINT8(0x11, device.origin.trigger_left);
  TEST_ASSERT_EQUAL_UINT8(0x12, device.origin.trigger_right);

  // Send an info command
  simulate_command(&device, info_command);

  // Test device info response includes the origin flag
  uint8_t expected_info_response_3[] = {0xEB, 0xB0, 0xB1};
  TEST_ASSERT_EQUAL(3, response_len);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_info_response_3, response_buf, 3);
}

// Test that the probe device response is correct, and ignored after setting the wireless ID
static void test_wavebird_probe_response()
{
  int rc;

  // Initialize as a WaveBird receiver
  struct si_device_gc_controller device;
  si_device_gc_init(&device, SI_TYPE_GC | SI_GC_WIRELESS | SI_GC_NOMOTOR);

  // Send a probe command
  uint8_t probe_command[] = {SI_CMD_GC_PROBE_DEVICE, 0x00, 0x00};
  rc                      = simulate_command(&device, probe_command);

  // Test probe response is as expected
  uint8_t expected_probe_response[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  TEST_ASSERT_EQUAL(8, rc);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_probe_response, response_buf, 8);

  // Set a 10-bit wireless ID
  si_device_gc_set_wireless_id(&device, 0x2B1);
  TEST_ASSERT_EQUAL(0x2B1, si_device_gc_get_wireless_id(&device));

  // Send another probe command
  rc = simulate_command(&device, probe_command);

  // Test probe response is ignored after setting wireless ID
  TEST_ASSERT_EQUAL(0, rc);
}

void test_gc_controller(void)
{
  Unity.TestFile = __FILE_NAME__;

  RUN_TEST(test_gcc_info);
  RUN_TEST(test_gcc_info_after_read_origin);
  RUN_TEST(test_gcc_info_after_poll);
  RUN_TEST(test_wavebird_info);
  RUN_TEST(test_wavebird_info_after_set_wireless_id);
  RUN_TEST(test_wavebird_info_after_set_wireless_id_multiple);
  RUN_TEST(test_wavebird_info_after_fix_device);
  RUN_TEST(test_set_wireless_id_when_fixed);
  RUN_TEST(test_wavebird_fix_device_without_wireless_id);
  RUN_TEST(test_set_wireless_origin);
  RUN_TEST(test_wavebird_probe_response);
}