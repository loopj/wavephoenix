#pragma once

#include <stdint.h>

// App version components
static const uint8_t VERSION_MAJOR = 0;
static const uint8_t VERSION_MINOR = 9;
static const uint8_t VERSION_PATCH = 0;

// Full version number
static const uint32_t VERSION_FULL = (VERSION_MAJOR << 24) | (VERSION_MINOR << 16) | (VERSION_PATCH << 8);