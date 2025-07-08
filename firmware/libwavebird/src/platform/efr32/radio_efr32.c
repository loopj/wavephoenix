/**
 * WaveBird radio implementation for EFR32 radios.
 */

#include "rail.h"
#include "rail_config.h"

#include "wavebird/packet.h"
#include "wavebird/radio.h"

// Radio states
enum {
  WB_RADIO_IDLE,
  WB_RADIO_RX_PAIRING_SCANNING,
  WB_RADIO_RX_PAIRING_READY_TO_QUALIFY,
  WB_RADIO_RX_PAIRING_QUALIFYING,
  WB_RADIO_RX_ACTIVE,
};

// Mapping from WaveBird channel number to channel index
// Assumes a starting frequency of 2404.80 Mhz, with channel spacing of 2.4 Mhz
// The channel map is 0-indexed, WaveBird channels on the channel dial are 1-indexed
static const uint8_t WAVEBIRD_CHANNEL_MAP[] = {
    31, 29, 0, 2, 6, 4, 8, 10, 14, 12, 17, 19, 23, 21, 25, 27,
};

// Interrupt status flags
static volatile bool packet_held = false;
static volatile int error_code   = 0;

// Current radio state
static uint8_t radio_state     = WB_RADIO_IDLE;
static uint8_t current_channel = 0;

// Callback functions
static wavebird_radio_work_pending_fn_t work_pending_callback         = NULL;
static wavebird_radio_packet_fn_t packet_callback                     = NULL;
static wavebird_radio_error_fn_t error_callback                       = NULL;
static wavebird_radio_pairing_started_fn_t pairing_started_callback   = NULL;
static wavebird_radio_pairing_finished_fn_t pairing_finished_callback = NULL;

// RAIL handle and RX buffer
static RAIL_Handle_t rail_handle;
static __ALIGNED(RAIL_FIFO_ALIGNMENT) uint8_t packet_buffer[WAVEBIRD_PACKET_BYTES];

// RAIL channel hopping configuration
uint32_t channel_hopping_buffer[1024]; // TODO
RAIL_RxChannelHoppingConfigEntry_t channel_hopping_entries[WAVEBIRD_RADIO_NUM_CHANNELS];
RAIL_RxChannelHoppingConfig_t channel_hopping_config = {
    .entries          = channel_hopping_entries,
    .bufferLength     = 1024,
    .buffer           = channel_hopping_buffer,
    .numberOfChannels = WAVEBIRD_RADIO_NUM_CHANNELS,
};

// Pairing timeouts
#define PAIRING_DETECT_TIMEOUT  10000     // Listen for sync words for 10ms on each channel
#define PAIRING_QUALIFY_TIMEOUT 200000    // Hold on the channel for 200ms to qualify activity

// Pairing configuration
static wavebird_radio_qualify_fn_t qualify_fn = NULL;
static uint8_t qualify_threshold              = 5;

// Pairing state
static struct pairing_state {
  uint16_t channel;
  uint32_t qualify_timeout;
  uint8_t qualified_packets;
} pairing_state;

static void start_channel_scanning(bool reset)
{
  RAIL_Idle(rail_handle, RAIL_IDLE, true);
  RAIL_EnableRxChannelHopping(rail_handle, true, reset);
  RAIL_StartRx(rail_handle, WAVEBIRD_CHANNEL_MAP[0], NULL);

  radio_state = WB_RADIO_RX_PAIRING_SCANNING;
}

static void stop_channel_scanning(void)
{
  RAIL_Idle(rail_handle, RAIL_IDLE, true);
  RAIL_EnableRxChannelHopping(rail_handle, false, false);
}

// Interrupt handler for RAIL events
static void handle_rail_event(RAIL_Handle_t handle, RAIL_Events_t events)
{
  // Handle RX events
  if (events & RAIL_EVENTS_RX_COMPLETION) {
    if (events & RAIL_EVENT_RX_PACKET_RECEIVED) {
      // When in active RX mode, or qualifying a channel for pairing, hold the packet
      if (radio_state == WB_RADIO_RX_PAIRING_QUALIFYING || radio_state == WB_RADIO_RX_ACTIVE) {
        RAIL_HoldRxPacket(handle);
        packet_held = true;

        if (work_pending_callback)
          work_pending_callback();
      }
    } else {
      // RX completed without a packet, this is an error
      error_code = -WB_RADIO_ERR_NO_PACKET;

      if (work_pending_callback)
        work_pending_callback();
    }
  }

  // Check for sync words during channel scanning
  if (events & RAIL_EVENT_RX_SYNC1_DETECT) {
    if (radio_state == WB_RADIO_RX_PAIRING_SCANNING) {
      RAIL_GetChannelAlt(handle, &pairing_state.channel);
      radio_state = WB_RADIO_RX_PAIRING_READY_TO_QUALIFY;

      if (work_pending_callback)
        work_pending_callback();
    }
  }

  // Perform all calibrations when needed
  if (events & RAIL_EVENT_CAL_NEEDED) {
    RAIL_Status_t status = RAIL_Calibrate(handle, NULL, RAIL_CAL_ALL_PENDING);
    if (status != RAIL_STATUS_NO_ERROR) {
      // Calibration error
      error_code = -WB_RADIO_ERR_CALIBRATION;

      if (work_pending_callback)
        work_pending_callback();
    }
  }
}

// Copy the oldest pending packet from the radio buffer to the application buffer
static bool get_oldest_pending_packet(uint8_t *buffer, RAIL_Handle_t rail_handle)
{
  RAIL_RxPacketInfo_t packet_info;
  RAIL_RxPacketHandle_t rx_handle;

  // Get the oldest complete packet (if any)
  rx_handle = RAIL_GetRxPacketInfo(rail_handle, RAIL_RX_PACKET_HANDLE_OLDEST_COMPLETE, &packet_info);
  if (rx_handle == RAIL_RX_PACKET_HANDLE_INVALID)
    return false;

  // Copy the packet from the radio buffer to the application buffer
  RAIL_CopyRxPacket(buffer, &packet_info);
  RAIL_ReleaseRxPacket(rail_handle, rx_handle);

  return true;
}

int wavebird_radio_init(wavebird_radio_packet_fn_t packet_fn, wavebird_radio_error_fn_t error_fn)
{
  RAIL_Status_t status = RAIL_STATUS_NO_ERROR;

  // Set the callback functions
  packet_callback = packet_fn;
  error_callback  = error_fn;

  // Initialize RAIL handle
  RAIL_Config_t rail_config = {.eventsCallback = handle_rail_event};
  rail_handle               = RAIL_Init(&rail_config, NULL);
  if (rail_handle == NULL)
    return -WB_RADIO_ERR;

  // Configure data handling
  RAIL_DataConfig_t data_config = {TX_PACKET_DATA, RX_PACKET_DATA, PACKET_MODE, PACKET_MODE};
  status |= RAIL_ConfigData(rail_handle, &data_config);

  // Configure channels, channelConfigs is generated from radio_settings.radioconf
  status |= RAIL_ConfigChannels(rail_handle, channelConfigs[0], NULL);

  // Configure calibration
  status |= RAIL_ConfigCal(rail_handle, RAIL_CAL_ALL);

  // Configure events
  RAIL_Events_t event_mask =
      (RAIL_EVENT_RX_SYNC1_DETECT | RAIL_EVENT_CAL_NEEDED | RAIL_EVENTS_RX_COMPLETION | RAIL_EVENT_RX_PACKET_RECEIVED);
  status |= RAIL_ConfigEvents(rail_handle, RAIL_EVENTS_ALL, event_mask);

  // Configure RX transitions
  RAIL_StateTransitions_t rx_transitions = {RAIL_RF_STATE_RX, RAIL_RF_STATE_RX};
  status |= RAIL_SetRxTransitions(rail_handle, &rx_transitions);

  // Initialize channel hopping configuration
  for (uint8_t i = 0; i < WAVEBIRD_RADIO_NUM_CHANNELS; i++) {
    channel_hopping_entries[i].channel   = WAVEBIRD_CHANNEL_MAP[i];
    channel_hopping_entries[i].mode      = RAIL_RX_CHANNEL_HOPPING_MODE_TIMING_SENSE;
    channel_hopping_entries[i].parameter = PAIRING_DETECT_TIMEOUT;
    channel_hopping_entries[i].delay     = 0; // No delay between hops
  }
  status |= RAIL_ConfigRxChannelHopping(rail_handle, &channel_hopping_config);

  // Check for errors during configuration
  if (status != RAIL_STATUS_NO_ERROR)
    return -WB_RADIO_ERR;

  return 0;
}

uint8_t wavebird_radio_get_channel(void)
{
  return current_channel;
}

int wavebird_radio_set_channel(uint8_t channel)
{
  // Check the channel is valid
  if (channel > 15)
    return -WB_RADIO_ERR_INVALID_CHANNEL;

  // Get the RAIL channel from the WaveBird channel number
  uint8_t rail_channel = WAVEBIRD_CHANNEL_MAP[channel];

  // Start receiving on the new channel
  RAIL_StartRx(rail_handle, rail_channel, NULL);

  // Update the current channel
  current_channel = channel;

  // Update the radio state
  radio_state = WB_RADIO_RX_ACTIVE;

  return 0;
}

void wavebird_radio_configure_qualification(wavebird_radio_qualify_fn_t _qualify_fn, uint8_t _qualify_threshold)
{
  qualify_fn        = _qualify_fn;
  qualify_threshold = _qualify_threshold;
}

void wavebird_radio_set_work_pending_callback(wavebird_radio_work_pending_fn_t callback)
{
  work_pending_callback = callback;
}

void wavebird_radio_set_pairing_started_callback(wavebird_radio_pairing_started_fn_t callback)
{
  pairing_started_callback = callback;
}

void wavebird_radio_set_pairing_finished_callback(wavebird_radio_pairing_finished_fn_t callback)
{
  pairing_finished_callback = callback;
}

void wavebird_radio_start_pairing(void)
{
  // Start the channel scanning process
  start_channel_scanning(true);

  // Fire the pairing started callback
  if (pairing_started_callback)
    pairing_started_callback();
}

void wavebird_radio_stop_pairing(void)
{
  // Stop channel scanning
  stop_channel_scanning();

  // Reset the channel
  wavebird_radio_set_channel(current_channel);

  // Fire the pairing finished callback
  if (pairing_finished_callback)
    pairing_finished_callback(WB_RADIO_PAIRING_CANCELLED, current_channel);
}

void wavebird_radio_process(void)
{
  switch (radio_state) {
    case WB_RADIO_RX_PAIRING_READY_TO_QUALIFY:
      stop_channel_scanning();

      RAIL_StartRx(rail_handle, pairing_state.channel, NULL);

      pairing_state.qualify_timeout   = RAIL_GetTime() + PAIRING_QUALIFY_TIMEOUT;
      pairing_state.qualified_packets = 0;
      radio_state                     = WB_RADIO_RX_PAIRING_QUALIFYING;

      break;
    // Hold on the channel for a short time to qualify pairing activity
    case WB_RADIO_RX_PAIRING_QUALIFYING:
      // Check for packets on the current channel
      if (packet_held) {
        while (get_oldest_pending_packet(packet_buffer, rail_handle)) {
          // Check if the packet qualifies for pairing
          if (!qualify_fn || qualify_fn(packet_buffer)) {
            pairing_state.qualified_packets++;
          }

          // If we have received enough qualifying packets, finish pairing
          if (pairing_state.qualified_packets >= qualify_threshold) {
            // Look up the channel in the channel map
            uint8_t channel_number;
            for (channel_number = 0; channel_number < WAVEBIRD_RADIO_NUM_CHANNELS; channel_number++) {
              if (WAVEBIRD_CHANNEL_MAP[channel_number] == pairing_state.channel)
                break;
            }

            // Set the new channel
            wavebird_radio_set_channel(channel_number);

            // Fire the pairing callback
            if (pairing_finished_callback)
              pairing_finished_callback(WB_RADIO_PAIRING_SUCCESS, channel_number);

            break;
          }
        }

        packet_held = false;
      }

      // If the qualify timeout has expired, move back to scanning
      if ((RAIL_GetTime() > pairing_state.qualify_timeout))
        start_channel_scanning(false);

      break;

    // Listen for packets on the selected/paired channel
    case WB_RADIO_RX_ACTIVE:
      // Process received packets, if any
      if (packet_held) {
        while (get_oldest_pending_packet(packet_buffer, rail_handle)) {
          // Pass the packet to the packet handler
          if (packet_callback != NULL)
            packet_callback(packet_buffer);
        }

        // Clear the interrupt flag
        packet_held = false;
      } else if (error_code < 0) {
        // Handle errors from the interrupt handler
        if (error_callback != NULL)
          error_callback(error_code);

        // Clear the interrupt flag
        error_code = 0;
      }
      break;

    default:
      break;
  }
}