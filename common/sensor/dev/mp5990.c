#include <stdio.h>
#include "sensor.h"
#include "hal_i2c.h"
#include "pal.h"


uint8_t mp5990_read(uint8_t sensor_num, int *reading) {
  
  uint8_t retry = 5;
  double val;
  I2C_MSG msg;
  
  snr_cfg *cfg = &sensor_config[SnrNum_SnrCfg_map[sensor_num]];
  
  msg.bus = cfg->port;
  msg.slave_addr = cfg->slave_addr;
  msg.tx_len = 1;
  msg.rx_len = 2;
  msg.data[0] = cfg->offset;

  if (i2c_master_read(&msg, retry))
    return SNR_FAIL_TO_ACCESS;
  
  switch (cfg->offset) {
	case PMBUS_READ_VOUT:
    /* 31.25 mv/LSB */
    val = ((msg.data[1] << 8) | msg.data[0])*31.25;
    break;
  case PMBUS_READ_IOUT:
    /* 62.5 mA/LSB */
    val = ((msg.data[1] << 8) | msg.data[0])*62.5;
    break;
  case PMBUS_READ_TEMPERATURE_1:
    /* 1 degree c/LSB */
    val = msg.data[0];
    break;
  case PMBUS_READ_POUT:
    /* 1 W/LSB */
    val = ((msg.data[1] << 8) | msg.data[0]);
    break;
  default:
    return SNR_NOT_FOUND;
  }

  sen_val *sval = (sen_val *)reading;
  sen_val_from_double(sval, val);

  return SNR_READ_SUCCESS;
}