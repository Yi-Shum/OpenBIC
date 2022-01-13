#include <stdio.h>
#include "sensor.h"
#include "hal_i2c.h"
#include "pal.h"

bool tca9548_select_chan(uint8_t snr_num, void *args) {
  
  if (!args)
    return false;

  snr_cfg *cfg = &sensor_config[SnrNum_SnrCfg_map[snr_num]];
  struct tca9548 *p = (struct tca9548 *)args;

  uint8_t retry = 5;
  I2C_MSG msg = {0};

  msg.bus = cfg->port;
  msg.slave_addr = p->addr;
  msg.tx_len = 1;
  msg.data[0] = (1 << (p->chan));

  if (i2c_master_write(&msg, retry))
    return false;
  
  return true;
}