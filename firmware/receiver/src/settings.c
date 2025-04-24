#include <string.h>

#include "em_msc.h"

#include "settings.h"

// User data base address
#define USERDATA  ((uint32_t *) USERDATA_BASE)

static uint32_t expected_signature;

void settings_init(void *settings, size_t size, uint32_t signature, const void *defaults)
{
  uint32_t stored_signature = *USERDATA;
  if (stored_signature == signature) {
    // Load the settings from USERDATA
    memcpy(settings, (void *)(USERDATA + 1), size);
  } else {
    // Initialize the settings with defaults
    memcpy(settings, defaults, size);
    settings_save(settings, size);
  }

  expected_signature = signature;
}

void settings_save(void *settings, size_t size)
{
  MSC_Init();
  MSC_ErasePage(USERDATA);
  MSC_WriteWord(USERDATA, &expected_signature, 4);
  MSC_WriteWord(USERDATA + 1, settings, size);
  MSC_Deinit();
}