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
#define SI_TX_USART             USART0
#define SI_TX_USART_IDX         0
#define SI_TX_USART_CLK         cmuClock_USART0
#define SI_TX_USART_IRQn        USART0_TX_IRQn
#define SI_TX_USART_IRQHandler  USART0_TX_IRQHandler
#define SI_TX_LDMA_PERIPHERAL   ldmaPeripheralSignal_USART0_TXBL

// Number of chips per bit for the line coding
#define CHIPS_PER_BIT           4

// Line coding (inverted, since we're inverting the USART output)
#define BIT_0                   0b1110
#define BIT_1                   0b1000
#define DEVICE_STOP             0b1100
#define HOST_STOP               0b1000

// SI bus idle period (in microseconds)
#define BUS_IDLE_US             100

// RX buffer size (16 edges per byte)
#define RX_BUFFER_SIZE          16

// TX buffer size (4 chips per bit, extra byte for stop bit)
#define TX_BUFFER_SIZE          (SI_BLOCK_SIZE * CHIPS_PER_BIT + 1)

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
static uint8_t tx_buffer[TX_BUFFER_SIZE];
static unsigned int tx_dma_channel;

// Transfer state
static struct {
  uint8_t *data;
  uint8_t length;
  si_callback_fn callback;
} si_xfer;

static void init_rx(uint8_t port, uint8_t pin, uint32_t freq);
static void init_tx(uint8_t port, uint8_t pin, uint32_t freq);
static uint8_t *encode_byte(uint8_t *dest, uint8_t src);
static void decode_edge_timings(uint8_t *dest, uint16_t *src);
static bool ldma_callback_rx(unsigned int chan, unsigned int iteration, void *user_data);

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
  init_tx(port, pin, tx_freq);

  // Save the SI configuration
  si_data_port = port;
  si_data_pin  = pin;
  si_mode      = mode;
}

void si_write_bytes(const uint8_t *bytes, uint8_t length, si_callback_fn callback)
{
  // Save the transfer state
  si_xfer.data     = (uint8_t *)bytes;
  si_xfer.length   = length;
  si_xfer.callback = callback;

  // Convert the bytes to appropriate line coding and add to the buffer
  uint8_t *buf_ptr = tx_buffer;
  for (int i = 0; i < length; i++)
    buf_ptr = encode_byte(buf_ptr, bytes[i]);

  // Add the stop bit
  buf_ptr[0] = (si_mode == SI_MODE_HOST ? HOST_STOP : DEVICE_STOP) << 4;

  // Start the DMA transfer
  DMADRV_MemoryPeripheral(tx_dma_channel, SI_TX_LDMA_PERIPHERAL, (void *)&(SI_TX_USART->TXDATA), tx_buffer, true,
                          length * CHIPS_PER_BIT + 1, dmadrvDataSize1, NULL, NULL);
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
  DMADRV_PeripheralMemoryPingPong(rx_dma_channel, SI_RX_LDMA_PERIPHERAL, rx_edge_timings[0], rx_edge_timings[1],
                                  (void *)&(SI_RX_TIMER->CC[0].ICF), true, 16, dmadrvDataSize2, ldma_callback_rx, NULL);
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
  NVIC_SetPriority(LDMA_IRQn, CORE_INTERRUPT_HIGHEST_PRIORITY);
}

// Initialize for SI data transmission
static void init_tx(uint8_t port, uint8_t pin, uint32_t freq)
{
  // Allocate a DMA channel
  DMADRV_AllocateChannel(&tx_dma_channel, NULL);

  // Enable clocks
  CMU_ClockEnable(SI_TX_USART_CLK, true);

  // Initialize USART
  USART_InitSync_TypeDef usartConfig = USART_INITSYNC_DEFAULT;
  usartConfig.baudrate               = freq * CHIPS_PER_BIT;
  usartConfig.msbf                   = true;
  USART_InitSync(SI_TX_USART, &usartConfig);

  // Invert the TX output so we have an active-low signal
  SI_TX_USART->CTRL_SET = USART_CTRL_TXINV;

  // Route USART output to the SI GPIO
  GPIO->USARTROUTE[SI_TX_USART_IDX].ROUTEEN = GPIO_USART_ROUTEEN_TXPEN;
  GPIO->USARTROUTE[SI_TX_USART_IDX].TXROUTE =
      (port << _GPIO_USART_TXROUTE_PORT_SHIFT) | (pin << _GPIO_USART_TXROUTE_PIN_SHIFT);

  // Enable USART TX complete interrupts
  USART_IntEnable(SI_TX_USART, USART_IF_TXC);
  NVIC_EnableIRQ(SI_TX_USART_IRQn);
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

// Convert a byte to the appropriate line coding for transmission
static uint8_t *encode_byte(uint8_t *dest, uint8_t src)
{
  uint8_t bit_7 = (src & 0x80) ? BIT_1 << 4 : BIT_0 << 4;
  uint8_t bit_6 = (src & 0x40) ? BIT_1 : BIT_0;
  *dest++       = bit_7 | bit_6;

  uint8_t bit_5 = (src & 0x20) ? BIT_1 << 4 : BIT_0 << 4;
  uint8_t bit_4 = (src & 0x10) ? BIT_1 : BIT_0;
  *dest++       = bit_5 | bit_4;

  uint8_t bit_3 = (src & 0x08) ? BIT_1 << 4 : BIT_0 << 4;
  uint8_t bit_2 = (src & 0x04) ? BIT_1 : BIT_0;
  *dest++       = bit_3 | bit_2;

  uint8_t bit_1 = (src & 0x02) ? BIT_1 << 4 : BIT_0 << 4;
  uint8_t bit_0 = (src & 0x01) ? BIT_1 : BIT_0;
  *dest++       = bit_1 | bit_0;

  return dest;
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
      TIMER_Enable(SI_RX_TIMER, false);

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

// USART TX complete interrupt handler
void SI_TX_USART_IRQHandler()
{
  // Clear the interrupt flags
  uint32_t flags = USART_IntGet(SI_TX_USART);
  USART_IntClear(SI_TX_USART, flags);

  // Call the transfer callback if one is set
  if (si_xfer.callback)
    si_xfer.callback(0);
}