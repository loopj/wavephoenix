#include <stddef.h>

#include "em_cmu.h"
#include "em_gpio.h"
#include "em_ldma.h"
#include "em_timer.h"
#include "em_usart.h"

#include "dmadrv.h"

#include "si/device/commands.h"

#ifdef __ZEPHYR__
#include <zephyr/irq.h>
#endif

// RX TIMER peripheral configuration
#ifndef SI_RX_TIMER_IDX
#define SI_RX_TIMER_IDX         0
#endif

#if SI_RX_TIMER_IDX == 0
#define SI_RX_TIMER             TIMER0
#define SI_RX_TIMER_CLK         cmuClock_TIMER0
#define SI_RX_LDMA_SIGNAL       ldmaPeripheralSignal_TIMER0_CC0
#elif SI_RX_TIMER_IDX == 1
#define SI_RX_TIMER             TIMER1
#define SI_RX_TIMER_CLK         cmuClock_TIMER1
#define SI_RX_LDMA_SIGNAL       ldmaPeripheralSignal_TIMER1_CC0
#elif SI_RX_TIMER_IDX == 2
#define SI_RX_TIMER             TIMER2
#define SI_RX_TIMER_CLK         cmuClock_TIMER2
#define SI_RX_LDMA_SIGNAL       ldmaPeripheralSignal_TIMER2_CC0
#elif SI_RX_TIMER_IDX == 3
#define SI_RX_TIMER             TIMER3
#define SI_RX_TIMER_CLK         cmuClock_TIMER3
#define SI_RX_LDMA_SIGNAL       ldmaPeripheralSignal_TIMER3_CC0
#else
#error "Invalid SI_RX_TIMER_IDX value"
#endif

// TX USART peripheral configuration
#ifndef SI_TX_USART_IDX
#define SI_TX_USART_IDX         0
#endif

#if SI_TX_USART_IDX == 0
#define SI_TX_USART             USART0
#define SI_TX_USART_CLK         cmuClock_USART0
#define SI_TX_USART_IRQn        USART0_TX_IRQn
#define SI_TX_USART_IRQHandler  USART0_TX_IRQHandler
#define SI_TX_LDMA_SIGNAL       ldmaPeripheralSignal_USART0_TXBL
#elif SI_TX_USART_IDX == 1
#define SI_TX_USART             USART1
#define SI_TX_USART_CLK         cmuClock_USART1
#define SI_TX_USART_IRQn        USART1_TX_IRQn
#define SI_TX_USART_IRQHandler  USART1_TX_IRQHandler
#define SI_TX_LDMA_SIGNAL       ldmaPeripheralSignal_USART1_TXBL
#else
#error "Invalid SI_TX_USART_IDX value"
#endif

// Number of chips per bit for the line coding
#define CHIPS_PER_BIT           4

// Line coding
#define BIT_0                   0b0001
#define BIT_1                   0b0111
#define DEVICE_STOP             0b00111111
#define HOST_STOP               0b01111111

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
  uint8_t max_length;
  si_byte_cb_t byte_callback;
  si_complete_cb_t complete_callback;
} si_xfer;

// RX LDMA configuration
static LDMA_TransferCfg_t ldma_rx_config       = LDMA_TRANSFER_CFG_PERIPHERAL(SI_RX_LDMA_SIGNAL);
static LDMA_Descriptor_t ldma_rx_descriptors[] = {
    LDMA_DESCRIPTOR_LINKREL_P2M_WORD(&SI_RX_TIMER->CC[0].ICF, &rx_edge_timings[0], RX_BUFFER_SIZE, 1),
    LDMA_DESCRIPTOR_LINKREL_P2M_WORD(&SI_RX_TIMER->CC[0].ICF, &rx_edge_timings[1], RX_BUFFER_SIZE, -1),
};

// TX LDMA configuration
static LDMA_TransferCfg_t ldma_tx_config       = LDMA_TRANSFER_CFG_PERIPHERAL(SI_TX_LDMA_SIGNAL);
static LDMA_Descriptor_t ldma_tx_descriptors[] = {
    LDMA_DESCRIPTOR_SINGLE_M2P_BYTE(tx_buffer, &(SI_TX_USART->TXDATA), 1),
};

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

  // Adjust size of rx transfer size to half-word
  ldma_rx_descriptors[0].xfer.size = ldmaCtrlSizeHalf;
  ldma_rx_descriptors[1].xfer.size = ldmaCtrlSizeHalf;

  // Initialize SI RX and TX
  init_rx(port, pin, rx_freq);
  init_tx(port, pin, tx_freq);

  // Save the SI configuration
  si_data_port = port;
  si_data_pin  = pin;
  si_mode      = mode;
}

void si_write_bytes(const uint8_t *bytes, uint8_t length, si_complete_cb_t callback)
{
  // Save the transfer state
  si_xfer.data              = (uint8_t *)bytes;
  si_xfer.max_length        = length;
  si_xfer.byte_callback     = NULL;
  si_xfer.complete_callback = callback;

  // Convert the bytes to appropriate line coding and add to the buffer
  uint8_t *buf_ptr = tx_buffer;
  for (int i = 0; i < length; i++)
    buf_ptr = encode_byte(buf_ptr, bytes[i]);

  // Add the stop bit
  *buf_ptr++ = (si_mode == SI_MODE_HOST ? HOST_STOP : DEVICE_STOP);

  // Set the transfer count (xferCnt expects the number of transfer minus one)
  ldma_tx_descriptors[0].xfer.xferCnt = (length * CHIPS_PER_BIT + 1) - 1;

  // Start the DMA transfer
  DMADRV_LdmaStartTransfer(tx_dma_channel, &ldma_tx_config, ldma_tx_descriptors, NULL, NULL);
}

void si_read_bytes(uint8_t *buffer, uint8_t max_length, si_byte_cb_t byte_callback, si_complete_cb_t complete_callback)
{
  // Save the transfer state
  si_xfer.data              = buffer;
  si_xfer.max_length        = max_length;
  si_xfer.byte_callback     = byte_callback;
  si_xfer.complete_callback = complete_callback;

  // Clear the RX buffer
  while (TIMER_CaptureGet(SI_RX_TIMER, 0))
    ;

  // Start the input capture timer
  TIMER_Enable(SI_RX_TIMER, true);

  // Start the LDMA transfer
  DMADRV_LdmaStartTransfer(rx_dma_channel, &ldma_rx_config, ldma_rx_descriptors, ldma_callback_rx, NULL);
}

void si_await_bus_idle(void)
{
  // Start the timer
  TIMER_Enable(SI_RX_TIMER, true);

  while (1) {
    // Wait for the line to go high
    // TODO: Add a timeout
    while (GPIO_PinInGet(si_data_port, si_data_pin) == 0)
      ;

    // Start timing the bus idle period
    TIMER_CounterSet(SI_RX_TIMER, 0);

    // Wait for either the bus idle period to elapse or line to go low
    // TODO: Add a timeout
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
  NVIC_SetPriority(LDMA_IRQn, 0);

  // Connect the LDMA interrupt if building for Zephyr
#ifdef __ZEPHYR__
  IRQ_CONNECT(LDMA_IRQn, 0, LDMA_IRQHandler, NULL, 0);
#endif
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

  // Tri-state the USART TX output
  SI_TX_USART->CTRL_SET = USART_CTRL_AUTOTRI;

  // Route USART output to the SI GPIO
  GPIO->USARTROUTE[SI_TX_USART_IDX].ROUTEEN = GPIO_USART_ROUTEEN_TXPEN;
  GPIO->USARTROUTE[SI_TX_USART_IDX].TXROUTE =
      (port << _GPIO_USART_TXROUTE_PORT_SHIFT) | (pin << _GPIO_USART_TXROUTE_PIN_SHIFT);

  // Enable USART TX complete interrupts
  USART_IntEnable(SI_TX_USART, USART_IF_TXC);
  NVIC_EnableIRQ(SI_TX_USART_IRQn);

  // Connect the USART interrupt if building for Zephyr
#ifdef __ZEPHYR__
  IRQ_CONNECT(SI_TX_USART_IRQn, 0, SI_TX_USART_IRQHandler, NULL, 0);
#endif
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

  // Call the byte callback if provided
  bool continue_transfer = true;
  if (si_xfer.byte_callback) {
    continue_transfer = si_xfer.byte_callback(si_xfer.data[byte_idx], byte_idx);
  }

  // If we have reached the maximum length, stop the transfer
  if (!continue_transfer || byte_idx == si_xfer.max_length - 1) {
    // Stop the timer
    TIMER_Enable(SI_RX_TIMER, false);

    // Call the complete callback if provided
    if (si_xfer.complete_callback)
      si_xfer.complete_callback(byte_idx + 1);

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

  // Call the transfer callback with the number of bytes written
  if (si_xfer.complete_callback)
    si_xfer.complete_callback(si_xfer.max_length);
}