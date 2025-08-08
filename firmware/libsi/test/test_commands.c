#include "unity.h"

#include "si/device/commands.h"
#include "si/si.h"

// Stub functions
void si_await_bus_idle(void)
{
}

static int handle_info(const uint8_t *command, si_complete_cb_t callback, void *context)
{
  return 3;
}

static int handle_reset(const uint8_t *command, si_complete_cb_t callback, void *context)
{
  return 3;
}

static void test_register_command()
{
  struct si_command *cmd = NULL;

  si_command_register(0x00, 1, handle_info, NULL);
  cmd = si_command_find_by_id(0x00);
  TEST_ASSERT_NOT_NULL(cmd);
  TEST_ASSERT_EQUAL(1, cmd->length);
  TEST_ASSERT_EQUAL(handle_info, cmd->handler);

  si_command_register(0xFF, 3, handle_reset, NULL);
  cmd = si_command_find_by_id(0xFF);
  TEST_ASSERT_NOT_NULL(cmd);
  TEST_ASSERT_EQUAL(3, cmd->length);
  TEST_ASSERT_EQUAL(handle_reset, cmd->handler);
}

static void test_register_command_missing()
{
  struct si_command *cmd = si_command_find_by_id(0x69);
  TEST_ASSERT_NULL(cmd);
}

void test_commands(void)
{
  Unity.TestFile = __FILE_NAME__;

  RUN_TEST(test_register_command);
  RUN_TEST(test_register_command_missing);
}