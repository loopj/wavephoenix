#pragma once

#include "sl_application_type.h"

#include "version.h"

// Type of signature this application is signed with
// Default: APPLICATION_SIGNATURE_NONE(0)
#define SL_APPLICATION_SIGNATURE               0

// Location of the signature
// Default: 0xFFFFFFFF
#define SL_APPLICATION_SIGNATURE_LOCATION      0xFFFFFFFF

// Bitfield representing type of application
#define SL_APPLICATION_TYPE                    APPLICATION_TYPE

// <o SL_APPLICATION_VERSION> Version number for this application
// <0-4294967295:1>
// <i> Default: 1 [0-4294967295]
#define SL_APPLICATION_VERSION                 (VERSION_MAJOR << 24) | (VERSION_MINOR << 16) | (VERSION_PATCH << 8)

// Capabilities of this application
// Default: 0
#define SL_APPLICATION_CAPABILITIES            0

// Product ID of the device for which the application is built
#define SL_APPLICATION_PRODUCT_ID              {0xcb, 0x39, 0xea, 0xcc, 0x71, 0x90, 0x44, 0x35, 0x8f, 0x77, 0xfc, 0xed, 0x4d, 0x0b, 0x96, 0xeb}
