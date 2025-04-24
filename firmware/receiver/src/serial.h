#pragma once

void serial_init(uint32_t baudrate);
char serial_getc();
void serial_putc(char c);

#if defined(DEBUG)
#include <stdio.h>
#define DEBUG_PRINT(msg, ...) printf(msg, ##__VA_ARGS__)
#define DEBUG_FLUSH()
#else
#define DEBUG_PRINT(msg, ...)
#endif
