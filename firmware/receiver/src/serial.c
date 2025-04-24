#include <stddef.h>

#include "em_cmu.h"
#include "em_gpio.h"

#include "board_config.h"

#include "serial.h"

#if defined(SERIAL_USART)
#include "em_usart.h"
#elif defined(SERIAL_EUSART)
#include "em_eusart.h"
#endif

// Newlib _write implementation for printf
int _write(int fd, char *ptr, int len)
{
  for (int i = 0; i < len; i++) {
    serial_putc(ptr[i]);
  }

  return len;
}

// Initialize the UART peripheral for RX and TX
void serial_init(uint32_t baudrate)
{
  // Enable clocks
  CMU_ClockEnable(cmuClock_GPIO, true);

  // Configure GPIO pins
  GPIO_PinModeSet(SERIAL_RXPORT, SERIAL_RXPIN, gpioModeInputPull, 1);
  GPIO_PinModeSet(SERIAL_TXPORT, SERIAL_TXPIN, gpioModePushPull, 1);

#if defined(SERIAL_USART)
  // Enable clocks
  CMU_ClockEnable(SERIAL_USART_CLK, true);

  // Configure USART peripheral, defaults to 115200 bits/s, 8N1
  USART_InitAsync_TypeDef usart_init = USART_INITASYNC_DEFAULT;
  usart_init.enable                  = usartDisable;
  usart_init.baudrate                = baudrate;
  USART_InitAsync(SERIAL_USART, &usart_init);

  // Route USART signals to the correct GPIO pins
  GPIO->USARTROUTE[SERIAL_USART_IDX].ROUTEEN = GPIO_USART_ROUTEEN_RXPEN | GPIO_USART_ROUTEEN_TXPEN;
  GPIO->USARTROUTE[SERIAL_USART_IDX].RXROUTE =
      (SERIAL_RXPORT << _GPIO_USART_RXROUTE_PORT_SHIFT) | (SERIAL_RXPIN << _GPIO_USART_RXROUTE_PIN_SHIFT);
  GPIO->USARTROUTE[SERIAL_USART_IDX].TXROUTE =
      (SERIAL_TXPORT << _GPIO_USART_TXROUTE_PORT_SHIFT) | (SERIAL_TXPIN << _GPIO_USART_TXROUTE_PIN_SHIFT);

  // Enable EUSART
  USART_Enable(SERIAL_USART, usartEnable);
#elif defined(SERIAL_EUSART)
  // Enable clocks
  CMU_ClockEnable(SERIAL_EUSART_CLK, true);

  // Configure EUSART peripheral, defaults to 115200 bits/s, 8N1
  EUSART_UartInit_TypeDef eusart_init = EUSART_UART_INIT_DEFAULT_HF;
  eusart_init.enable                  = eusartDisable;
  eusart_init.baudrate                = baudrate;
  EUSART_UartInitHf(SERIAL_EUSART, &eusart_init);

  // Route USART signals to the correct GPIO pins
  GPIO->EUSARTROUTE[SERIAL_EUSART_IDX].ROUTEEN = GPIO_EUSART_ROUTEEN_RXPEN | GPIO_EUSART_ROUTEEN_TXPEN;
  GPIO->EUSARTROUTE[SERIAL_EUSART_IDX].RXROUTE =
      (SERIAL_RXPORT << _GPIO_EUSART_RXROUTE_PORT_SHIFT) | (SERIAL_RXPIN << _GPIO_EUSART_RXROUTE_PIN_SHIFT);
  GPIO->EUSARTROUTE[SERIAL_EUSART_IDX].TXROUTE =
      (SERIAL_TXPORT << _GPIO_EUSART_TXROUTE_PORT_SHIFT) | (SERIAL_TXPIN << _GPIO_EUSART_TXROUTE_PIN_SHIFT);

  // Enable EUSART
  EUSART_Enable(SERIAL_EUSART, eusartEnable);
#endif
}

char serial_getc()
{
#if defined(SERIAL_USART)
  return USART_Rx(SERIAL_USART);
#elif defined(SERIAL_EUSART)
  return EUSART_Rx(SERIAL_EUSART);
#endif
}

void serial_putc(char c)
{
#if defined(SERIAL_USART)
  USART_Tx(SERIAL_USART, c);
#elif defined(SERIAL_EUSART)
  EUSART_Tx(SERIAL_EUSART, c);
#endif
}