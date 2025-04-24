// Required application properties for the Gecko bootloader

#include "application_properties.h"
#include "version.h"

// Application UUID for WavePhoenix receiver firmware
#define PRODUCT_ID {0xcb, 0x39, 0xea, 0xcc, 0x71, 0x90, 0x44, 0x35, 0x8f, 0x77, 0xfc, 0xed, 0x4d, 0x0b, 0x96, 0xeb}

const ApplicationProperties_t sl_app_properties = {
    .magic             = APPLICATION_PROPERTIES_MAGIC,
    .structVersion     = APPLICATION_PROPERTIES_VERSION,
    .signatureType     = APPLICATION_SIGNATURE_NONE,
    .signatureLocation = 0,
    .app =
        {
            .type         = APPLICATION_TYPE_MCU,
            .version      = VERSION_FULL,
            .capabilities = 0,
            .productId    = PRODUCT_ID,
        },
};