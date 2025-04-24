/**
 * WaveBird radio functions.
 *
 * Frequency and modulation:
 * - Modulation:          FSK+DSSS
 * - Base frequency:      2404.8 Mhz
 * - Channel spacing:     2.4 Mhz
 * - Number of channels:  32 channels (16 channels used)
 *
 * DSSS chipping:
 * - DSSS spreading factor: 15
 * - DSSS chipping code: 0x164F = 0b01011001001111
 *
 * Data rate:
 * - 96,000 bits/s (1,440,000 chip/s)
 *
 * Message timing:
 * - 4ms per transmission (250 packets/s) -
 * - ~100us of unmodulated carrier before preamble
 * - 3,000 chips, at 1,440,000 chip/s (2083us)
 * - Silence until next transmission
 *
 * Message framing:
 * - Bit endianness:      MSB first
 * - Frame length:        25 bytes / 200 bits
 * - Preamble:            0xFAAAAAAA (32 bits)
 * - Sync word:           0x1234 (16 bits)
 * - Packet:              Remaining 19 bytes (includes CRC and footer)
 *
 * Virtual pairing:
 *   This library adds support for "virtual pairing" of WaveBird controllers.
 *   In contrast to an OEM WaveBird receiver, which has a channel selection wheel,
 *   virtual pairing allows for pairing via software, or a single button press.
 *   Once pairing is initiated, the receiver will scan all channels for activity,
 *   and qualify packets based on a user-defined qualification function. Once the
 *   qualification threshold is met, the channel is set.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

// Radio error codes
enum {
  WB_RADIO_ERR = 1,
  WB_RADIO_ERR_CALIBRATION,
  WB_RADIO_ERR_NO_PACKET,
  WB_RADIO_ERR_INVALID_PACKET_LENGTH,
  WB_RADIO_ERR_INVALID_CHANNEL,
};

// Pairing callback states
enum {
  WB_RADIO_PAIRING_SUCCESS,
  WB_RADIO_PAIRING_CANCELLED,
  WB_RADIO_PAIRING_TIMEOUT,
};

// Packet ready callback function
typedef void (*wavebird_radio_packet_fn_t)(const uint8_t *packet);

// Radio error callback function
typedef void (*wavebird_radio_error_fn_t)(int error);

// Packet qualification callback function
typedef bool (*wavebird_radio_qualify_fn_t)(const uint8_t *packet);

// Pairing callback functions
typedef void (*wavebird_radio_pairing_started_fn_t)(void);
typedef void (*wavebird_radio_pairing_finished_fn_t)(uint8_t status, uint8_t channel);

/**
 * Initialize the radio.
 *
 * @param packet_fn callback function for received packets
 * @param error_fn callback function for radio errors
 *
 * @return 0 on success, negative error code on failure
 */
int wavebird_radio_init(wavebird_radio_packet_fn_t packet_fn, wavebird_radio_error_fn_t error_fn);

/**
 * Get the current radio channel.
 *
 * @return the current channel, 0-15
 */
uint8_t wavebird_radio_get_channel(void);

/**
 * Set the radio channel, and start packet reception.
 *
 * @param channel the channel to set, 0-15
 *
 * @return 0 on success, -WB_RADIO_ERR_INVALID_CHANNEL on failure
 */
int wavebird_radio_set_channel(uint8_t channel);

/**
 * Configure pairing packet qualification.
 *
 * @param qualify_fn callback function to qualify packets
 * @param qualify_threshold number of packets that must qualify for pairing to succeed
 */
void wavebird_radio_configure_qualification(wavebird_radio_qualify_fn_t qualify_fn, uint8_t qualify_threshold);

/**
 * Set the pairing started callback function.
 *
 * @param callback callback function to call when pairing starts
 */
void wavebird_radio_set_pairing_started_callback(wavebird_radio_pairing_started_fn_t callback);

/**
 * Set the pairing finished callback function.
 *
 * @param callback callback function to call when pairing finishes
 */
void wavebird_radio_set_pairing_finished_callback(wavebird_radio_pairing_finished_fn_t callback);

/**
 * Start the virtual pairing process.
 */
void wavebird_radio_start_pairing(void);

/**
 * Stop the virtual pairing process.
 */
void wavebird_radio_stop_pairing(void);

/**
 * Process radio events.
 *
 * This function should be called periodically to process radio events.
 */
void wavebird_radio_process(void);