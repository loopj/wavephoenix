#include "si/commands.h"

#define COMMAND_TABLE_SIZE 18

enum {
  BUS_STATE_UNKNOWN = 0,
  BUS_STATE_IDLE,
  BUS_STATE_BUSY,
  BUS_STATE_ERROR,
};

struct command_entry {
  uint8_t command;
  uint8_t length;
  si_command_handler_fn handler;
  void *context;
};

static uint8_t bus_state                                      = BUS_STATE_UNKNOWN;
static struct command_entry command_table[COMMAND_TABLE_SIZE] = {0};
static uint8_t command_buffer[SI_BLOCK_SIZE];
static bool auto_tx_rx_transition = false;

// Vaguely context-aware hash function
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

// Find a command entry by command id
static struct command_entry *find_command(uint8_t command)
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
  if (result == 0) {
    // Look up the command in the table
    struct command_entry *command = find_command(command_buffer[0]);
    if (command && command->handler) {
      // Call the command handler
      bus_state = BUS_STATE_BUSY;
      command->handler(command_buffer, on_tx_complete, command->context);
      return;
    }
  }

  // Error during command read or handler not found
  bus_state = BUS_STATE_ERROR;

  // Transition back to RX mode if auto transition is enabled
  if (auto_tx_rx_transition)
    si_command_process();
}

void si_command_register(uint8_t command, uint8_t length, si_command_handler_fn handler, void *context)
{
  uint8_t index = hash_command(command);

  // Linear probing for collision resolution
  while (command_table[index].handler != NULL && command_table[index].command != command) {
    index = (index + 1) % COMMAND_TABLE_SIZE;
  }

  command_table[index].command = command;
  command_table[index].length  = length;
  command_table[index].handler = handler;
  command_table[index].context = context;
}

uint8_t si_command_get_length(uint8_t command)
{
  struct command_entry *entry = find_command(command);
  if (entry == NULL)
    return 0;

  return entry->length;
}

si_command_handler_fn si_command_get_handler(uint8_t command)
{
  struct command_entry *entry = find_command(command);
  if (entry == NULL)
    return NULL;

  return entry->handler;
}

void si_command_process()
{
  if (bus_state != BUS_STATE_IDLE)
    si_await_bus_idle();

  bus_state = BUS_STATE_BUSY;
  si_read_command(command_buffer, on_rx_complete);
}

void si_command_processing_enable()
{
  auto_tx_rx_transition = true;
  si_command_process();
}

void si_command_processing_disable()
{
  auto_tx_rx_transition = false;
  bus_state             = BUS_STATE_UNKNOWN;
}