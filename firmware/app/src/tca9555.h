#pragma once

#include <stdint.h>

// ICA9555 I2C address and register map
#define TCA9555_BASE_ADDR     0x20
#define TCA9555_REG_INPORT0   0x00
#define TCA9555_REG_INPORT1   0x01
#define TCA9555_REG_OUTPORT0  0x02
#define TCA9555_REG_OUTPORT1  0x03
#define TCA9555_REG_POLINV0   0x04
#define TCA9555_REG_POLINV1   0x05
#define TCA9555_REG_CONF0     0x06
#define TCA9555_REG_CONF1     0x07

/**
 * Write a value to a register on the TCA9555.
 *
 * @param addr  I2C address of the TCA9555.
 * @param reg   Register to write to.
 * @param value Value to write to the register.
 *
 * @return I2C transfer status, negative on error.
 */
int tca9555_write_reg(uint8_t addr, uint8_t reg, uint8_t value);

/**
 * Read a value from a register on the TCA9555.
 *
 * @param addr  I2C address of the TCA9555.
 * @param reg   Register to read from.
 * @param value Pointer to store the read value.
 *
 * @return I2C transfer status, negative on error.
 */
int tca9555_read_reg(uint8_t addr, uint8_t reg, uint8_t *value);

/**
 * Read both input ports from the TCA9555.
 *
 * @param addr  I2C address of the TCA9555.
 * @param state Pointer to store the read port states (2 bytes).
 *
 * @return I2C transfer status, negative on error.
 */
int tca9555_read_ports(uint8_t addr, uint8_t *state);