#include "unity.h"

#include "si/commands.h"
#include "si/si.h"

// Stub functions
void si_await_bus_idle(void)
{
}

static int handle_info(const uint8_t *command, si_callback_fn callback, void *context)
{
  return 3;
}

static int handle_reset(const uint8_t *command, si_callback_fn callback, void *context)
{
  return 3;
}

static void test_register_command()
{
  si_command_register(0x00, 1, handle_info, NULL);
  TEST_ASSERT_EQUAL(1, si_command_get_length(0x00));
  TEST_ASSERT_EQUAL(handle_info, si_command_get_handler(0x00));

  si_command_register(0xFF, 3, handle_reset, NULL);
  TEST_ASSERT_EQUAL(3, si_command_get_length(0xFF));
  TEST_ASSERT_EQUAL(handle_reset, si_command_get_handler(0xFF));
}

static void test_register_command_missing()
{
  TEST_ASSERT_EQUAL(0, si_command_get_length(0x69));
  TEST_ASSERT_NULL(si_command_get_handler(0x69));
}

void test_commands(void)
{
  Unity.TestFile = __FILE_NAME__;

  RUN_TEST(test_register_command);
  RUN_TEST(test_register_command_missing);
}