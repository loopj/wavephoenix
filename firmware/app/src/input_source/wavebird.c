#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "nvm3.h"
#include "sl_sleeptimer.h"

#include <joybus/target/gc_controller.h>
#include <wavebird/wavebird.h>

#include "status_led.h"

#include "wavebird.h"

#define INPUT_VALID_MS  100
#define NVM3_KEY_BASE   200

// Settings keys
enum {
  SETTING_WAVEBIRD_CHANNEL,
  SETTING_CONTROLLER_TYPE,
  SETTING_PIN_WIRELESS_ID,
  SETTING_PAIRING_BUTTONS,
};

// Controller types
typedef enum {
  // Present as an OEM WaveBird receiver
  WP_CONT_TYPE_GC_WAVEBIRD,

  // Present as an OEM wired GameCube controller
  WP_CONT_TYPE_GC_WIRED,

  // Present as a wired GameCube controller without rumble
  WP_CONT_TYPE_GC_WIRED_NOMOTOR,
} wp_controller_type_t;

// Joybus WaveBird controller instance
struct joybus_gc_controller wavebird_controller;

// Settings storage, with defaults
static uint8_t chan                   = 0;
static wp_controller_type_t cont_type = WP_CONT_TYPE_GC_WAVEBIRD;
static bool pin_id                    = true;
static uint16_t pair_btns             = WB_BUTTONS_X | WB_BUTTONS_Y;

// Pairing state
static bool pairing_active = false;

// Stale input timer
static sl_sleeptimer_timer_handle_t input_valid_timer;

// Handle input validity timer expiry
static void handle_input_valid_timer_expiry(sl_sleeptimer_timer_handle_t *handle, void *data)
{
  // If we don't receive an input packet within a certain time, mark input as invalid
  // Internally this will cause all Joybus reads to return origin state
  joybus_gc_controller_input_valid(&wavebird_controller, false);
}

// Handle packets from the WaveBird radio
static void handle_wavebird_packet(const uint8_t *packet)
{
  // Decode the WaveBird packet
  uint8_t message[WAVEBIRD_MESSAGE_BYTES];
  int rc = wavebird_packet_decode(message, packet);
  if (rc < 0) {
    return;
  }

  // Handle wireless ID pinning, if enabled
  if (pin_id) {
    // Get the controller ID from the packet
    uint16_t wireless_id = wavebird_message_get_controller_id(message);

    // Check the controller id is as expected
    if (cont_type == WP_CONT_TYPE_GC_WAVEBIRD) {
      // Implement wireless ID pinning exactly as OEM WaveBird receivers do
      if (joybus_gc_controller_wireless_id_fixed(&wavebird_controller)) {
        // Drop packets from other controllers if the ID has been fixed
        if (joybus_gc_controller_get_wireless_id(&wavebird_controller) != wireless_id)
          return;
      } else {
        // Set the controller ID if it is not fixed
        joybus_gc_controller_set_wireless_id(&wavebird_controller, wireless_id);
      }
    } else {
      // Emulate wireless ID pinning for wired controllers
      static uint16_t first_seen_id = 0;
      if (first_seen_id == 0) {
        // Set the first seen ID
        first_seen_id = wireless_id;
      } else if (first_seen_id != wireless_id) {
        // Drop packets from other controllers
        return;
      }
    }
  }

  // Indicate packet reception via status LED
  status_led_set(STATUS_LED_WIRELESS_ACTIVITY);

  // Handle the packet
  if (wavebird_message_get_type(message) == WB_MESSAGE_TYPE_INPUT_STATE) {
    //
    // Handle input state packets
    //

    // Copy the buttons from the WaveBird message
    wavebird_controller.input.buttons &= ~JOYBUS_GCN_BUTTON_MASK;
    wavebird_controller.input.buttons |=
        ((message[3] & 0x80) >> 7) | ((message[2] & 0x0F) << 1) | ((message[3] & 0x7F) << 8);

    // Copy the stick, substick, and trigger values
    memcpy(&wavebird_controller.input.stick_x, &message[4], 6);

    // Set the input state as valid, and (re)set the validity timer
    joybus_gc_controller_input_valid(&wavebird_controller, true);
    sl_sleeptimer_restart_timer_ms(&input_valid_timer, INPUT_VALID_MS, handle_input_valid_timer_expiry, NULL, 0, 0);
  } else {
    //
    // Handle origin packets
    //

    // Copy the origin values from the packet
    struct joybus_gc_controller_input new_origin;
    wavebird_origin_copy((uint8_t *)&new_origin.stick_x, message);

    // Update the origin state in the Joybus device
    joybus_gc_controller_set_origin(&wavebird_controller, &new_origin);
  }
}

// Handle errors from the WaveBird radio
static void handle_wavebird_error(int error)
{
  // Update packet stats
}

// Qualify a WaveBird packet during pairing
static bool qualify_packet(const uint8_t *packet)
{
  // Decode the packet into an input state
  uint8_t message[WAVEBIRD_MESSAGE_BYTES];
  int rc = wavebird_packet_decode(message, packet);
  if (rc < 0 || wavebird_message_get_type(message) != WB_MESSAGE_TYPE_INPUT_STATE)
    return false;

  // Check for a specific key combination
  uint16_t buttons = wavebird_input_state_get_buttons(message);
  return (buttons & pair_btns) == pair_btns;
}

// Handle pairing start events
static void handle_pairing_started(void)
{
  printf("Pairing started\n");

  // Set the pairing active flag
  pairing_active = true;

  // Update the status LED
  status_led_set(STATUS_LED_PAIRING_ACTIVE);
}

// Handle pairing finish events
static void handle_pairing_finished(uint8_t status, uint8_t new_chan)
{
  // Set the pairing active flag
  pairing_active = false;

  // Save the new channel if pairing was successful
  if (status == WB_RADIO_PAIRING_SUCCESS) {
    printf("Pairing successful, new channel: %d\n", new_chan + 1);

    // Set the new channel and save to NVM
    chan = new_chan;
    nvm3_writeData(nvm3_defaultHandle, NVM3_KEY_BASE + SETTING_WAVEBIRD_CHANNEL, &chan, sizeof(chan));

    // Update the status LED
    status_led_set(STATUS_LED_PAIRING_SUCCESS);
  } else if (status == WB_RADIO_PAIRING_TIMEOUT) {
    printf("Pairing timed out\n");

    // Slow-blink the status LED to indicate pairing timeout
    status_led_set(STATUS_LED_PAIRING_TIMEOUT);
  } else {
    printf("Pairing cancelled\n");

    // Update the status LED
    status_led_set(STATUS_LED_OFF);
  }
}

void input_source_wavebird_init(void)
{
  // Initialize and configure the WaveBird radio
  wavebird_radio_configure_qualification(qualify_packet, 5);
  wavebird_radio_set_pairing_started_callback(handle_pairing_started);
  wavebird_radio_set_pairing_finished_callback(handle_pairing_finished);
  wavebird_radio_init(handle_wavebird_packet, handle_wavebird_error);

  // Load Wavebird settings from NVM
  nvm3_readData(nvm3_defaultHandle, NVM3_KEY_BASE + SETTING_WAVEBIRD_CHANNEL, &chan, sizeof(chan));
  nvm3_readData(nvm3_defaultHandle, NVM3_KEY_BASE + SETTING_CONTROLLER_TYPE, &cont_type, sizeof(cont_type));
  nvm3_readData(nvm3_defaultHandle, NVM3_KEY_BASE + SETTING_PIN_WIRELESS_ID, &pin_id, sizeof(pin_id));
  nvm3_readData(nvm3_defaultHandle, NVM3_KEY_BASE + SETTING_PAIRING_BUTTONS, &pair_btns, sizeof(pair_btns));

  // Set the radio channel
  wavebird_radio_set_channel(chan);

  // Initialize the WaveBird controller target
  switch (cont_type) {
    case WP_CONT_TYPE_GC_WIRED:
      joybus_gc_controller_init(&wavebird_controller, JOYBUS_GAMECUBE_CONTROLLER);
      break;
    case WP_CONT_TYPE_GC_WIRED_NOMOTOR:
      joybus_gc_controller_init(&wavebird_controller, JOYBUS_GAMECUBE_CONTROLLER | JOYBUS_ID_GCN_NO_MOTOR);
      break;
    default:
      printf("Unknown controller type '%d', defaulting to WaveBird", cont_type);
      /* fall through */
    case WP_CONT_TYPE_GC_WAVEBIRD:
      joybus_gc_controller_init(&wavebird_controller, JOYBUS_WAVEBIRD_RECEIVER);
      break;
  }

  // Print configuration
  printf("WaveBird input source initialized\n");
  printf("- WaveBird channel:     %d\n", chan + 1);
  printf("- Controller type:      %d\n", cont_type);
  printf("- Wireless ID pinning:  %s\n", pin_id ? "enabled" : "disabled");
  printf("- Pairing buttons:      0x%04X\n", pair_btns);
  printf("\n");
}

void input_source_wavebird_process(void)
{
  // Check for new wavebird packets
  wavebird_radio_process();
}

void input_source_wavebird_handle_button_press(void)
{
  pairing_active ? wavebird_radio_stop_pairing() : wavebird_radio_start_pairing();
}

struct joybus_gc_controller *input_source_wavebird_get_controller(void)
{
  return &wavebird_controller;
}