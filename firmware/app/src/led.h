/**
 * LED effects library.
 *
 * This library provides a simple API for setting effects on non-addressable LEDs,
 * such as blinking, fading, and breathing.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

// Special value for infinite repeats
#define LED_REPEAT_FOREVER -1

// Forward declaration of the led struct
struct led;

// LED effect function
typedef uint8_t (*led_effect_fn)(struct led *led, uint32_t millis, void *data);

// LED effects
enum led_effect {
  EFFECT_NONE,
  EFFECT_BLINK,
  EFFECT_BLINK_PATTERN,
  EFFECT_FADE_ON,
  EFFECT_FADE_OFF,
  EFFECT_BREATHE,
  EFFECT_CUSTOM,
};

// LED struct
struct led {
  // GPIO port and pin
  uint8_t port;
  uint8_t pin;
  bool inverted;

  // Active effect
  enum led_effect effect;

  // Data for the active effect
  struct {
    // Start time of the effect
    uint32_t start_time;

    // How many times to repeat the effect
    int8_t repeat;

    // Counter for the current iteration
    uint8_t iteration;

    union {
      // Blink, fade, and breathe effect data
      struct {
        uint16_t period;
      };

      // Blink pattern effect data
      struct {
        uint16_t *pattern;
        uint8_t length;
        uint8_t index;
        uint32_t last_change;
      };

      // Custom effect data
      struct {
        led_effect_fn user_fn;
        void *user_data;
      };
    };
  } data;
};

/**
 * Initialize the LED
 */
void led_init(struct led *led, uint8_t port, uint8_t pin, bool inverted);

/**
 * Set the LED state, and disable any effects.
 *
 * @param state The state of the LED
 */
void led_set(struct led *led, uint8_t state);

/**
 * Turn the LED on, and disable any effects.
 */
static inline void led_on(struct led *led)
{
  led_set(led, 255);
}

/**
 * Turn the LED off, and disable any effects.
 */
static inline void led_off(struct led *led)
{
  led_set(led, 0);
}

/**
 * Disable any active LED effect.
 */
void led_effect_none(struct led *led);

/**
 * Blink the LED on and off.
 *
 * @param led The LED to blink
 * @param period The period of the blink, in milliseconds
 * @param repeat The number of times to repeat the blink, or LED_REPEAT_FOREVER
 */
void led_effect_blink(struct led *led, uint16_t period, int8_t repeat);

/**
 * Blink the LED on and off with a custom pattern.
 *
 * @param led     The LED to blink
 * @param pattern The pattern to blink, where each element is the duration in milliseconds
 *                for the LED to be on or off, starting with on
 * @param length  The length of the pattern
 * @param repeat  The number of times to repeat the pattern, or LED_REPEAT_FOREVER
 */
void led_effect_blink_pattern(struct led *led, uint16_t *pattern, uint8_t length, int8_t repeat);

/**
 * Fade the LED on over time.
 *
 * @param led      The LED to fade
 * @param duration The duration of the fade, in milliseconds
 */
void led_effect_fade_on(struct led *led, uint16_t duration);

/**
 * Fade the LED off over time.
 *
 * @param led      The LED to fade
 * @param duration The duration of the fade, in milliseconds
 */
void led_effect_fade_off(struct led *led, uint16_t duration);

/**
 * Breathe the LED on and off.
 *
 * @param led      The LED to breathe
 * @param duration The duration of the breathe cycle, in milliseconds
 * @param repeat   The number of times to repeat the breathe, or LED_REPEAT_FOREVER
 */
void led_effect_breathe(struct led *led, uint16_t duration, int8_t repeat);

/**
 * Run a custom LED effect.
 *
 * @param led       The LED to run the effect on
 * @param user_fn   The custom effect function
 * @param user_data User data to pass to the custom effect function
 */
void led_effect_custom(struct led *led, led_effect_fn user_fn, void *user_data);

/**
 * Update the LED effect.
 *
 * This function should be called periodically to update the LED effect,
 * typically from an periodic interrupt, or from the main loop.
 *
 * @param led    The LED to update
 * @param millis The current time in milliseconds
 */
void led_effect_update(struct led *led, uint32_t millis);
