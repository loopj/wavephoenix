#pragma once

// Board config
#define HFXO_FREQ         38400000
#define HFXO_CTUNE        72

// GPIO config
#define SI_DATA_PORT      gpioPortA
#define SI_DATA_PIN       2

// Serial debug config
#define SERIAL_USART      USART1
#define SERIAL_USART_CLK  cmuClock_USART1
#define SERIAL_USART_IDX  1
#define SERIAL_RXPORT     gpioPortA
#define SERIAL_RXPIN      6
#define SERIAL_TXPORT     gpioPortA
#define SERIAL_TXPIN      5

// Pair button
#define HAS_PAIR_BTN      1
#define PAIR_BTN_PORT     gpioPortB
#define PAIR_BTN_PIN      0

// Status LED
#define HAS_STATUS_LED    1
#define STATUS_LED_PORT   gpioPortC
#define STATUS_LED_PIN    1
#define STATUS_LED_INVERT 0