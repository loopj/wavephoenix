#include "si/device/commands.h"

#define COMMAND_TABLE_SIZE 18

enum {
  BUS_STATE_UNKNOWN = 0,
  BUS_STATE_IDLE,
  BUS_STATE_BUSY,
  BUS_STATE_ERROR,
};

// Command table for registered SI commands
static struct si_command command_table[COMMAND_TABLE_SIZE] = {0};
static struct si_command *current_command                  = NULL;

// Current bus state
static uint8_t bus_state = BUS_STATE_UNKNOWN;

// Buffer for reading/writing commands
static uint8_t command_buffer[SI_BLOCK_SIZE];

// Automatically transition back to reading commands
static bool auto_tx_rx_transition = false;

// Vaguely context-aware hash function
// We're hashing to reduce memory usage while keeping O(1) lookup time in most cases
static inline uint8_t hash_command(uint8_t command)
{
  // Reserve hash slots for universal commands
  if (command == 0x00)
    return 0;
  if (command == 0xFF)
    return 1;

  // Hash all other commands to remaining slots
  return (command % (COMMAND_TABLE_SIZE - 2)) + 2;
}

// Command handler TX completion callback
static void on_tx_complete(int result)
{
  // Update bus state based on result
  bus_state = (result < 0) ? BUS_STATE_ERROR : BUS_STATE_IDLE;

  // Transition back to RX mode if auto transition is enabled
  if (auto_tx_rx_transition)
    si_command_process();
}

// Command handler RX completion callback
static void on_rx_complete(int result)
{
  // If we successfully read a command and have a handler, call it
  if (result >= 0 && current_command && current_command->handler) {
    current_command->handler(command_buffer, on_tx_complete, current_command->user_data);
    return;
  }

  // Otherwise, there was either an error during the read, or handler not found
  bus_state = BUS_STATE_ERROR;

  // Transition back to RX mode if auto transition is enabled
  if (auto_tx_rx_transition)
    si_command_process();
}

static bool command_byte_cb(uint8_t byte, uint8_t byte_index)
{
  // If this is the first byte, check if there is a registered command
  if (byte_index == 0) {
    current_command = si_command_find_by_id(byte);
    if (current_command == NULL)
      return false;
  }

  // Stop reading when we reach the expected length
  if (byte_index == current_command->length - 1)
    return false;

  return true;
}

void si_command_register(uint8_t command, uint8_t length, si_command_handler_fn handler, void *user_data)
{
  uint8_t index = hash_command(command);

  // Linear probing for collision resolution
  // If the slot is already occupied, find the next available slot
  // This has high clustering, but is simple and effective for our use case
  while (command_table[index].handler != NULL && command_table[index].command != command) {
    index = (index + 1) % COMMAND_TABLE_SIZE;
  }

  // Store the command
  command_table[index] = (struct si_command){
      .command   = command,
      .length    = length,
      .handler   = handler,
      .user_data = user_data,
  };
}

struct si_command *si_command_find_by_id(uint8_t command)
{
  uint8_t index = hash_command(command);

  while (command_table[index].handler != NULL) {
    if (command_table[index].command == command) {
      return &command_table[index];
    }
    index = (index + 1) % COMMAND_TABLE_SIZE;
  }
  return NULL;
}

void si_command_process()
{
  if (bus_state != BUS_STATE_IDLE)
    si_await_bus_idle();

  // Initialize command reading context
  current_command = NULL;

  bus_state = BUS_STATE_BUSY;
  si_read_bytes(command_buffer, SI_BLOCK_SIZE, command_byte_cb, on_rx_complete);
}

void si_command_processing_enable()
{
  if (auto_tx_rx_transition)
    return;

  auto_tx_rx_transition = true;
  si_command_process();
}

void si_command_processing_disable()
{
  if (!auto_tx_rx_transition)
    return;

  auto_tx_rx_transition = false;
  bus_state             = BUS_STATE_UNKNOWN;
}