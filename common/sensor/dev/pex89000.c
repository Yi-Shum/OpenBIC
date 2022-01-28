/* 
  PEX89000 Hardware I2C Slave UG_v1.0.pdf
  PEX89000_RM100.pdf
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
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

#define BRCM_FULL_ADDR_BIT  0x007C0000  // bits[22:18]

#define TEMP 0xFE   // TBD: sensor offset

typedef struct {
  uint8_t oft9_2bit: 8;
  uint8_t oft11_10bit: 2;
  uint8_t be: 4;
  uint8_t oft13_12bit: 2;
  uint8_t oft21_14bit: 8;
  uint8_t cmd: 3;
  uint8_t reserve1: 5;
} __packed __aligned(4) HW_I2C_Cmd;

typedef struct PEX89000_unit {
  uint8_t number; // sensor number
  struct k_mutex mutex;
  struct PEX89000_unit *next;
} pex89000_unit;

static pex89000_unit *head = NULL;

static void swap32(uint32_t *d)
{
  if (!d)
    return;

  uint32_t t = ((*d >> 24) & 0xFF) |
              ((*d >> 8) & 0xFF00) |
              ((*d << 8) & 0xFF0000) |
              ((*d << 24) & 0xFF000000);

  *d = t;
}

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
  buf->oft13_12bit = (oft >> 12) & 0x1;
  buf->be = be;
  buf->oft11_10bit = (oft >> 10) & 0x3;
  buf->oft9_2bit = (oft >> 2) & 0xFF;
}

/* The old spec requires the use of full address */
static uint8_t set_full_addr(uint8_t bus, uint8_t addr, uint8_t full_oft) // full_oft: bit[22:18]
{
  /* I2C write command to set address bits[22:18] in 0x2CC[4:0] */
  HW_I2C_Cmd cmd;
  pex89000_i2c_encode(0x2CC, 0xF, BRCM_I2C5_CMD_WRITE, &cmd);

  uint8_t retry = 5;
  I2C_MSG msg;

  msg.bus = bus;
  msg.slave_addr = addr;
  msg.tx_len = 8;
  memcpy(&msg.data[0], &cmd, sizeof(cmd));
  msg.data[7] = full_oft;


  if (i2c_master_write(&msg, retry)) {
    /* write fail */
    printf("set AXI register to full mode failed!\n");
    return 0;
  }

  return 1;
}

static uint8_t pex89000_chime_read(uint8_t bus, uint8_t addr, uint32_t oft, uint8_t *resp, uint16_t resp_len)
{
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

    if ((resp >> 24) & 0x8) {	// CHIME_to_AXI_CSR Control Status -> Read_data_vaild
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

  swap32(&wbuf);
  if(!pex89000_chime_write(bus, addr, BRCM_CHIME_AXI_CSR_ADDR, (uint8_t *)&wbuf, sizeof(wbuf))){
    goto exit;
  }
  wbuf = data;
  swap32(&wbuf);
  if(!pex89000_chime_write(bus, addr, BRCM_CHIME_AXI_CSR_DATA, (uint8_t *)&wbuf, sizeof(wbuf))){
    goto exit;
  }
  wbuf = 0x1; // CHIME_to_AXI_CSR Control Status: write command
  swap32(&wbuf);
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

  uint32_t data = oft;
  swap32(&data);
  if(!pex89000_chime_write(bus, addr, BRCM_CHIME_AXI_CSR_ADDR, (uint8_t *)&data, sizeof(data))){
    goto exit;
  }
  data = 0x2;// CHIME_to_AXI_CSR Control Status: read command
  swap32(&data);
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

  swap32(resp);
  rc = 1;

exit:
  return rc;
}

uint8_t pex89000_die_temp(uint8_t bus, uint8_t addr, sen_val *val)
{
  if (!val)
    return 0;

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

/* The following functions are for linked list */
pex89000_unit* pex89000_new_node(uint8_t number)
{
    pex89000_unit* temp = (pex89000_unit *)malloc(sizeof(pex89000_unit));
    if(!temp)
        return NULL;
    temp->number = number;
    temp->next = NULL;  
    return temp;
}

void pex89000_insert_node(pex89000_unit* a, pex89000_unit* b)
{
  b->next = a->next;
  a->next = b;
}

pex89000_unit* pex89000_find_node(uint8_t number)
{
  pex89000_unit* p = head;
  while(p->next != NULL) {
    if(p->number == number)
      return p;
    else
     p = p->next;
  }

  return (p->number == number) ? p : NULL;

}
  
/* linked list end */

uint8_t pex89000_read(uint8_t sensor_num, int* reading)
{
  if(reading == NULL)
    return SNR_UNSPECIFIED_ERROR;

  pex89000_unit *p = pex89000_find_node(sensor_num);
  uint8_t rc = SNR_UNSPECIFIED_ERROR;
  int ret = k_mutex_lock(&p->mutex, K_MSEC(5000)); //  TBD: timeout 5s
  
  if (ret) {
    printk("pex89000_die_temp[0x%02x]: mutex get fail status: %x\n", p->number, ret);
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

  static pex89000_unit* p; // pointer to end node
  if(sensor_config[SnrNum_SnrCfg_map[sensor_num]].type == sen_dev_pex89000) {
    pex89000_unit *temp = pex89000_new_node(sensor_num); // temp node
    
    if(head == NULL) {
      head = temp;
    }
    else {
      pex89000_insert_node(p, temp); 
    }

    p = temp;
  }

  return true;
}
