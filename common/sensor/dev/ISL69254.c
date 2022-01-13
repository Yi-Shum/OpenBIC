#include <stdio.h>
#include <string.h>
#include "sensor.h"
#include "hal_i2c.h"

#define ISL69254_VOL_CMD              0x8B
#define ISL69254_CUR_CMD              0x8C
#define ISL69254_TEMP_CMD             0x8D
#define ISL69254_PWR_CMD              0x96


uint8_t ISL69254_read(uint8_t sensor_num, int* reading) {
  if(reading == NULL) {
    return SNR_UNSPECIFIED_ERROR;
  }

  uint8_t retry = 5;
  sen_val *snr_val = (sen_val*)reading;
  I2C_MSG msg;
  memset(snr_val, 0, sizeof(sen_val));

  msg.bus = sensor_config[SnrNum_SnrCfg_map[sensor_num]].port;
  msg.slave_addr = sensor_config[SnrNum_SnrCfg_map[sensor_num]].slave_addr;
  msg.tx_len = 1;
  msg.rx_len = 2;
  msg.data[0] = sensor_config[SnrNum_SnrCfg_map[sensor_num]].offset;
  
  if (i2c_master_read(&msg, retry)) {
    /* read fail */
    return SNR_FAIL_TO_ACCESS;
  }

  switch (sensor_config[SnrNum_SnrCfg_map[sensor_num]].offset) {
  case ISL69254_VOL_CMD:
    /* unit get from sensor: 1 mV
       return unit: 1 V */
    snr_val->integer = ((msg.data[1] << 8) | msg.data[0]) / 1000;
    snr_val->fraction = ((msg.data[1] << 8) | msg.data[0]) % 1000;
    break;
  case ISL69254_CUR_CMD:
    /* unit get from sensor: 0.1 Amp
       return unit: 1 Amp */
    snr_val->integer = (((msg.data[1] << 8) | msg.data[0]) / 10);
    snr_val->fraction = ((((msg.data[1] << 8) | msg.data[0]) % 10) * 100);
    break;
  case ISL69254_TEMP_CMD:
    /* unit: 1 Degree C */
    snr_val->integer = ((msg.data[1] << 8) | msg.data[0]);
    break;
  case ISL69254_PWR_CMD:
    /* unit: 1 Watt */
    snr_val->integer = ((msg.data[1] << 8) | msg.data[0]);
    break;
  default:
    return SNR_FAIL_TO_ACCESS;
  }

  return SNR_READ_SUCCESS;
}
