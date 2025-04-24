#include <stddef.h>

#include "channel_wheel.h"

#include "gpiointerrupt.h"

static void change_handler(uint8_t intNo, void *ctx)
{
  (void)intNo;

  struct channel_wheel *channel_wheel = (struct channel_wheel *)ctx;
  if (channel_wheel->change_callback)
    channel_wheel->change_callback(channel_wheel, channel_wheel_get_value(channel_wheel));
}

void channel_wheel_init(struct channel_wheel *channel_wheel, uint8_t bit0_port, uint8_t bit0_pin, uint8_t bit1_port,
                        uint8_t bit1_pin, uint8_t bit2_port, uint8_t bit2_pin, uint8_t bit3_port, uint8_t bit3_pin)
{
  // Save the GPIOs
  channel_wheel->bit0_pin  = bit0_pin;
  channel_wheel->bit0_port = bit0_port;
  channel_wheel->bit1_pin  = bit1_pin;
  channel_wheel->bit1_port = bit1_port;
  channel_wheel->bit2_pin  = bit2_pin;
  channel_wheel->bit2_port = bit2_port;
  channel_wheel->bit3_pin  = bit3_pin;
  channel_wheel->bit3_port = bit3_port;

  // Set the GPIOs to input with pull-up and filter
  GPIO_PinModeSet(bit0_port, bit0_pin, gpioModeInputPullFilter, 1);
  GPIO_PinModeSet(bit1_port, bit1_pin, gpioModeInputPullFilter, 1);
  GPIO_PinModeSet(bit2_port, bit2_pin, gpioModeInputPullFilter, 1);
  GPIO_PinModeSet(bit3_port, bit3_pin, gpioModeInputPullFilter, 1);

  channel_wheel->change_callback = NULL;
}

void channel_wheel_set_change_callback(struct channel_wheel *channel_wheel, channel_wheel_change_callback_t callback)
{
  channel_wheel->change_callback = callback;

  // Initialize GPIOINT driver
  GPIOINT_Init();

  // Register the interrupt handlers
  GPIOINT_CallbackRegisterExt(channel_wheel->bit0_pin, change_handler, channel_wheel);
  GPIOINT_CallbackRegisterExt(channel_wheel->bit1_pin, change_handler, channel_wheel);
  GPIOINT_CallbackRegisterExt(channel_wheel->bit2_pin, change_handler, channel_wheel);
  GPIOINT_CallbackRegisterExt(channel_wheel->bit3_pin, change_handler, channel_wheel);

  // Configure the interrupts to trigger on both rising and falling edges
  GPIO_ExtIntConfig(channel_wheel->bit0_port, channel_wheel->bit0_pin, channel_wheel->bit0_pin, true, true, true);
  GPIO_ExtIntConfig(channel_wheel->bit1_port, channel_wheel->bit1_pin, channel_wheel->bit1_pin, true, true, true);
  GPIO_ExtIntConfig(channel_wheel->bit2_port, channel_wheel->bit2_pin, channel_wheel->bit2_pin, true, true, true);
  GPIO_ExtIntConfig(channel_wheel->bit3_port, channel_wheel->bit3_pin, channel_wheel->bit3_pin, true, true, true);
}

uint8_t channel_wheel_get_value(struct channel_wheel *channel_wheel)
{
  uint8_t value = 0;

  value |= (!GPIO_PinInGet(channel_wheel->bit0_port, channel_wheel->bit0_pin)) << 0;
  value |= (!GPIO_PinInGet(channel_wheel->bit1_port, channel_wheel->bit1_pin)) << 1;
  value |= (!GPIO_PinInGet(channel_wheel->bit2_port, channel_wheel->bit2_pin)) << 2;
  value |= (!GPIO_PinInGet(channel_wheel->bit3_port, channel_wheel->bit3_pin)) << 3;

  return value;
}