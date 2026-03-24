#pragma once

enum status_led_signal {
  STATUS_LED_OFF = 0,
  STATUS_LED_WIRELESS_ACTIVITY,
  STATUS_LED_PAIRING_ACTIVE,
  STATUS_LED_PAIRING_SUCCESS,
  STATUS_LED_PAIRING_TIMEOUT,
};

void status_led_init(void);
void status_led_set(enum status_led_signal signal);
void status_led_update(void);