#include <string.h>

#include "btl_interface.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_gpio.h"

#include "si/commands.h"
#include "si/device/gc_controller.h"
#include "wavebird/message.h"
#include "wavebird/packet.h"
#include "wavebird/radio.h"

#include "button.h"
#include "channel_wheel.h"
#include "led.h"
#include "serial.h"
#include "settings.h"
#include "version.h"

#include "board_config.h"

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

// SI state
static struct si_device_gc_controller si_device = {0};
static bool enable_si_command_handling          = true;

// Buttons, switches, and LEDs
static struct led *status_led              = NULL;
static struct button *pair_button          = NULL;
static struct channel_wheel *channel_wheel = NULL;

// Pairing state
static bool pairing_active = false;

// Current settings
static wp_settings_t settings;

// Stale inpute validation
static uint32_t input_valid_until = 0;

// Milliseconds timer
static volatile uint32_t millis = 0;
void SysTick_Handler(void)
{
  millis++;
}

// Initialize (or reinitialize) as a controller on the SI bus
static void initialize_controller(uint8_t controller_type)
{
  if (controller_type == WP_CONT_TYPE_GC_WAVEBIRD) {
    // Present as an OEM WaveBird receiver
    si_device_gc_init(&si_device, SI_TYPE_GC | SI_GC_WIRELESS | SI_GC_NOMOTOR);
    enable_si_command_handling = true;
  } else if (controller_type == WP_CONT_TYPE_GC_WIRED_NOMOTOR) {
    // Present as a wired GameCube controller without rumble
    si_device_gc_init(&si_device, SI_TYPE_GC | SI_GC_STANDARD | SI_GC_NOMOTOR);
  } else if (controller_type == WP_CONT_TYPE_GC_WIRED) {
    // Present as an OEM wired GameCube controller
    si_device_gc_init(&si_device, SI_TYPE_GC | SI_GC_STANDARD);
  }
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
  DEBUG_PRINT("Rebooting into bootloader...\n\n");
  DEBUG_FLUSH();

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
    // DEBUG_PRINT("Failed to decode WaveBird packet: %d\n", rcode);
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
      if (si_device_gc_wireless_id_fixed(&si_device)) {
        // Drop packets from other controllers if the ID has been fixed
        if (si_device_gc_get_wireless_id(&si_device) != wireless_id)
          return;
      } else {
        // Set the controller ID if it is not fixed
        si_device_gc_set_wireless_id(&si_device, wireless_id);
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

    // Clear the buttons in the SI input state
    si_device.input.buttons.bytes[0] &= ~0x1F;
    si_device.input.buttons.bytes[1] &= ~0x7F;

    // Copy the buttons from the WaveBird message
    si_device.input.buttons.bytes[0] |= (message[3] & 0x80) >> 7 | (message[2] & 0x0F) << 1;
    si_device.input.buttons.bytes[1] |= (message[3] & 0x7F);

    // Copy the stick, substick, and trigger values
    memcpy(&si_device.input.stick_x, &message[4], 6);

    // We have a good input state, enable SI command handling if it was disabled
    enable_si_command_handling = true;

    // Set the input state as valid
    input_valid_until = millis + INPUT_VALID_MS;
    si_device_set_input_valid(&si_device, true);
  } else {
    //
    // Handle origin packets
    //

    // Copy the origin values from the packet
    uint8_t new_origin[] = {
        wavebird_origin_get_stick_x(message),      wavebird_origin_get_stick_y(message),
        wavebird_origin_get_substick_x(message),   wavebird_origin_get_substick_y(message),
        wavebird_origin_get_trigger_left(message), wavebird_origin_get_trigger_right(message),
    };

    // Check if the origin packet is different from the last known origin
    if (memcmp(&si_device.origin.stick_x, new_origin, 6) != 0) {
      // Update the origin state
      memcpy(&si_device.origin.stick_x, new_origin, 6);

      // Set the "need origin" flag to true so the host knows to fetch the new origin
      si_device.input.buttons.need_origin = true;
    }
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
  DEBUG_PRINT("Pairing started\n");

  // Set the pairing active flag
  pairing_active = true;

  // Disable SI command handling during pairing
  enable_si_command_handling = false;

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
    DEBUG_PRINT("Pairing successful, new channel: %d\n", channel + 1);

    // Set the new channel and save to NVM
    settings.chan = channel;
    settings_save(&settings, sizeof(wp_settings_t));

    // Set the LED solid for 1 second to indicate pairing success
    if (status_led)
      led_effect_blink(status_led, 1000, 1);

    // Reset the controller
    initialize_controller(settings.cont_type);
  } else if (status == WB_RADIO_PAIRING_TIMEOUT) {
    DEBUG_PRINT("Pairing timed out\n");

    // Slow-blink the status LED to indicate pairing timeout
    if (status_led)
      led_effect_blink(status_led, 500, 3);

    // Immediately reenable SI command handling
    enable_si_command_handling = true;
  } else {
    DEBUG_PRINT("Pairing cancelled\n");

    // Turn off the status LED
    if (status_led)
      led_off(status_led);

    // Immediately reenable SI command handling
    enable_si_command_handling = true;
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

void system_init(void)
{
  // Chip errata
  CHIP_Init();

  // HFXO initialization
  CMU_HFXOInit_TypeDef hfxoInit = CMU_HFXOINIT_DEFAULT;
  hfxoInit.ctuneXoAna           = HFXO_CTUNE;
  hfxoInit.ctuneXiAna           = HFXO_CTUNE;
  CMU_HFXOInit(&hfxoInit);
  SystemHFXOClockSet(HFXO_FREQ);

  // PLL initialization
  CMU_DPLLInit_TypeDef dpllInit = CMU_DPLL_HFXO_TO_76_8MHZ;
  bool dpllLock                 = false;
  while (!dpllLock)
    dpllLock = CMU_DPLLLock(&dpllInit);
  CMU_ClockSelectSet(cmuClock_SYSCLK, cmuSelect_HFRCODPLL);

  // Set default NVIC priorities
  for (IRQn_Type i = SVCall_IRQn; i < EXT_IRQ_COUNT; i++)
    NVIC_SetPriority(i, CORE_INTERRUPT_DEFAULT_PRIORITY);
}

// Initialize the various GPIOs
static void gpio_init(void)
{
  // Enable GPIO clocks
  CMU_ClockEnable(cmuClock_GPIO, true);

  // Make SWDIO available as a GPIO, if necessary
  if (SI_DATA_PORT == GPIO_SWDIO_PORT && SI_DATA_PIN == GPIO_SWDIO_PIN) {
    DEBUG_PRINT("[WARNING] SI is using SWDIO as GPIO, disabling SWD\n");
    GPIO_DbgSWDIOEnable(false);
  }

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
}

int main(void)
{
  // Initialize the system
  system_init();

  // Initialize the debug console
  serial_init(115200);

  // Initialize the GPIOs
  gpio_init();

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

  // Initialize the SI bus
  si_init(SI_DATA_PORT, SI_DATA_PIN, SI_MODE_DEVICE, 200000, 250000);

  // Register to handle controller SI commands
  initialize_controller(settings.cont_type);

  // Lets-a-go!
  DEBUG_PRINT("WavePhoenix receiver ready!\n");
  DEBUG_PRINT("- Firmware version: %d.%d.%d\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
  DEBUG_PRINT("- Radio channel:    %u\n", settings.chan + 1);
  DEBUG_PRINT("- Controller type:  %s\n", (settings.cont_type == WP_CONT_TYPE_GC_WAVEBIRD) ? "WaveBird"
                                          : (settings.cont_type == WP_CONT_TYPE_GC_WIRED)  ? "Wired"
                                                                                           : "Wired (no motor)");
  DEBUG_PRINT("\n");

  // Wait for the SI bus to be idle before starting the main loop
  si_await_bus_idle();

  // Main loop
  while (1) {
    // Check if we need to initiate the next SI transfer
    if (enable_si_command_handling)
      si_command_process();

    // Check for new wavebird packets
    wavebird_radio_process();

    // Update status LED
    if (status_led)
      led_effect_update(status_led, millis);

    // Invalidate stale inputs
    if (si_device.input_valid && (int32_t)(millis - input_valid_until) >= 0)
      si_device_set_input_valid(&si_device, false);
  }
}
