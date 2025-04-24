/**
 * Channel wheel support for 4-bit rotary DIP switches
 */

#pragma once

#include <stdint.h>

// Forward declaration of the channel_wheel struct for the callback
struct channel_wheel;

// Callback function for when the channel wheel value changes
typedef void (*channel_wheel_change_callback_t)(struct channel_wheel *channel_wheel, uint8_t value);

// Channel wheel struct
struct channel_wheel {
  // GPIOs
  uint8_t bit0_port;
  uint8_t bit0_pin;
  uint8_t bit1_port;
  uint8_t bit1_pin;
  uint8_t bit2_port;
  uint8_t bit2_pin;
  uint8_t bit3_port;
  uint8_t bit3_pin;

  // Callback for when the channel wheel value changes
  channel_wheel_change_callback_t change_callback;
};

/**
 * Initialize a hex dip switch as a channel wheel
 *
 * @param channel_wheel The channel wheel to initialize
 */
void channel_wheel_init(struct channel_wheel *channel_wheel, uint8_t bit0_port, uint8_t bit0_pin, uint8_t bit1_port,
                        uint8_t bit1_pin, uint8_t bit2_port, uint8_t bit2_pin, uint8_t bit3_port, uint8_t bit3_pin);

/**
 * Set the callback for when the hex dip switch value changes
 *
 * @param channel_wheel The channel wheel
 * @param callback The callback function
 */
void channel_wheel_set_change_callback(struct channel_wheel *channel_wheel, channel_wheel_change_callback_t callback);

/**
 * Get the binary value of the channel wheel
 *
 * @param channel_wheel The channel wheel
 *
 * @return The value of the hex dip switch
 */
uint8_t channel_wheel_get_value(struct channel_wheel *channel_wheel);