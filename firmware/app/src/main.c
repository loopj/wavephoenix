#include <stdio.h>
#include <string.h>

#include "btl_interface.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "sl_main_init.h"

#include <joybus/backend/gecko.h>
#include <joybus/joybus.h>

#include <wavebird/wavebird.h>

#include "button.h"
#include "channel_wheel.h"
#include "led.h"
#include "local_input.h"
#include "settings.h"
#include "version.h"

#define INPUT_VALID_MS 100

// Controller types
typedef enum {
  // Present as an OEM WaveBird receiver
  WP_CONT_TYPE_GC_WAVEBIRD,

  // Present as an OEM wired GameCube controller
  WP_CONT_TYPE_GC_WIRED,

  // Present as a wired GameCube controller without rumble
  WP_CONT_TYPE_GC_WIRED_NOMOTOR,
} wp_controller_type_t;

// Settings structure
typedef union {
  struct {
    // 0-indexed channel number (0 - 15)
    uint32_t chan : 4;

    // Should wireless ID pinning be enabled?
    uint32_t pin_id : 1;

    // Bitmask of buttons to use for pairing qualification
    uint32_t pair_btns : 12;

    // Controller type
    uint32_t cont_type : 3;

    // Reserved, must be 0
    uint32_t _reserved : 12;
  } __attribute__((packed));

  uint32_t _align; // Ensure 4-byte alignment
} wp_settings_t;

// Default settings
// - Present as an OEM WaveBird receiver, with wireless ID pinning enabled
// - Start on WaveBird channel 1 (0 indexed)
// - Virtual pairing buttons: X and Y
static const uint32_t SETTINGS_SIGNATURE    = 0x57500000;
static const wp_settings_t DEFAULT_SETTINGS = {
    .chan      = 0,
    .pin_id    = true,
    .pair_btns = WB_BUTTONS_X | WB_BUTTONS_Y,
    .cont_type = WP_CONT_TYPE_GC_WAVEBIRD,
};

// Packet stats
struct {
  uint8_t packets;
  uint8_t radio_errors;
  uint8_t decode_errors;
  uint8_t _reserved;
} packet_stats = {0};

// Joybus state
static struct joybus_gecko joybus_gecko;
static struct joybus *joybus = JOYBUS(&joybus_gecko);
static struct joybus_gc_controller wavebird_controller;
static struct joybus_gc_controller local_controller;
static struct joybus_gc_controller *active_controller = NULL;

// Buttons, switches, and LEDs
static struct led *status_led              = NULL;
static struct button *pair_button          = NULL;
static struct channel_wheel *channel_wheel = NULL;

// Pairing state
static bool pairing_active = false;

// Current settings
static wp_settings_t settings;

// Stale input validation
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
  if (settings.pin_id) {
    // Get the controller ID from the packet
    uint16_t wireless_id = wavebird_message_get_controller_id(message);

    // Check the controller id is as expected
    if (settings.cont_type == WP_CONT_TYPE_GC_WAVEBIRD) {
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
    settings.chan = channel;
    settings_save(&settings, sizeof(wp_settings_t));

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
  return (buttons & settings.pair_btns) == settings.pair_btns;
}

static void handle_local_input(struct joybus_gc_controller_input *input)
{
  // Update Joybus button states
  local_controller.input.buttons = (local_controller.input.buttons & ~JOYBUS_GCN_BUTTON_MASK) | input->buttons;

  // Update Joybus analog states
  local_controller.input.stick_x       = input->stick_x;
  local_controller.input.stick_y       = input->stick_y;
  local_controller.input.substick_x    = input->substick_x;
  local_controller.input.substick_y    = input->substick_y;
  local_controller.input.trigger_left  = input->trigger_left;
  local_controller.input.trigger_right = input->trigger_right;
}

static bool motor_changed = false;
static bool motor_enabled = false;
void motor_state_change_callback(struct joybus_gc_controller *controller, enum joybus_gcn_motor_state motor_state)
{
  motor_enabled = (motor_state == JOYBUS_GCN_MOTOR_RUMBLE);
  motor_changed = true;
}

int main(void)
{
  // Initialize the device
  sl_main_init();

  // Enable GPIO clocks
  CMU_ClockEnable(cmuClock_GPIO, true);

  // Make SWDIO/SWCLK available as a GPIOs
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

  // Enable millisecond systick interrupts
  SysTick_Config(CMU_ClockFreqGet(cmuClock_CORE) / 1000);

  // Initialize persistent settings
  settings_init(&settings, sizeof(wp_settings_t), SETTINGS_SIGNATURE, &DEFAULT_SETTINGS);

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
    wavebird_radio_set_channel(settings.chan);
  }

  // Initialize the WaveBird controller target
  switch (settings.cont_type) {
    case WP_CONT_TYPE_GC_WIRED:
      joybus_gc_controller_init(&wavebird_controller, JOYBUS_GAMECUBE_CONTROLLER);
      break;
    case WP_CONT_TYPE_GC_WIRED_NOMOTOR:
      joybus_gc_controller_init(&wavebird_controller, JOYBUS_GAMECUBE_CONTROLLER | JOYBUS_ID_GCN_NO_MOTOR);
      break;
    default:
      printf("Unknown controller type '%d', defaulting to WaveBird", settings.cont_type);
      /* fall through */
    case WP_CONT_TYPE_GC_WAVEBIRD:
      joybus_gc_controller_init(&wavebird_controller, JOYBUS_WAVEBIRD_RECEIVER);
      break;
  }

#ifdef BOARD_WAVEPHOENIX_PLUS
  // Default to local input on WavePhoenix Plus
  active_controller = &local_controller;
#else
  // Default to WaveBird input on other boards
  active_controller = &wavebird_controller;
#endif

  // Initialize Joybus
  joybus_gecko_init(&joybus_gecko, JOYBUS_PORT, JOYBUS_PIN, JOYBUS_TIMER, JOYBUS_USART);
  joybus_target_register(joybus, JOYBUS_TARGET(active_controller));

  // Enable Joybus
  joybus_enable(joybus);

#ifdef BOARD_WAVEPHOENIX_PLUS
  // Initialize the local input controller target
  joybus_gc_controller_init(&local_controller, JOYBUS_GAMECUBE_CONTROLLER);
  joybus_gc_controller_set_motor_callback(&local_controller, motor_state_change_callback);

  // Initialize local inputs
  local_input_init(handle_local_input);
#endif

  // Lets-a-go!
  printf("WavePhoenix receiver ready!\n");
  printf("- Firmware version: %d.%d.%d\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
  printf("- Radio channel:    %u\n", settings.chan + 1);
  printf("- Controller type:  %s\n", get_controller_type_name(settings.cont_type));
  printf("\n");

  // Main loop
  while (1) {
    // Check for new wavebird packets
    wavebird_radio_process();

#ifdef BOARD_WAVEPHOENIX_PLUS
    // Populate local input controller state
    local_input_process();

    // Set motor state
    if (motor_changed) {
      motor_changed = false;
      local_input_set_motor_state(motor_enabled);
    }
#endif

    // Update status LED
    if (status_led)
      led_effect_update(status_led, millis);

    // Invalidate stale wireless inputs
    if (wavebird_controller.input_valid && (int32_t)(millis - input_valid_until) >= 0) {
      joybus_gc_controller_input_valid(&wavebird_controller, false);
    }
  }
}
