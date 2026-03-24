#include <stdio.h>
#include <string.h>

#include "btl_interface.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "em_rmu.h"
#include "sl_main_init.h"

#include "nvm3_default.h"

#include <joybus/backend/gecko.h>
#include <joybus/joybus.h>

#include <wavebird/wavebird.h>

#include "button.h"
#include "channel_wheel.h"
#include "led.h"
#include "version.h"

#define INPUT_VALID_MS 100

#define NVM3_KEY_BASE 200

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

// Settings storage, with defaults
static uint8_t chan                   = 0;
static wp_controller_type_t cont_type = WP_CONT_TYPE_GC_WAVEBIRD;
static bool pin_id                    = true;
static uint16_t pair_btns             = WB_BUTTONS_X | WB_BUTTONS_Y;

// Packet stats
struct {
  uint8_t packets;
  uint8_t radio_errors;
  uint8_t decode_errors;
  uint8_t _reserved;
} packet_stats = {0};

// Joybus state
static struct joybus_gecko joybus_gecko;
static struct joybus_gc_controller wavebird_controller;
static struct joybus *joybus = JOYBUS(&joybus_gecko);

// Buttons, switches, and LEDs
static struct led *status_led              = NULL;
static struct button *pair_button          = NULL;
static struct channel_wheel *channel_wheel = NULL;

// Pairing state
static bool pairing_active = false;

// Stale inpute validation
static uint32_t input_valid_until = 0;

// Milliseconds timer
static volatile uint32_t millis = 0;
void SysTick_Handler(void)
{
  millis++;
}

#if HAS_PAIR_BTN
// Begin or end pairing when the pair button is pressed
static void handle_pair_button_press()
{
  pairing_active ? wavebird_radio_stop_pairing() : wavebird_radio_start_pairing();
}

// Reboot into the bootloader when the pair button is held
static void handle_pair_button_hold()
{
  printf("Rebooting into bootloader...\n\n");
  bootloader_rebootAndInstall();
}
#endif

#if HAS_CHANNEL_WHEEL
// Handle channel wheel changes
static void handle_channel_wheel_change(struct channel_wheel *channel_wheel, uint8_t value)
{
  wavebird_radio_set_channel(value);
}
#endif

// Handle packets from the WaveBird radio
static void handle_wavebird_packet(const uint8_t *packet)
{
  // Update packet stats
  packet_stats.packets++;

  // Decode the WaveBird packet
  uint8_t message[WAVEBIRD_MESSAGE_BYTES];
  int rc = wavebird_packet_decode(message, packet);
  if (rc < 0) {
    // printf("Failed to decode WaveBird packet: %d\n", rc);
    packet_stats.decode_errors++;
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

  // Blink the status LED to indicate packet reception
  if (status_led)
    led_effect_blink(status_led, INPUT_VALID_MS, 1);

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
    input_valid_until = millis + INPUT_VALID_MS;
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
  packet_stats.radio_errors++;
}

// Handle pairing start events
static void handle_pairing_started(void)
{
  printf("Pairing started\n");

  // Set the pairing active flag
  pairing_active = true;

  // Disable Joybus bus during pairing
  joybus_disable(joybus);

  // Set the LED effect to indicate pairing mode
  if (status_led)
    led_effect_blink(status_led, 150, LED_REPEAT_FOREVER);
}

// Handle pairing finish events
static void handle_pairing_finished(uint8_t status, uint8_t channel)
{
  // Set the pairing active flag
  pairing_active = false;

  // Store the new channel in NVM if pairing was successful
  if (status == WB_RADIO_PAIRING_SUCCESS) {
    printf("Pairing successful, new channel: %d\n", channel + 1);

    // Set the new channel and save to NVM
    chan = channel;
    nvm3_writeData(nvm3_defaultHandle, NVM3_KEY_BASE + SETTING_WAVEBIRD_CHANNEL, &chan, sizeof(chan));

    // Set the LED solid for 1 second to indicate pairing success
    if (status_led)
      led_effect_blink(status_led, 1000, 1);
  } else if (status == WB_RADIO_PAIRING_TIMEOUT) {
    printf("Pairing timed out\n");

    // Slow-blink the status LED to indicate pairing timeout
    if (status_led)
      led_effect_blink(status_led, 500, 3);
  } else {
    printf("Pairing cancelled\n");

    // Turn off the status LED
    if (status_led)
      led_off(status_led);
  }

  // Re-enable Joybus
  joybus_enable(joybus);
}

// Get a printable name for the controller type
static const char *get_controller_type_name(uint8_t type)
{
  switch (type) {
    case WP_CONT_TYPE_GC_WAVEBIRD:
      return "WaveBird";
    case WP_CONT_TYPE_GC_WIRED:
      return "Wired GameCube";
    case WP_CONT_TYPE_GC_WIRED_NOMOTOR:
      return "Wired GameCube (no motor)";
    default:
      return "Unknown";
  }
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

int main(void)
{
  // Initialize the device
  sl_main_init();

  // Enable millisecond systick interrupts
  SysTick_Config(CMU_ClockFreqGet(cmuClock_CORE) / 1000);

  // Enable GPIO clocks
  CMU_ClockEnable(cmuClock_GPIO, true);

  // Check the reset cause and clear it
  uint32_t resetCause = RMU_ResetCauseGet();
  RMU_ResetCauseClear();

  // Make SWDIO/SWCLK available as a GPIOs
  // If the debugger caused the reset, delay first to let the probe disconnect
  if (resetCause & EMU_RSTCAUSE_SYSREQ) {
    while (millis < 100)
      ;
  }
  GPIO_DbgSWDIOEnable(false);
  GPIO_DbgSWDClkEnable(false);

  // Initialize status LED, if present
#if HAS_STATUS_LED
  static struct led _status_led;
  status_led = &_status_led;
  led_init(status_led, STATUS_LED_PORT, STATUS_LED_PIN, STATUS_LED_INVERT);
#endif

  // Initialize the pair button, if present
#if HAS_PAIR_BTN
  static struct button _pair_button;
  pair_button = &_pair_button;
  button_init(pair_button, PAIR_BTN_PORT, PAIR_BTN_PIN);
  button_set_press_callback(pair_button, handle_pair_button_press);
  button_set_long_press_callback(pair_button, handle_pair_button_hold);
#endif

  // Initialize channel wheel, if present
#if HAS_CHANNEL_WHEEL
  static struct channel_wheel _channel_wheel;
  channel_wheel = &_channel_wheel;
  channel_wheel_init(channel_wheel, CHANNEL_WHEEL_PORT_0, CHANNEL_WHEEL_PIN_0, CHANNEL_WHEEL_PORT_1,
                     CHANNEL_WHEEL_PIN_1, CHANNEL_WHEEL_PORT_2, CHANNEL_WHEEL_PIN_2, CHANNEL_WHEEL_PORT_3,
                     CHANNEL_WHEEL_PIN_3);
  channel_wheel_set_change_callback(channel_wheel, handle_channel_wheel_change);
#endif

  // Initialize persistent settings
  nvm3_readData(nvm3_defaultHandle, NVM3_KEY_BASE + SETTING_WAVEBIRD_CHANNEL, &chan, sizeof(chan));
  nvm3_readData(nvm3_defaultHandle, NVM3_KEY_BASE + SETTING_CONTROLLER_TYPE, &cont_type, sizeof(cont_type));
  nvm3_readData(nvm3_defaultHandle, NVM3_KEY_BASE + SETTING_PIN_WIRELESS_ID, &pin_id, sizeof(pin_id));
  nvm3_readData(nvm3_defaultHandle, NVM3_KEY_BASE + SETTING_PAIRING_BUTTONS, &pair_btns, sizeof(pair_btns));

  // Initialize and configure the WaveBird radio
  wavebird_radio_configure_qualification(qualify_packet, 5);
  wavebird_radio_set_pairing_started_callback(handle_pairing_started);
  wavebird_radio_set_pairing_finished_callback(handle_pairing_finished);
  wavebird_radio_init(handle_wavebird_packet, handle_wavebird_error);

  // Se the initial radio channel
  if (channel_wheel) {
    // Set the initial radio channel from the channel wheel
    wavebird_radio_set_channel(channel_wheel_get_value(channel_wheel));
  } else {
    // Set the initial radio channel from NVM (defaulting to 1)
    wavebird_radio_set_channel(chan);
  }

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

  // Initialize Joybus
  joybus_gecko_init(&joybus_gecko, JOYBUS_PORT, JOYBUS_PIN, JOYBUS_TIMER, JOYBUS_USART);
  joybus_target_register(joybus, JOYBUS_TARGET(&wavebird_controller));

  // Enable Joybus
  joybus_enable(joybus);

  // Lets-a-go!
  printf("WavePhoenix receiver ready!\n");
  printf("- Firmware version: %d.%d.%d\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
  printf("- Radio channel:    %u\n", chan + 1);
  printf("- Controller type:  %s\n", get_controller_type_name(cont_type));
  printf("\n");

  // Main loop
  while (1) {
    // Check for new wavebird packets
    wavebird_radio_process();

    // Update status LED
    if (status_led)
      led_effect_update(status_led, millis);

    // Invalidate stale inputs
    if (wavebird_controller.input_valid && (int32_t)(millis - input_valid_until) >= 0)
      joybus_gc_controller_input_valid(&wavebird_controller, false);
  }
}
