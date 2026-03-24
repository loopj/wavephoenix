#include "sl_gpio.h"
#include "sl_sleeptimer.h"

#include "led.h"

#include "status_led.h"

static struct led *status_led = NULL;

static uint64_t millis(void)
{
  uint64_t ms = 0;
  sl_sleeptimer_tick64_to_ms(sl_sleeptimer_get_tick_count64(), &ms);
  return ms;
}

void status_led_init(void)
{
#if HAS_STATUS_LED
  static struct led _status_led;
  status_led = &_status_led;
  led_init(status_led, STATUS_LED_PORT, STATUS_LED_PIN, STATUS_LED_INVERT);
#endif
}

void status_led_set(enum status_led_signal signal)
{
  if (!status_led)
    return;

  switch (signal) {
    case STATUS_LED_OFF:
      // Turn off the status LED
      led_off(status_led);
      break;
    case STATUS_LED_WIRELESS_ACTIVITY:
      // Briefly flash the status LED to indicate wireless activity
      led_effect_blink(status_led, 100, 1);
      break;
    case STATUS_LED_PAIRING_ACTIVE:
      // Set the status LED to indicate pairing is active
      led_effect_blink(status_led, 150, LED_REPEAT_FOREVER);
      break;
    case STATUS_LED_PAIRING_SUCCESS:
      // Set the status LED to indicate pairing was successful
      led_effect_blink(status_led, 1000, 1);
      break;
    case STATUS_LED_PAIRING_TIMEOUT:
      // Set the status LED to indicate pairing timed out
      led_effect_blink(status_led, 500, 3);
      break;
    default:
      break;
  }
}

void status_led_update(void)
{
  if (!status_led)
    return;

  led_effect_update(status_led, millis());
}