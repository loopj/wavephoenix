#include <em_cmu.h>
#include <em_gpio.h>
#include <em_i2c.h>
#include <em_iadc.h>
#include <em_ldma.h>

#include "gpiointerrupt.h"

#include "local_input.h"
#include "tca9555.h"

// TCA9555 Port 0
#define WPP_LT_PIN          0
#define WPP_DU_PIN          1
#define WPP_DD_PIN          2
#define WPP_DR_PIN          3
#define WPP_DL_PIN          4
#define WPP_Z2_PIN          5
#define WPP_RUMBLE_EN_PIN   6
#define WPP_AUX_PIN         7

// TCA9555 Port 1
#define WPP_A_PIN           0
#define WPP_B_PIN           1
#define WPP_X_PIN           2
#define WPP_Y_PIN           3
#define WPP_Z_PIN           4
#define WPP_S_PIN           5
#define WPP_RT_PIN          6
#define WPP_WIRELESS_EN_PIN 7

#define LDMA_CHANNEL_ADC 2
#define NUM_ANALOG_INPUTS 6

// LDMA descriptor for ADC
static LDMA_Descriptor_t descriptor;

// Latest analog state
static volatile uint32_t adc_state[NUM_ANALOG_INPUTS];

static struct joybus_gc_controller_input input_state;
static local_input_cb_t input_callback = NULL;

static void i2c_init(void)
{
  // Enable clocks to the I2C and GPIO
  CMU_ClockEnable(cmuClock_GPIO, true);
  CMU_ClockEnable(cmuClock_I2C0, true);

  // Using PA5 (SDA) and PA6 (SCL)
  GPIO_PinModeSet(I2C_SCL_PORT, I2C_SCL_PIN, gpioModeWiredAndPullUp, 1);
  GPIO_PinModeSet(I2C_SDA_PORT, I2C_SDA_PIN, gpioModeWiredAndPullUp, 1);

  // Route I2C pins to GPIO
  GPIO->I2CROUTE[0].SDAROUTE =
      (I2C_SDA_PORT << _GPIO_I2C_SDAROUTE_PORT_SHIFT | (I2C_SDA_PIN << _GPIO_I2C_SDAROUTE_PIN_SHIFT));
  GPIO->I2CROUTE[0].SCLROUTE =
      (I2C_SCL_PORT << _GPIO_I2C_SCLROUTE_PORT_SHIFT | (I2C_SCL_PIN << _GPIO_I2C_SCLROUTE_PIN_SHIFT));
  GPIO->I2CROUTE[0].ROUTEEN = GPIO_I2C_ROUTEEN_SDAPEN | GPIO_I2C_ROUTEEN_SCLPEN;

  // Initialize the I2C peripheral
  I2C_Init_TypeDef i2c_init = I2C_INIT_DEFAULT;
  i2c_init.freq             = I2C_FREQ_FAST_MAX;
  I2C_Init(I2C0, &i2c_init);

  // Enable automatic STOP on NACK
  I2C0->CTRL = I2C_CTRL_AUTOSN;
}

static void adc_init(void)
{
  // Enable ADC clock
  CMU_ClockEnable(cmuClock_IADC0, true);

  // Configure the IADC
  IADC_Init_t init    = IADC_INIT_DEFAULT;
  init.warmup         = iadcWarmupKeepWarm;
  init.srcClkPrescale = IADC_calcSrcClkPrescale(IADC0, 1000000, 0);

  // Configure for 3.3V VDD reference, 1x gain, 2x oversampling, normal mode
  IADC_AllConfigs_t initAllConfigs       = IADC_ALLCONFIGS_DEFAULT;
  initAllConfigs.configs[0].reference    = iadcCfgReferenceVddx;
  initAllConfigs.configs[0].vRef         = 3300;
  initAllConfigs.configs[0].osrHighSpeed = iadcCfgOsrHighSpeed2x;
  initAllConfigs.configs[0].analogGain   = iadcCfgAnalogGain1x;
  initAllConfigs.configs[0].adcClkPrescale =
      IADC_calcAdcClkPrescale(IADC0, 125000, 0, iadcCfgModeNormal, init.srcClkPrescale);

  // Trigger continuously once scan is started
  IADC_InitScan_t initScan = IADC_INITSCAN_DEFAULT;
  initScan.triggerAction   = iadcTriggerActionContinuous;
  initScan.dataValidLevel  = iadcFifoCfgDvl1;
  initScan.fifoDmaWakeup   = true;

  // Configure scan inputs
  IADC_ScanTable_t scanTable         = IADC_SCANTABLE_DEFAULT;
  scanTable.entries[0].posInput      = STICK_X_IADC;
  scanTable.entries[0].negInput      = iadcNegInputGnd;
  scanTable.entries[0].includeInScan = true;

  scanTable.entries[1].posInput      = STICK_Y_IADC;
  scanTable.entries[1].negInput      = iadcNegInputGnd;
  scanTable.entries[1].includeInScan = true;

  scanTable.entries[2].posInput      = SUBSTICK_X_IADC;
  scanTable.entries[2].negInput      = iadcNegInputGnd;
  scanTable.entries[2].includeInScan = true;

  scanTable.entries[3].posInput      = SUBSTICK_Y_IADC;
  scanTable.entries[3].negInput      = iadcNegInputGnd;
  scanTable.entries[3].includeInScan = true;

  scanTable.entries[4].posInput      = TRIGGER_LEFT_IADC;
  scanTable.entries[4].negInput      = iadcNegInputGnd;
  scanTable.entries[4].includeInScan = true;

  scanTable.entries[5].posInput      = TRIGGER_RIGHT_IADC;
  scanTable.entries[5].negInput      = iadcNegInputGnd;
  scanTable.entries[5].includeInScan = true;

  // Initialize IADC
  IADC_init(IADC0, &init, &initAllConfigs);

  // Initialize scan
  IADC_initScan(IADC0, &initScan, &scanTable);

  // Allocate the analog bus for each pin
  GPIO->STICK_X_BUS |= STICK_X_BUSALLOC;
  GPIO->STICK_Y_BUS |= STICK_Y_BUSALLOC;
  GPIO->SUBSTICK_X_BUS |= SUBSTICK_X_BUSALLOC;
  GPIO->SUBSTICK_Y_BUS |= SUBSTICK_Y_BUSALLOC;
  GPIO->TRIGGER_LEFT_BUS |= TRIGGER_LEFT_BUSALLOC;
  GPIO->TRIGGER_RIGHT_BUS |= TRIGGER_RIGHT_BUSALLOC;

  // Configure LDMA transfer on IADC scan completion
  LDMA_TransferCfg_t transferCfg = LDMA_TRANSFER_CFG_PERIPHERAL(ldmaPeripheralSignal_IADC0_IADC_SCAN);
  descriptor =
      (LDMA_Descriptor_t)LDMA_DESCRIPTOR_LINKREL_P2M_WORD(&(IADC0->SCANFIFODATA), adc_state, NUM_ANALOG_INPUTS, 0);
  descriptor.xfer.doneIfs = false;

  // Start transfer
  LDMA_StartTransfer(LDMA_CHANNEL_ADC, (void *)&transferCfg, (void *)&descriptor);

  // Start ADC scanning
  IADC_command(IADC0, iadcCmdStartScan);
}

static void read_buttons(void)
{
  uint8_t port_state[2];
  int rcode = tca9555_read_ports(TCA9555_ADDR, port_state);
  if (rcode < 0)
    return;

  // Copy button states from GPIO expander
  // clang-format off
  input_state.buttons &= ~JOYBUS_GCN_BUTTON_MASK;
  input_state.buttons |= 
    ((port_state[0] & 0x01) ? JOYBUS_GCN_BUTTON_L     : 0) |
    ((port_state[0] & 0x02) ? JOYBUS_GCN_BUTTON_UP    : 0) |
    ((port_state[0] & 0x04) ? JOYBUS_GCN_BUTTON_DOWN  : 0) |
    ((port_state[0] & 0x08) ? JOYBUS_GCN_BUTTON_RIGHT : 0) |
    ((port_state[0] & 0x10) ? JOYBUS_GCN_BUTTON_LEFT  : 0) |
    ((port_state[1] & 0x01) ? JOYBUS_GCN_BUTTON_A     : 0) |
    ((port_state[1] & 0x02) ? JOYBUS_GCN_BUTTON_B     : 0) |
    ((port_state[1] & 0x04) ? JOYBUS_GCN_BUTTON_X     : 0) |
    ((port_state[1] & 0x08) ? JOYBUS_GCN_BUTTON_Y     : 0) |
    ((port_state[1] & 0x10) ? JOYBUS_GCN_BUTTON_Z     : 0) |
    ((port_state[1] & 0x20) ? JOYBUS_GCN_BUTTON_START : 0) |
    ((port_state[1] & 0x40) ? JOYBUS_GCN_BUTTON_R     : 0);
  // clang-format on
}

static void read_analog_channels(void)
{
  // Copy analog input states from ADC readings, converting from 12-bit to 8-bit
  input_state.stick_x       = (adc_state[0] >> 4) & 0xFF;
  input_state.stick_y       = (adc_state[1] >> 4) & 0xFF;
  input_state.substick_x    = (adc_state[2] >> 4) & 0xFF;
  input_state.substick_y    = (adc_state[3] >> 4) & 0xFF;
  input_state.trigger_left  = (adc_state[4] >> 4) & 0xFF;
  input_state.trigger_right = (adc_state[5] >> 4) & 0xFF;
}

static void gpio_interrupt(uint8_t intNo, void *ctx)
{
  read_buttons();
}

void local_input_init(local_input_cb_t callback)
{
  input_callback = callback;

  i2c_init();
  adc_init();

  // Configure all GPIOs as inputs, except RUMBLE_EN and AUX
  tca9555_write_reg(TCA9555_ADDR, TCA9555_REG_CONF0, 0x3F);
  tca9555_write_reg(TCA9555_ADDR, TCA9555_REG_CONF1, 0xFF);

  // Invert input polarity for buttons and WIRELESS_EN
  tca9555_write_reg(TCA9555_ADDR, TCA9555_REG_POLINV0, 0x3F);
  tca9555_write_reg(TCA9555_ADDR, TCA9555_REG_POLINV1, 0xFF);

  // Set all outputs low
  tca9555_write_reg(TCA9555_ADDR, TCA9555_REG_OUTPORT0, 0x00);

  GPIO_PinModeSet(gpioPortD, 1, gpioModePushPull, 0); // RUMBLE_PWM
  GPIO_PinOutSet(gpioPortD, 1);

  // Set up GPIO interrupt for TCA9555 IRQ pin
  GPIOINT_Init();
  GPIO_PinModeSet(TCA9555_IRQ_PORT, TCA9555_IRQ_PIN, gpioModeInputPull, 1);
  GPIOINT_CallbackRegisterExt(TCA9555_IRQ_PIN, gpio_interrupt, NULL);
  GPIO_ExtIntConfig(TCA9555_IRQ_PORT, TCA9555_IRQ_PIN, TCA9555_IRQ_PIN, false, true, true);
}

void local_input_set_motor_state(bool enabled)
{
  if (enabled) {
    tca9555_write_reg(TCA9555_ADDR, TCA9555_REG_OUTPORT0, 0xFF);
  } else {
    tca9555_write_reg(TCA9555_ADDR, TCA9555_REG_OUTPORT0, 0x00);
  }
}

void local_input_process()
{
  read_analog_channels();

  if (input_callback)
    input_callback(&input_state);
}