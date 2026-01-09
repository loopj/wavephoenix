#include "button.h"

#include <stddef.h>
#include <stdio.h>

#include "em_cmu.h"
#include "em_gpio.h"
#include "em_letimer.h"
#include "gpiointerrupt.h"

#define BUTTON_DEBOUNCE_MS    50
#define BUTTON_LONG_PRESS_MS  2000

static struct button *timer_button = NULL;

static void timer_init()
{
  // Set up clocks
  CMU_ClockSelectSet(cmuClock_EM23GRPACLK, cmuSelect_ULFRCO);
  CMU_ClockEnable(cmuClock_LETIMER0, true);

  // Set up LETIMER0
  LETIMER_Init_TypeDef letimerInit = LETIMER_INIT_DEFAULT;
  letimerInit.enable               = false;
  letimerInit.repMode              = letimerRepeatOneshot;
  LETIMER_Init(LETIMER0, &letimerInit);

  // Enable the LETIMER0 interrupt
  NVIC_EnableIRQ(LETIMER0_IRQn);
  LETIMER_IntEnable(LETIMER0, LETIMER_IEN_UF);
}

static void timer_run(uint32_t ms)
{
  LETIMER_TopSet(LETIMER0, ms);
  LETIMER_RepeatSet(LETIMER0, 0, 1);
  LETIMER_Enable(LETIMER0, true);
}

static void timer_cancel()
{
  LETIMER_Enable(LETIMER0, false);
  LETIMER_CounterSet(LETIMER0, 0);
}

void LETIMER0_IRQHandler(void)
{
  // Clear the interrupt
  LETIMER_IntClear(LETIMER0, LETIMER_IF_UF);

  timer_cancel();

  if (timer_button->button_state == BUTTON_STATE_DEBOUNCING) {
    timer_run(BUTTON_LONG_PRESS_MS - BUTTON_DEBOUNCE_MS);
    timer_button->button_state = BUTTON_STATE_PENDING_LONG_PRESS;

    if (timer_button->long_press_callback != NULL)
      timer_button->press_callback(timer_button);
  } else if (timer_button->button_state == BUTTON_STATE_PENDING_LONG_PRESS) {
    timer_button->button_state = BUTTON_STATE_IDLE;

    if (timer_button->press_callback != NULL)
      timer_button->long_press_callback(timer_button);
  }
}

static void gpio_interrupt(uint8_t intNo, void *ctx)
{
  (void)intNo;

  struct button *button = (struct button *)ctx;

  if (GPIO_PinInGet(button->port, button->pin) == 0) {
    // (Re)start the timer
    timer_run(BUTTON_DEBOUNCE_MS);

    // Save state
    timer_button               = button;
    timer_button->button_state = BUTTON_STATE_DEBOUNCING;
  } else {
    // Button released, stop the timer
    timer_cancel();
  }
}

void button_init(struct button *button, uint8_t port, uint8_t pin)
{
  GPIOINT_Init();
  GPIO_PinModeSet(port, pin, gpioModeInputPullFilter, 1);
  GPIOINT_CallbackRegisterExt(pin, gpio_interrupt, button);
  GPIO_ExtIntConfig(port, pin, pin, true, true, true);

  // Set the button state
  button->port                = port;
  button->pin                 = pin;
  button->button_state        = BUTTON_STATE_IDLE;
  button->press_callback      = NULL;
  button->long_press_callback = NULL;

  // Initialize the press timer
  timer_init();
}

void button_set_press_callback(struct button *button, button_callback_t press_callback)
{
  button->press_callback = press_callback;
}

void button_set_long_press_callback(struct button *button, button_callback_t long_press_callback)
{
  button->long_press_callback = long_press_callback;
}