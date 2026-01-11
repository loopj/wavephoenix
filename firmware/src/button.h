/**
 * Quick and dirty button abstraction for EFR32.
 * Handles debouncing and long press detection of a single button entirely with interrupts.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

// Forward declaration of the button struct for the callback
struct button;

// Callback function type for button events
typedef void (*button_callback_t)(struct button *button);

// Button state
enum button_state {
  BUTTON_STATE_IDLE,
  BUTTON_STATE_DEBOUNCING,
  BUTTON_STATE_PENDING_LONG_PRESS,
};

// Button struct
struct button {
  // GPIO port and pin
  uint8_t port;
  uint8_t pin;

  // State
  enum button_state button_state;

  // Callbacks
  button_callback_t press_callback;
  button_callback_t long_press_callback;
};

/**
 * Initialize a button
 *
 * @param button The button to initialize
 * @param port The port of the button
 * @param pin The pin of the button
 */
void button_init(struct button *button, uint8_t port, uint8_t pin);

/**
 * Set the callback function for when the button is pressed
 *
 * @param button The button to set the callback for
 * @param press_callback The callback function for when the button is pressed
 */
void button_set_press_callback(struct button *button, button_callback_t press_callback);

/**
 * Set the callback function for when the button is held for a long time
 *
 * @param button The button to set the callback for
 * @param long_press_callback The callback function for when the button is held for a long time
 */
void button_set_long_press_callback(struct button *button, button_callback_t long_press_callback);