#include <stdio.h>
#include "sensor.h"
#include "hal_i2c.h"
#include "pal.h"

/* 
    * Note1: SDR need to set 2'component
    * Note2: /8 could also be set with MBR
*/

uint8_t lm75b_read(uint8_t sensor_num, int *reading) {
    uint8_t retry = 5;
    I2C_MSG msg;
    int val;

    msg.bus = sensor_config[SnrNum_SnrCfg_map[sensor_num]].port;
    msg.slave_addr = sensor_config[SnrNum_SnrCfg_map[sensor_num]].slave_addr;
    msg.tx_len = 1;
    msg.rx_len = 2;
    msg.data[0] = sensor_config[SnrNum_SnrCfg_map[sensor_num]].offset;

    if ( i2c_master_read(&msg, retry) )
        return SNR_FAIL_TO_ACCESS;
    
    val = (msg.data[0] << 3) | (msg.data[1] >> 5);

    sen_val *sval = (sen_val *)reading;
    sval->integer = (val/8) & 0xFFFF;;
    sval->fraction = (int)( ( (float)val/8 - val/8 )*1000 ) & 0xFFFF;

    return SNR_READ_SUCCESS;
}

uint8_t lm75b_init(uint8_t sensor_num)
{
  sensor_config[SnrNum_SnrCfg_map[sensor_num]].read = lm75b_read;
  return true;
}
