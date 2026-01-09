#include "em_gpio.h"

#include "led.h"

static void led_set_raw(struct led *led, uint8_t state)
{
  // No PWM for now on EFR32
  if (led->inverted)
    state = 255 - state;

  state ? GPIO_PinOutSet(led->port, led->pin) : GPIO_PinOutClear(led->port, led->pin);
}

void led_init(struct led *led, uint8_t port, uint8_t pin, bool inverted)
{
  // Save the configuration
  led->port     = port;
  led->pin      = pin;
  led->inverted = inverted;

  // Set the GPIO pin mode to push-pull output
  GPIO_PinModeSet(port, pin, gpioModePushPull, 0);

  // Set the initial state of the LED
  if (inverted)
    GPIO_PinOutSet(port, pin);
  else
    GPIO_PinOutClear(port, pin);
}

void led_set(struct led *led, uint8_t state)
{
  led->effect = EFFECT_NONE;
  led_set_raw(led, state);
}

void led_effect_none(struct led *led)
{
  led->effect = EFFECT_NONE;
}

void led_effect_blink(struct led *led, uint16_t period, int8_t repeat)
{
  led->effect          = EFFECT_BLINK;
  led->data.start_time = 0;
  led->data.iteration  = 0;

  led->data.repeat = repeat;
  led->data.period = period;
}

void led_effect_blink_pattern(struct led *led, uint16_t *pattern, uint8_t length, int8_t repeat)
{
  led->effect          = EFFECT_BLINK_PATTERN;
  led->data.start_time = 0;
  led->data.iteration  = 0;

  led->data.repeat      = repeat;
  led->data.pattern     = pattern;
  led->data.length      = length;
  led->data.index       = 0;
  led->data.last_change = 0;
}

void led_effect_fade_on(struct led *led, uint16_t period)
{
  led->effect          = EFFECT_FADE_ON;
  led->data.start_time = 0;

  led->data.period = period;
}

void led_effect_fade_off(struct led *led, uint16_t period)
{
  led->effect          = EFFECT_FADE_OFF;
  led->data.start_time = 0;

  led->data.period = period;
}

void led_effect_breathe(struct led *led, uint16_t period, int8_t repeat)
{
  led->effect          = EFFECT_BREATHE;
  led->data.start_time = 0;
  led->data.iteration  = 0;

  led->data.repeat = repeat;
  led->data.period = period;
}

void led_effect_custom(struct led *led, led_effect_fn user_fn, void *user_data)
{
  led->effect          = EFFECT_CUSTOM;
  led->data.start_time = 0;

  led->data.user_fn   = user_fn;
  led->data.user_data = user_data;
}

void led_effect_update(struct led *led, uint32_t millis)
{
  if (led->effect == EFFECT_NONE)
    return;

  // Initialize the start time
  if (led->data.start_time == 0)
    led->data.start_time = millis;

  // Calculate the elapsed time since the effect started
  uint32_t elapsed = millis - led->data.start_time;

  // Update the LED state based on time
  uint32_t pos = elapsed % (led->data.period * 2);
  led_set_raw(led, pos < led->data.period ? 255 : 0);

  // Check if the effect has completed
  if (elapsed >= led->data.period * 2) {
    led->data.iteration++;

    if (led->data.repeat != LED_REPEAT_FOREVER && led->data.iteration >= led->data.repeat) {
      led_set(led, 0);
    } else {
      led->data.start_time = millis;
    }
  }
}