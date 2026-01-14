#include "tca9555.h"

#include "em_i2c.h"

int tca9555_write_reg(uint8_t addr, uint8_t reg, uint8_t value)
{
  // Build the request
  uint8_t msg[2] = {reg, value};
  I2C_TransferSeq_TypeDef i2cTransfer;
  i2cTransfer.addr        = addr << 1;
  i2cTransfer.flags       = I2C_FLAG_WRITE;
  i2cTransfer.buf[0].data = msg;
  i2cTransfer.buf[0].len  = 2;

  // Send data and wait for completion
  I2C_TransferReturn_TypeDef result = I2C_TransferInit(I2C0, &i2cTransfer);
  while (result == i2cTransferInProgress) {
    result = I2C_Transfer(I2C0);
  }

  return result;
}

int tca9555_read_reg(uint8_t addr, uint8_t reg, uint8_t *value)
{
  // Build the request
  I2C_TransferSeq_TypeDef i2cTransfer;
  i2cTransfer.addr        = addr << 1;
  i2cTransfer.flags       = I2C_FLAG_WRITE_READ;
  i2cTransfer.buf[0].data = &reg;
  i2cTransfer.buf[0].len  = 1;
  i2cTransfer.buf[1].data = value;
  i2cTransfer.buf[1].len  = 1;

  // Send data and wait for completion
  I2C_TransferReturn_TypeDef result = I2C_TransferInit(I2C0, &i2cTransfer);
  while (result == i2cTransferInProgress) {
    result = I2C_Transfer(I2C0);
  }

  return result;
}

int tca9555_read_ports(uint8_t addr, uint8_t *state)
{
  // Build the request
  I2C_TransferSeq_TypeDef i2cTransfer;
  i2cTransfer.addr        = addr << 1;
  i2cTransfer.flags       = I2C_FLAG_WRITE_READ;
  i2cTransfer.buf[0].data = (uint8_t[]){TCA9555_REG_INPORT0};
  i2cTransfer.buf[0].len  = 1;
  i2cTransfer.buf[1].data = state;
  i2cTransfer.buf[1].len  = 2;

  // Send data and wait for completion
  I2C_TransferReturn_TypeDef result = I2C_TransferInit(I2C0, &i2cTransfer);
  while (result == i2cTransferInProgress) {
    result = I2C_Transfer(I2C0);
  }

  return result;
}