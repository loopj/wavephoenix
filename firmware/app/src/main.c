#include <stdio.h>
#include <string.h>

#include "btl_interface.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "em_rmu.h"

#include "sl_main_init.h"
#include "sl_sleeptimer.h"

#include "nvm3_default.h"

#include <joybus/backend/gecko.h>
#include <joybus/joybus.h>

#include "button.h"
#include "status_led.h"
#include "version.h"

#include "input_source/wavebird.h"

// Joybus state
static struct joybus_gecko joybus_gecko;
static struct joybus *joybus = JOYBUS(&joybus_gecko);

// Currently active controller
static struct joybus_gc_controller *active_controller = NULL;

// Pair button
static struct button *pair_button = NULL;

#if HAS_PAIR_BTN
// Begin or end pairing when the pair button is pressed
static void handle_pair_button_press()
{
  input_source_wavebird_handle_button_press();
}

// Reboot into the bootloader when the pair button is held
static void handle_pair_button_hold()
{
  printf("Rebooting into bootloader...\n\n");
  bootloader_rebootAndInstall();
}
#endif

int main(void)
{
  // Initialize the device
  sl_main_init();

  // Enable GPIO clocks
  CMU_ClockEnable(cmuClock_GPIO, true);

  // Check/clear the reset cause
  uint32_t resetCause = RMU_ResetCauseGet();
  RMU_ResetCauseClear();

  // If the debugger caused the reset, delay first to let the probe disconnect
  if (resetCause & EMU_RSTCAUSE_SYSREQ)
    sl_sleeptimer_delay_millisecond(100);

  // Make SWDIO/SWCLK available as a GPIOs
  GPIO_DbgSWDIOEnable(false);
  GPIO_DbgSWDClkEnable(false);

  // Initialize NVM driver for settings storage
  nvm3_initDefault();

  // Initialize the pair button, if present
#if HAS_PAIR_BTN
  static struct button _pair_button;
  pair_button = &_pair_button;
  button_init(pair_button, PAIR_BTN_PORT, PAIR_BTN_PIN);
  button_set_press_callback(pair_button, handle_pair_button_press);
  button_set_long_press_callback(pair_button, handle_pair_button_hold);
#endif

  // Initialize status LED, if present
  status_led_init();

  // Initialize WaveBird input source
  input_source_wavebird_init();
  active_controller = input_source_wavebird_get_controller();

  // Initialize Joybus
  joybus_gecko_init(&joybus_gecko, JOYBUS_PORT, JOYBUS_PIN, JOYBUS_TIMER, JOYBUS_USART);
  joybus_target_register(joybus, JOYBUS_TARGET(active_controller));

  // Enable Joybus
  joybus_enable(joybus);

  // Lets-a-go!
  printf("WavePhoenix receiver v%d.%d.%d ready!\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);

  // Main loop
  while (1) {
    // Check for new wavebird inputs
    input_source_wavebird_process();

    // Update status LED
    status_led_update();
  }
}
