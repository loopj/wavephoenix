#include "si/commands.h"

enum {
  COMMAND_STATE_IDLE,
  COMMAND_STATE_RX,
  COMMAND_STATE_TX,
  COMMAND_STATE_ERROR,
};

struct command_entry {
  uint8_t length;
  si_command_handler_fn handler;
  void *context;
};

static uint8_t command_state                   = COMMAND_STATE_IDLE;
static struct command_entry command_table[256] = {0};
static uint8_t command_buffer[SI_BLOCK_SIZE];
static bool auto_tx_rx_transition = false;

// Command handler TX completion callback
static void on_tx_complete(int result)
{
  if (result == 0) {
    command_state = COMMAND_STATE_IDLE;

    // Check if we need to transition back to RX mode
    if (auto_tx_rx_transition)
      si_command_process();
  } else {
    command_state = COMMAND_STATE_ERROR;
  }
}

// Command handler RX completion callback
static void on_rx_complete(int result)
{
  if (result == 0) {
    // Look up the command in the table
    struct command_entry *command = &command_table[command_buffer[0]];
    if (command->handler) {
      // Call the command handler
      command_state = COMMAND_STATE_TX;
      command->handler(command_buffer, on_tx_complete, command->context);
      return;
    }
  }

  // Error during command read or handler not found
  command_state = COMMAND_STATE_ERROR;
}

void si_command_register(uint8_t command, uint8_t length, si_command_handler_fn handler, void *context)
{
  command_table[command].length  = length;
  command_table[command].handler = handler;
  command_table[command].context = context;
}

uint8_t si_command_get_length(uint8_t command)
{
  return command_table[command].length;
}

si_command_handler_fn si_command_get_handler(uint8_t command)
{
  return command_table[command].handler;
}

void si_command_process()
{
  if (command_state == COMMAND_STATE_ERROR) {
    si_await_bus_idle();
    command_state = COMMAND_STATE_IDLE;
  }

  if (command_state == COMMAND_STATE_IDLE) {
    command_state = COMMAND_STATE_RX;
    si_read_command(command_buffer, on_rx_complete);
  }
}

void si_command_processing_enable()
{
  auto_tx_rx_transition = true;
  si_command_process();
}

void si_command_processing_disable()
{
  auto_tx_rx_transition = false;
}