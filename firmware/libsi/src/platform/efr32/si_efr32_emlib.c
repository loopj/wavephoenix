#include <stddef.h>

#include "em_cmu.h"
#include "em_gpio.h"
#include "em_ldma.h"
#include "em_timer.h"
#include "em_usart.h"

#include "dmadrv.h"

#include "si/commands.h"

// RX peripheral configuration
#define SI_RX_TIMER             TIMER0
#define SI_RX_TIMER_IDX         0
#define SI_RX_TIMER_CLK         cmuClock_TIMER0
#define SI_RX_LDMA_PERIPHERAL   ldmaPeripheralSignal_TIMER0_CC0

// TX peripheral configuration
#define SI_TX_TIMER             TIMER1
#define SI_TX_TIMER_IDX         1
#define SI_TX_TIMER_CLK         cmuClock_TIMER1
#define SI_TX_LDMA_PERIPHERAL   ldmaPeripheralSignal_TIMER1_UFOF

// SI bus idle period (in microseconds)
#define BUS_IDLE_US             100

// RX buffer size (16 edges per byte)
#define RX_BUFFER_SIZE          16

// TX buffer size (One word per bit, 8 bits per byte)
#define TX_BUFFER_SIZE          SI_BLOCK_SIZE * 8

// SI configuration
static uint8_t si_data_port;
static uint8_t si_data_pin;
static bool si_mode;

// RX state
static uint16_t rx_edge_timings[2][RX_BUFFER_SIZE];
static uint16_t rx_pulse_period_half;
static uint16_t rx_bus_idle_period;
static unsigned int rx_dma_channel;

// TX State
static uint16_t tx_buffer[TX_BUFFER_SIZE];
static uint16_t tx_pulse_period;
static uint16_t tx_pulse_period_inv_0;
static uint16_t tx_pulse_period_inv_1;
static uint16_t tx_pulse_period_inv_stop;
static unsigned int tx_dma_channel;

static const uint16_t dummy_period = 0;

// Transfer state
static struct {
  uint8_t *data;
  uint8_t length;
  si_callback_fn callback;
} si_xfer;

// RX LDMA configuration
static LDMA_TransferCfg_t ldma_rx_config       = LDMA_TRANSFER_CFG_PERIPHERAL(SI_RX_LDMA_PERIPHERAL);
static LDMA_Descriptor_t ldma_rx_descriptors[] = {
    LDMA_DESCRIPTOR_LINKREL_P2M_WORD(&SI_RX_TIMER->CC[0].ICF, &rx_edge_timings[0], RX_BUFFER_SIZE, 1),
    LDMA_DESCRIPTOR_LINKREL_P2M_WORD(&SI_RX_TIMER->CC[0].ICF, &rx_edge_timings[1], RX_BUFFER_SIZE, -1),
};

// TX LDMA configuration
static LDMA_TransferCfg_t ldma_tx_config       = LDMA_TRANSFER_CFG_PERIPHERAL(ldmaPeripheralSignal_TIMER1_CC0);
static LDMA_Descriptor_t ldma_tx_descriptors[] = {
    // Shift out the data pulses
    LDMA_DESCRIPTOR_LINKREL_M2P_BYTE(tx_buffer + 1, &(TIMER1->CC[0].OCB), 1, 1),

    // Shift out the stop bit
    LDMA_DESCRIPTOR_LINKREL_M2P_BYTE(&tx_pulse_period_inv_stop, &(TIMER1->CC[0].OCB), 1, 1),

    // Shift out a dummy period
    LDMA_DESCRIPTOR_LINKREL_M2P_BYTE(&dummy_period, &(TIMER1->CC[0].OCB), 1, 1),

    // Stop the timer
    LDMA_DESCRIPTOR_LINKREL_WRITE(TIMER_CMD_STOP, &(TIMER1->CMD), 1),

    // Reset the timer counter
    LDMA_DESCRIPTOR_SINGLE_WRITE(0, &(TIMER1->CNT)),
};

static void init_rx(uint8_t port, uint8_t pin, uint32_t freq);
static void init_tx(uint8_t port, uint8_t pin, uint32_t freq, uint8_t mode);
static void decode_edge_timings(uint8_t *dest, uint16_t *src);
static bool ldma_callback_rx(unsigned int chan, unsigned int iteration, void *user_data);
static bool ldma_callback_tx(unsigned int chan, unsigned int iteration, void *user_data);

void si_init(uint8_t port, uint8_t pin, uint8_t mode, uint32_t rx_freq, uint32_t tx_freq)
{
  // Initialize LDMA
  DMADRV_Init();

  // Use the HFXO as the TIMER clock source
  CMU_ClockSelectSet(cmuClock_EM01GRPACLK, cmuSelect_HFXO);

  // Enable clocks
  CMU_ClockEnable(cmuClock_GPIO, true);

  // Set the SI data line as open-drain output
  GPIO_PinModeSet(port, pin, gpioModeWiredAnd, 1);

  // Initialize SI RX and TX
  init_rx(port, pin, rx_freq);
  init_tx(port, pin, tx_freq, mode);

  // Save the SI configuration
  si_data_port = port;
  si_data_pin  = pin;
  si_mode      = mode;
}

void si_write_bytes(const uint8_t *bytes, uint8_t length, si_callback_fn callback)
{
  // Save the transfer state
  si_xfer.callback = callback;

  // Convert the bytes to appropriate line coding and add to the buffer
  uint16_t *buf_ptr = tx_buffer;
  for (int i = 0; i < length; i++) {
    uint8_t byte = bytes[i];
    *buf_ptr++   = (byte & 0x80) ? tx_pulse_period_inv_1 : tx_pulse_period_inv_0;
    *buf_ptr++   = (byte & 0x40) ? tx_pulse_period_inv_1 : tx_pulse_period_inv_0;
    *buf_ptr++   = (byte & 0x20) ? tx_pulse_period_inv_1 : tx_pulse_period_inv_0;
    *buf_ptr++   = (byte & 0x10) ? tx_pulse_period_inv_1 : tx_pulse_period_inv_0;
    *buf_ptr++   = (byte & 0x08) ? tx_pulse_period_inv_1 : tx_pulse_period_inv_0;
    *buf_ptr++   = (byte & 0x04) ? tx_pulse_period_inv_1 : tx_pulse_period_inv_0;
    *buf_ptr++   = (byte & 0x02) ? tx_pulse_period_inv_1 : tx_pulse_period_inv_0;
    *buf_ptr++   = (byte & 0x01) ? tx_pulse_period_inv_1 : tx_pulse_period_inv_0;
  }

  // Set the transfer count (xferCnt expects the number of transfer minus one)
  ldma_tx_descriptors[0].xfer.xferCnt = (length * 8 - 1) - 1;

  // Pre-load the TX timer with the first pulse period, and start the timer
  SI_TX_TIMER->CC[0].OCB = tx_buffer[0];
  SI_TX_TIMER->CMD       = TIMER_CMD_START;

  // Start the DMA transfer
  DMADRV_LdmaStartTransfer(tx_dma_channel, &ldma_tx_config, ldma_tx_descriptors, ldma_callback_tx, NULL);
}

void si_read_bytes(uint8_t *buffer, uint8_t length, si_callback_fn callback)
{
  // Save the transfer state
  si_xfer.data     = buffer;
  si_xfer.length   = length;
  si_xfer.callback = callback;

  // Clear the RX buffer
  while (TIMER_CaptureGet(SI_RX_TIMER, 0))
    ;

  // Start the input capture timer
  TIMER_Enable(SI_RX_TIMER, true);

  // Start the LDMA transfer
  DMADRV_LdmaStartTransfer(rx_dma_channel, &ldma_rx_config, ldma_rx_descriptors, ldma_callback_rx, NULL);
}

void si_read_command(uint8_t *buffer, si_callback_fn callback)
{
  si_read_bytes(buffer, 0, callback);
}

void si_await_bus_idle(void)
{
  // Start the timer
  TIMER_Enable(SI_RX_TIMER, true);

  while (1) {
    // Wait for the line to go high
    while (GPIO_PinInGet(si_data_port, si_data_pin) == 0)
      ;

    // Start timing the bus idle period
    TIMER_CounterSet(SI_RX_TIMER, 0);

    // Wait for either the bus idle period to elapse or line to go low
    while (GPIO_PinInGet(si_data_port, si_data_pin) == 1) {
      if (TIMER_CounterGet(SI_RX_TIMER) >= rx_bus_idle_period)
        goto idle_detected;
    }
  }

idle_detected:
  // Stop the timer
  TIMER_Enable(SI_RX_TIMER, false);
}

// Initialize for SI pulse capture
static void init_rx(uint8_t port, uint8_t pin, uint32_t freq)
{
  // Allocate a DMA channel
  DMADRV_AllocateChannel(&rx_dma_channel, NULL);

  // Set up the timings for rx pulses
  uint32_t rx_timer_freq = CMU_ClockFreqGet(SI_RX_TIMER_CLK);
  rx_pulse_period_half   = (rx_timer_freq / freq) / 2;
  rx_bus_idle_period     = rx_timer_freq / 1000000UL * BUS_IDLE_US;

  // Adjust size of rx transfer size to half-word
  ldma_rx_descriptors[0].xfer.size = ldmaCtrlSizeHalf;
  ldma_rx_descriptors[1].xfer.size = ldmaCtrlSizeHalf;

  // Enable clocks
  CMU_ClockEnable(SI_RX_TIMER_CLK, true);

  // Initialize timer
  TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
  timerInit.enable             = false;
  TIMER_Init(SI_RX_TIMER, &timerInit);

  // Configure CC0 for pulse width capture
  TIMER_InitCC_TypeDef timerCCInit = TIMER_INITCC_DEFAULT;
  timerCCInit.edge                 = timerEdgeBoth;
  timerCCInit.mode                 = timerCCModeCapture;
  TIMER_InitCC(SI_RX_TIMER, 0, &timerCCInit);

  // Route timer capture input to the SI GPIO
  GPIO->TIMERROUTE[SI_RX_TIMER_IDX].ROUTEEN = GPIO_TIMER_ROUTEEN_CC0PEN;
  GPIO->TIMERROUTE[SI_RX_TIMER_IDX].CC0ROUTE =
      (port << _GPIO_TIMER_CC0ROUTE_PORT_SHIFT) | (pin << _GPIO_TIMER_CC0ROUTE_PIN_SHIFT);

  // Set LDMA interrupts as high priority, since we need to reply immediately on completed RX
  NVIC_SetPriority(LDMA_IRQn, 0);
}

// Initialize for SI data transmission
static void init_tx(uint8_t port, uint8_t pin, uint32_t freq, uint8_t mode)
{
  // Allocate a DMA channel
  DMADRV_AllocateChannel(&tx_dma_channel, NULL);

  // Set up the timings for tx pulses
  uint32_t tx_timer_freq = CMU_ClockFreqGet(SI_TX_TIMER_CLK);
  tx_pulse_period        = (tx_timer_freq / freq);
  tx_pulse_period_inv_0  = tx_pulse_period * 3 / 4;
  tx_pulse_period_inv_1  = tx_pulse_period * 1 / 4;
  if (mode == SI_MODE_HOST) {
    tx_pulse_period_inv_stop = tx_pulse_period_inv_1;
  } else {
    tx_pulse_period_inv_stop = tx_pulse_period * 2 / 4;
  }

  // Adjust size of tx transfer size to half-word
  ldma_tx_descriptors[0].xfer.size    = ldmaCtrlSizeHalf;
  ldma_tx_descriptors[1].xfer.size    = ldmaCtrlSizeHalf;
  ldma_tx_descriptors[2].xfer.size    = ldmaCtrlSizeHalf;
  ldma_tx_descriptors[0].xfer.doneIfs = false;
  ldma_tx_descriptors[1].xfer.doneIfs = false;
  ldma_tx_descriptors[2].xfer.doneIfs = false;
  ldma_tx_descriptors[3].xfer.doneIfs = false;
  ldma_tx_descriptors[4].xfer.doneIfs = true;

  // Enable clocks
  CMU_ClockEnable(SI_TX_TIMER_CLK, true);

  // Initialize TIMER
  TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
  timerInit.enable             = false;
  timerInit.dmaClrAct          = true;
  TIMER_Init(SI_TX_TIMER, &timerInit);
  TIMER_TopSet(SI_TX_TIMER, tx_pulse_period - 1);

  // Configure CC0 for PWM output
  TIMER_InitCC_TypeDef timerCCInit = TIMER_INITCC_DEFAULT;
  timerCCInit.mode                 = timerCCModePWM;
  timerCCInit.outInvert            = true;
  TIMER_InitCC(SI_TX_TIMER, 0, &timerCCInit);

  // Route TIMER output to the SI GPIO
  GPIO->TIMERROUTE[SI_TX_TIMER_IDX].ROUTEEN = GPIO_TIMER_ROUTEEN_CC0PEN;
  GPIO->TIMERROUTE[SI_TX_TIMER_IDX].CC0ROUTE =
      (port << _GPIO_TIMER_CC0ROUTE_PORT_SHIFT) | (pin << _GPIO_TIMER_CC0ROUTE_PIN_SHIFT);
}

// Process received SI edge timings into a byte
static void decode_edge_timings(uint8_t *dest, uint16_t *src)
{
  // Clear the destination byte
  *dest = 0;

  // Write out the byte, most significant bit first
  for (int i = 7; i >= 0; i--) {
    // Determine how long the SI line was low
    // NOTE: We're explicitly casting back to uint16_t to handle timer overflow
    uint16_t ticks_low = (uint16_t)(src[1] - src[0]);

    // Set the bit based on the low period of the pulse
    *dest |= (ticks_low < rx_pulse_period_half) << i;

    // Move to the next pair of edges
    src += 2;
  }
}

// LDMA callback for RX data capture
static bool ldma_callback_rx(unsigned int chan, unsigned int iteration, void *user_data)
{
  // Iteration count is 1-indexed
  uint8_t byte_idx = iteration - 1;

  // Process the received pulses into the byte buffer
  decode_edge_timings(&si_xfer.data[byte_idx], rx_edge_timings[byte_idx % 2]);

  // If this is the first byte, determine how many bytes are expected
  if (si_xfer.length == 0 && iteration == 1) {
    si_xfer.length = si_command_get_length(si_xfer.data[0]);

    // Unknown command, stop the transfer
    if (si_xfer.length == 0) {
      // Don't clock in any more data
      SI_RX_TIMER->CMD = TIMER_CMD_STOP;

      // Call the transfer callback if one is set
      if (si_xfer.callback)
        si_xfer.callback(-SI_ERR_UNKNOWN_COMMAND);

      // Stop the LDMA chain
      return false;
    }
  }

  // We have all the bytes we expected
  if (iteration == si_xfer.length) {
    // Don't clock in any more data
    TIMER_Enable(SI_RX_TIMER, false);

    // Call the transfer callback if one is set
    if (si_xfer.callback)
      si_xfer.callback(0);

    // Stop the LDMA chain
    return false;
  }

  // Continue the LDMA chain
  return true;
}

static bool ldma_callback_tx(unsigned int chan, unsigned int iteration, void *user_data)
{
  // Call the transfer callback if one is set
  if (si_xfer.callback)
    si_xfer.callback(0);

  return false;
}