/**
 * WaveBird message unpacking functions.
 *
 * After WaveBird packets are decoded, they result in an 84-bit message.
 * Messages have a 16-bit header, followed by a 68-bit body padded with zeros.
 * There are two types of messages, "input state" messages and "origin" messages.
 *
 * Message headers are structured as follows:
 *   Bits 15-12:  Unknown, always seems to be 0
 *   Bit 11:      Unknown, always seems to be 1
 *   Bit 10:      Message type (0 = input state, 1 = origin)
 *   Bits 9-0:    Controller ID
 *
 * Header examples:
 *   0x0AB1 (input state message, controller ID 0x2B1)
 *   0x0C38 (origin message, controller ID 0x038)
 *
 * Input state messages describe the current state of a controller's buttons, sticks,
 * and triggers. They are broadcast 250 times per second.
 *
 * Input state messages are structured as follows:
 *   0xHHHHBBBXXYYCXCYLLRRFF
 *   - HHHH:  16-bit message header (see above)
 *   - BBB:   12-bit button state (Start, Y, X, B, A, L, R, Z, Up, Down, Right, Left)
 *   - XX:    8-bit stick X position
 *   - YY:    8-bit stick Y position
 *   - CX:    8-bit C-stick X position
 *   - CY:    8-bit C-stick Y position
 *   - LL:    8-bit left analog trigger position
 *   - RR:    8-bit right analog trigger position
 *   - FF:    footer, likely just padding
 *
 * Input state message example:
 *   0x0AB1180DA568A831A1300
 *
 * Origin messages describe the state of a controller's analog sticks and triggers when
 * it was first powered on. They are broadcast once when the controller is powered on,
 * and then repeated every second.
 *
 * Origin messages are structured as follows:
 *   0xHHHHXXYYCXCYLLRRFFFFF
 *   - HHHH:  16-bit message header (see above)
 *   - XX:    8-bit stick X origin
 *   - YY:    8-bit stick Y origin
 *   - CX:    8-bit C-stick X origin
 *   - CY:    8-bit C-stick Y positiooriginn
 *   - LL:    8-bit left analog trigger origin
 *   - RR:    8-bit right analog trigger origin
 *   - FFFFF: footer, likely just padding
 *
 * Origin message example:
 *   0x0EB1867F8B831B1300000
 *
 * Things to note:
 * - When decoding a packet the 84 bits are stored "right aligned" in an 11-byte buffer.
 * - We're explicitly not using bitfield structs here, since input and origin messages
 *   have different byte alignments for the stick and trigger values.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

/**
 * WaveBird message types.
 */
enum {
  WB_MESSAGE_TYPE_INPUT_STATE = 0,
  WB_MESSAGE_TYPE_ORIGIN,
};

/**
 * Button state bitfield struct for WaveBird input state messages.
 */
typedef union {
  struct {
    uint16_t left  : 1;
    uint16_t right : 1;
    uint16_t down  : 1;
    uint16_t up    : 1;
    uint16_t z     : 1;
    uint16_t r     : 1;
    uint16_t l     : 1;
    uint16_t a     : 1;
    uint16_t b     : 1;
    uint16_t x     : 1;
    uint16_t y     : 1;
    uint16_t start : 1;
  };
  uint16_t raw;
} wavebird_buttons_t;

/**
 * Button state bitmask for WaveBird input state messages.
 */
#define WB_BUTTONS_LEFT       (1 << 0)
#define WB_BUTTONS_RIGHT      (1 << 1)
#define WB_BUTTONS_DOWN       (1 << 2)
#define WB_BUTTONS_UP         (1 << 3)
#define WB_BUTTONS_Z          (1 << 4)
#define WB_BUTTONS_R          (1 << 5)
#define WB_BUTTONS_L          (1 << 6)
#define WB_BUTTONS_A          (1 << 7)
#define WB_BUTTONS_B          (1 << 8)
#define WB_BUTTONS_X          (1 << 9)
#define WB_BUTTONS_Y          (1 << 10)
#define WB_BUTTONS_START      (1 << 11)

/**
 * Get the controller ID from the header of a WaveBird message.
 *
 * @param message the message decoded from a WaveBird packet
 *
 * @return the controller ID
 */
static inline uint16_t wavebird_message_get_controller_id(const uint8_t *message)
{
  return ((message[0] & 0x0F) << 12 | message[1] << 4 | message[2] >> 4) & 0x3FF;
}

/**
 * Get the message type from a WaveBird message.
 *
 * @param message the message decoded from a WaveBird packet
 *
 * @return the message type
 */
static inline uint8_t wavebird_message_get_type(const uint8_t *message)
{
  return message[1] & 0x40 ? WB_MESSAGE_TYPE_ORIGIN : WB_MESSAGE_TYPE_INPUT_STATE;
}

/**
 * Get the buttons from a WaveBird input state message.
 *
 * @param message the message decoded from a WaveBird packet
 *
 * @return the button state
 */
static inline uint16_t wavebird_input_state_get_buttons(const uint8_t *message)
{
  return (message[2] & 0x0F) << 8 | message[3];
}

/**
 * Get the stick X position from a WaveBird input state message.
 *
 * @param message the message decoded from a WaveBird packet
 *
 * @return the stick X position
 */
static inline uint8_t wavebird_input_state_get_stick_x(const uint8_t *message)
{
  return message[4];
}

/**
 * Get the stick Y position from a WaveBird input state message.
 *
 * @param message the message decoded from a WaveBird packet
 *
 * @return the stick Y position
 */
static inline uint8_t wavebird_input_state_get_stick_y(const uint8_t *message)
{
  return message[5];
}

/**
 * Get the C-stick X position from a WaveBird input state message.
 *
 * @param message the message decoded from a WaveBird packet
 *
 * @return the C-stick X position
 */
static inline uint8_t wavebird_input_state_get_substick_x(const uint8_t *message)
{
  return message[6];
}

/**
 * Get the C-stick Y position from a WaveBird input state message.
 *
 * @param message the message decoded from a WaveBird packet
 *
 * @return the C-stick Y position
 */
static inline uint8_t wavebird_input_state_get_substick_y(const uint8_t *message)
{
  return message[7];
}

/**
 * Get the left analog trigger position from a WaveBird input state message.
 *
 * @param message the message decoded from a WaveBird packet
 *
 * @return the left analog trigger position
 */
static inline uint8_t wavebird_input_state_get_trigger_left(const uint8_t *message)
{
  return message[8];
}

/**
 * Get the right analog trigger position from a WaveBird input state message.
 *
 * @param message the message decoded from a WaveBird packet
 *
 * @return the right analog trigger position
 */
static inline uint8_t wavebird_input_state_get_trigger_right(const uint8_t *message)
{
  return message[9];
}

/**
 * Get the stick X origin from a WaveBird origin message.
 *
 * @param message the message decoded from a WaveBird packet
 *
 * @return the stick X origin
 */
static inline uint8_t wavebird_origin_get_stick_x(const uint8_t *message)
{
  return (message[2] & 0x0F) << 4 | message[3] >> 4;
}

/**
 * Get the stick Y origin from a WaveBird origin message.
 *
 * @param message the message decoded from a WaveBird packet
 *
 * @return the stick Y origin
 */
static inline uint8_t wavebird_origin_get_stick_y(const uint8_t *message)
{
  return (message[3] & 0x0F) << 4 | message[4] >> 4;
}

/**
 * Get the C-stick X origin from a WaveBird origin message.
 *
 * @param message the message decoded from a WaveBird packet
 *
 * @return the C-stick X origin
 */
static inline uint8_t wavebird_origin_get_substick_x(const uint8_t *message)
{
  return (message[4] & 0x0F) << 4 | message[5] >> 4;
}

/**
 * Get the C-stick Y origin from a WaveBird origin message.
 *
 * @param message the message decoded from a WaveBird packet
 *
 * @return the C-stick Y origin
 */
static inline uint8_t wavebird_origin_get_substick_y(const uint8_t *message)
{
  return (message[5] & 0x0F) << 4 | message[6] >> 4;
}

/**
 * Get the left analog trigger origin from a WaveBird origin message.
 *
 * @param message the message decoded from a WaveBird packet
 *
 * @return the left analog trigger origin
 */
static inline uint8_t wavebird_origin_get_trigger_left(const uint8_t *message)
{
  return (message[6] & 0x0F) << 4 | message[7] >> 4;
}

/**
 * Get the right analog trigger origin from a WaveBird origin message.
 *
 * @param message the message decoded from a WaveBird packet
 *
 * @return the right analog trigger origin
 */
static inline uint8_t wavebird_origin_get_trigger_right(const uint8_t *message)
{
  return (message[7] & 0x0F) << 4 | message[8] >> 4;
}