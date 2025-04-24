#pragma once

// Board config
#define HFXO_FREQ         38400000
#define HFXO_CTUNE        121

// GPIO config
#define SI_DATA_PORT      gpioPortD
#define SI_DATA_PIN       3

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
#define PAIR_BTN_PORT     gpioPortC
#define PAIR_BTN_PIN      7

// Status LED
#define HAS_STATUS_LED    1
#define STATUS_LED_PORT   gpioPortA
#define STATUS_LED_PIN    4
#define STATUS_LED_INVERT 0