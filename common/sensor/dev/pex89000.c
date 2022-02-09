/* 
  PEX89000 Hardware I2C Slave UG_v1.0.pdf
  PEX89000_RM100.pdf
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <zephyr.h>
#include <sys/util.h>
#include <sys/byteorder.h>
#include "sensor.h"
#include "hal_i2c.h"

#define BRCM_I2C5_CMD_READ  0b100
#define BRCM_I2C5_CMD_WRITE 0b011

#define BRCM_CHIME_AXI_CSR_ADDR   0x001F0100
#define BRCM_CHIME_AXI_CSR_DATA   0x001F0104
#define BRCM_CHIME_AXI_CSR_CTL    0x001F0108

#define BRCM_TEMP_SNR0_CTL_REG1     0xFFE78504
#define BRCM_TEMP_SNR0_STAT_REG0    0xFFE78538

#define BRCM_TEMP_SNR0_CTL_REG1_RESET     0x000653E8

#define TEMP 0xFE   // TBD: sensor offset

static sys_slist_t pex89000_list;

typedef struct {
  uint8_t idx; // Create index based on init variable
  struct k_mutex mutex;
  sys_snode_t node; // linked list node
} pex89000_unit;

typedef struct {
  uint8_t oft9_2bit: 8;
  uint8_t oft11_10bit: 2;
  uint8_t be: 4;
  uint8_t oft13_12bit: 2;
  uint8_t oft21_14bit: 8;
  uint8_t cmd: 3;
  uint8_t reserve1: 5;
} __packed __aligned(4) HW_I2C_Cmd;

/*
 * be: byte enables
 * oft: Atlas register address
 * cmd: read or write command
 * buf: encoded byte array to send to pesw
 */

static void pex89000_i2c_encode(uint32_t oft, uint8_t be, uint8_t cmd, HW_I2C_Cmd *buf)
{
  if (!buf) {
    printf("pex89000_i2c_encode buf fail!\n");
    return;
  }
  
  buf->reserve1 = 0;
  buf->cmd = cmd;
  buf->oft21_14bit = (oft >> 14) & 0xFF;
  buf->oft13_12bit = (oft >> 12) & 0x3;
  buf->be = be;
  buf->oft11_10bit = (oft >> 10) & 0x3;
  buf->oft9_2bit = (oft >> 2) & 0xFF;
}

static uint8_t pex89000_chime_read(uint8_t bus, uint8_t addr, uint32_t oft, uint8_t *resp, uint16_t resp_len)
{
  if (!resp) {
    printf("pex89000_chime_read *resp does not exist !!\n");
    return 0;
  }

  HW_I2C_Cmd cmd;
  pex89000_i2c_encode(oft, 0xF, BRCM_I2C5_CMD_READ, &cmd);

  uint8_t retry = 5;
  I2C_MSG msg;

  msg.bus = bus;
  msg.slave_addr = addr;
  msg.tx_len = sizeof(cmd);
  msg.rx_len = resp_len;
  memcpy(&msg.data[0], &cmd, sizeof(cmd));

  if (i2c_master_read(&msg, retry)) {
    /* read fail */
    printf("pex89000 read failed!!\n");
    return 0;
  }

  memcpy(resp, &msg.data[0], resp_len);

  return 1;
}

static uint8_t pex89000_chime_write(uint8_t bus, uint8_t addr, uint32_t oft, uint8_t *data, uint8_t data_len)
{
  if (!data) {
    printf("pex89000_chime_write *data does not exist !!\n");
    return 0;
  }

  HW_I2C_Cmd cmd;
  pex89000_i2c_encode(oft, 0xF, BRCM_I2C5_CMD_WRITE, &cmd);

  uint8_t retry = 5;
  I2C_MSG msg;

  msg.bus = bus;
  msg.slave_addr = addr;
  msg.tx_len = sizeof(cmd) + data_len;
  memcpy(&msg.data[0], &cmd, sizeof(cmd));
  memcpy(&msg.data[4], data, data_len);

  if (i2c_master_write(&msg, retry)) {
    /* write fail */
    printf("pex89000 write failed!!\n");
    return 0;
  }

  return 1;
}

static uint8_t pend_for_read_valid(uint8_t bus, uint8_t addr)
{
  uint8_t rty = 50;
  uint32_t resp = 0;

  while (rty--) {
    if(!pex89000_chime_read(bus, addr, BRCM_CHIME_AXI_CSR_CTL, (uint8_t *)&resp, sizeof(resp))) {
      k_msleep(10);
      continue;
    }

    if (resp & 0x8000000000) {	// CHIME_to_AXI_CSR Control Status -> Read_data_vaild
      return 1;   //  success
    }

    k_msleep(10);
  }

  return 0;
}

static uint8_t pex89000_chime_to_axi_write(uint8_t bus, uint8_t addr, uint32_t oft, uint32_t data)
{
  uint8_t rc = 0;

  uint32_t wbuf = oft;

  wbuf = sys_cpu_to_be32(wbuf);
  if(!pex89000_chime_write(bus, addr, BRCM_CHIME_AXI_CSR_ADDR, (uint8_t *)&wbuf, sizeof(wbuf))){
    goto exit;
  }
  wbuf = sys_cpu_to_be32(data);
  if(!pex89000_chime_write(bus, addr, BRCM_CHIME_AXI_CSR_DATA, (uint8_t *)&wbuf, sizeof(wbuf))){
    goto exit;
  }
  wbuf = sys_cpu_to_be32(0x1);  // CHIME_to_AXI_CSR Control Status: write command
  if(!pex89000_chime_write(bus, addr, BRCM_CHIME_AXI_CSR_CTL, (uint8_t *)&wbuf, sizeof(wbuf))){
    goto exit;
  }

  rc = 1;

exit:
  return rc;
}

static uint8_t pex89000_chime_to_axi_read(uint8_t bus, uint8_t addr, uint32_t oft, uint32_t *resp)
{
  uint8_t rc = 0;

  if (!resp) {
    printf("pex89000_chime_to_axi_read *resp does not exist !!\n");
    return rc;
  }

  uint32_t data;
  data = sys_cpu_to_be32(oft);
  if(!pex89000_chime_write(bus, addr, BRCM_CHIME_AXI_CSR_ADDR, (uint8_t *)&data, sizeof(data))){
    goto exit;
  }
  data = sys_cpu_to_be32(0x2);  // CHIME_to_AXI_CSR Control Status: read command
  if(!pex89000_chime_write(bus, addr, BRCM_CHIME_AXI_CSR_CTL, (uint8_t *)&data, sizeof(data))){
    goto exit;
  }

  k_msleep(10);
  if (!pend_for_read_valid(bus, addr)) {
    printf("read data invaild\n");
    goto exit;
  }

  if(!pex89000_chime_read(bus, addr, BRCM_CHIME_AXI_CSR_DATA, (uint8_t *)resp, sizeof(resp))){
    goto exit;
  }

  *resp = sys_cpu_to_be32(*resp);
  rc = 1;

exit:
  return rc;
}

uint8_t pex89000_die_temp(uint8_t bus, uint8_t addr, sen_val *val)
{
  if (!val) {
    printf("pex89000_die_temp *val does not exist !!\n");
    return 0;
  }

  uint8_t rc = 0;
  uint32_t resp = 0;

  //check 0xFFE78504 value
  if (pex89000_chime_to_axi_read(bus, addr, BRCM_TEMP_SNR0_CTL_REG1, &resp)) {
    if(resp != BRCM_TEMP_SNR0_CTL_REG1_RESET){
      printf("ADC temperature control register1 check fail!\n");
       goto exit;
    }
    resp = 0;
  }
  else {
    printf("CHIME to AXI Read 0xFFE78504 fail!\n");
    goto exit;
  }

  //Write 0xFFE78504 = 200653E8
  if (!pex89000_chime_to_axi_write(bus, addr, BRCM_TEMP_SNR0_CTL_REG1, BRCM_TEMP_SNR0_CTL_REG1_RESET)) {
    printf("CHIME to AXI Write 0xFFE78504 fail!\n");
    goto exit;
  }

  //Read 0xFFE78538 
  if (pex89000_chime_to_axi_read(bus, addr, BRCM_TEMP_SNR0_STAT_REG0, &resp)) {
    float tmp = (resp & 0xFFFF) / 128;
    val->integer = (int)tmp;
    val->fraction = (int)(tmp*1000)%1000;
    rc = 1;
    goto exit;
  }

exit:
  return rc;
}

pex89000_unit *find_pex89000_from_idx(uint8_t idx)
{
  sys_snode_t *node;
  SYS_SLIST_FOR_EACH_NODE(&pex89000_list, node) {
    pex89000_unit *p;
    p = CONTAINER_OF(node, pex89000_unit, node);
    if(p->idx == idx) {
      return p;
    }
  }

  return NULL;
}

uint8_t pex89000_read(uint8_t sensor_num, int* reading)
{
  if(!reading) {
    printf("pex89000_read *reading does not exist !!\n");
    return SNR_UNSPECIFIED_ERROR;
  }

  uint8_t *idx = (uint8_t *)sensor_config[SnrNum_SnrCfg_map[sensor_num]].priv_data;
  pex89000_unit *p = find_pex89000_from_idx(*idx);  // pointer to linked list node 
  uint8_t rc = SNR_UNSPECIFIED_ERROR;
  int ret = k_mutex_lock(&p->mutex, K_MSEC(5000)); //  TBD: timeout 5s
  
  if (ret) {
    printk("pex89000_die_temp: mutex %d get fail status: %x\n", p->idx, ret);
    return rc;
  }
  
  switch(sensor_config[SnrNum_SnrCfg_map[sensor_num]].offset)
  {
    case TEMP:
      if(!pex89000_die_temp(sensor_config[SnrNum_SnrCfg_map[sensor_num]].port, sensor_config[SnrNum_SnrCfg_map[sensor_num]].slave_addr, (sen_val*)reading)) {
        printf("sensor pex89000_read read temp fail!\n");
        rc = SNR_FAIL_TO_ACCESS;
        goto exit;
      }
      break;
    default:
      printf("sensor pex89000_read type fail!\n");
      rc = SNR_UNSPECIFIED_ERROR;
      goto exit;
  }

  rc = SNR_READ_SUCCESS;
  
exit:
  k_mutex_unlock(&p->mutex);
  return rc;
}

bool pex89000_init(uint8_t sensor_num)
{
  sensor_config[SnrNum_SnrCfg_map[sensor_num]].read = pex89000_read;

  if (sensor_config[SnrNum_SnrCfg_map[sensor_num]].init_args == NULL) {
    printf("pex89000_init: init_arg is NULL\n");
    return false;
  }

  /* init linked list */
  static uint8_t list_init = 0;
  if(!list_init) {
    sys_slist_init(&pex89000_list);
    list_init = 1;
  }

  pex89000_init_arg *init_arg = (pex89000_init_arg *)sensor_config[SnrNum_SnrCfg_map[sensor_num]].init_args;

  pex89000_unit *p;
  p = find_pex89000_from_idx(init_arg->idx);
  if(p == NULL) {
    pex89000_unit *temp = (pex89000_unit *)malloc(sizeof(pex89000_unit));
    if(!temp) {
      printf("pex89000_init: pex89000_unit malloc failed!\n");
      return NULL;
    }
        
    temp->idx = init_arg->idx;

    if ( k_mutex_init(&temp->mutex) ) {
      printf("pex89000 mutex %d init fail\n", temp->idx);
      init_arg->is_init = false;
      return false;
    }

    sys_slist_append(&pex89000_list, &temp->node);
  }

  uint8_t *idx = (uint8_t *)sensor_config[SnrNum_SnrCfg_map[sensor_num]].priv_data;
  *idx = init_arg->idx;  // priv_data = index

  return true;
}
