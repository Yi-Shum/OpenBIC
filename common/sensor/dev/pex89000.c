#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "sensor.h"
#include "hal_i2c.h"
#include "pex89000.h"

typedef struct {
	uint8_t oft9_2bit : 8;
	uint8_t oft11_10bit : 2;
	uint8_t be : 4;
	uint8_t reserve3 : 1;
	uint8_t oft12bit : 1;
	uint8_t oft17_13bit : 5;
	uint8_t access_mode : 2;
	uint8_t reserve2 : 1;
	uint8_t cmd : 3;
	uint8_t reserve1 : 5;
} __packed __aligned(4) HW_I2C_Cmd;

static void swap32(uint32_t *d)
{
	if (!d)
		return;

	uint32_t t = ((*d >> 24) & 0xFF) | ((*d >> 8) & 0xFF00) | ((*d << 8) & 0xFF0000) |
		     ((*d << 24) & 0xFF000000);

	*d = t;
}

/*
 * be: byte enables
 * oft: Atlas register address
 * cmd: read or write command
 * access_mode: access mode
 * data: DWORD data
 * buf: encoded byte array to send to pesw
 */

static void pex89000_i2c_encode(uint32_t oft, uint8_t be, uint8_t cmd, uint8_t access_mode,
				HW_I2C_Cmd *buf)
{
	buf->reserve1 = 0;
	buf->cmd = cmd;
	buf->reserve2 = 0;
	buf->access_mode = access_mode;
	buf->oft17_13bit = (oft >> 13) & 0x1F;
	buf->oft12bit = (oft >> 12) & 0x1;
	buf->reserve3 = 0;
	buf->be = be;
	buf->oft11_10bit = (oft >> 10) & 0x3;
	buf->oft9_2bit = (oft >> 2) & 0xFF;
}

static uint8_t set_full_addr(uint8_t bus, uint8_t addr, uint32_t oft)
{
	/* I2C write command to set address bits[22:18] in 0x2CC[4:0] */

	uint8_t sub_oft = (uint8_t)(oft & BRCM_FULL_ADDR_BIT) >> 18; // sub_oft: bit[22:18]

	HW_I2C_Cmd cmd;
	pex89000_i2c_encode(0x2CC, 0xF, BRCM_I2C5_CMD_WRITE, BRCM_I2C5_ACCESS_MODE_LOWER18, &cmd);

	uint8_t data[8] = { 0 };
	memcpy(&data[0], &cmd, sizeof(cmd));
	memcpy(&data[7], &sub_oft, sizeof(sub_oft));

	uint8_t retry = 5;
	I2C_MSG msg;

	msg.bus = bus;
	msg.slave_addr = addr;
	msg.tx_len = sizeof(data);
	memcpy(&msg.data[0], data, sizeof(data));

	if (i2c_master_write(&msg, retry)) {
		/* write fail */
		printf("set AXI register to full mode failed!\n");
		return 1;
	}

	return 0;
}

static uint8_t pex89000_chime_read(uint8_t bus, uint8_t addr, uint32_t oft, uint8_t *resp,
				   uint16_t resp_len)
{
	if (set_full_addr(bus, addr, oft)) // fail to set full addr
		return 1;

	HW_I2C_Cmd cmd;
	pex89000_i2c_encode(oft, 0xF, BRCM_I2C5_CMD_READ, BRCM_I2C5_ACCESS_MODE_FULL, &cmd);

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
		return 1;
	}

	memcpy(resp, &msg.data[0], resp_len);

	return 0;
}

static uint8_t pex89000_chime_write(uint8_t bus, uint8_t addr, uint32_t oft, uint8_t *data,
				    uint8_t data_len)
{
	if (set_full_addr(bus, addr, oft))
		return 1;

	HW_I2C_Cmd cmd;
	pex89000_i2c_encode(oft, 0xF, BRCM_I2C5_CMD_WRITE, BRCM_I2C5_ACCESS_MODE_FULL, &cmd);

	/* This buffer is for PEC calculated and write to switch by I2C */
	uint8_t *data_buf = (uint8_t *)malloc(sizeof(cmd) + data_len);
	if (!data_buf)
		return 1;
	memcpy(data_buf, &cmd, sizeof(cmd));
	memcpy(data_buf + sizeof(cmd), data, data_len);

	uint8_t retry = 5;
	I2C_MSG msg;

	msg.bus = bus;
	msg.slave_addr = addr;
	msg.tx_len = sizeof(cmd) + data_len;
	memcpy(&msg.data[0], data_buf, sizeof(cmd) + data_len);

	if (i2c_master_write(&msg, retry)) {
		/* write fail */
		printf("pex89000 write failed!!\n");
		free(data_buf);
		return 1;
	}

	free(data_buf);
	return 0;
}

static uint8_t pend_for_read_valid(uint8_t bus, uint8_t addr)
{
	uint8_t rty = 50;
	uint32_t resp = 0;

	while (rty--) {
		if (pex89000_chime_read(bus, addr, BRCM_CHIME_AXI_CSR_CTL, (uint8_t *)&resp,
					sizeof(resp))) {
			k_msleep(10);
			continue;
		}

		if ((resp >> 24) & 0x8) { // CHIME_to_AXI_CSR Control Status -> Read_data_vaild
			return 0; //  success
		}

		k_msleep(10);
	}

	return 1;
}

static uint8_t pex89000_chime_to_axi_write(uint8_t bus, uint8_t addr, uint32_t oft, uint32_t data)
{
	uint32_t wbuf = oft;

	swap32(&wbuf);
	if (pex89000_chime_write(bus, addr, BRCM_CHIME_AXI_CSR_ADDR, (uint8_t *)&wbuf,
				 sizeof(wbuf))) {
		return 1;
	}
	wbuf = data;
	swap32(&wbuf);
	if (pex89000_chime_write(bus, addr, BRCM_CHIME_AXI_CSR_DATA, (uint8_t *)&wbuf,
				 sizeof(wbuf))) {
		return 1;
	}
	wbuf = 0x1; // CHIME_to_AXI_CSR Control Status: write command
	swap32(&wbuf);
	if (pex89000_chime_write(bus, addr, BRCM_CHIME_AXI_CSR_CTL, (uint8_t *)&wbuf,
				 sizeof(wbuf))) {
		return 1;
	}

	return 0;
}

static uint8_t pex89000_chime_to_axi_read(uint8_t bus, uint8_t addr, uint32_t oft, uint32_t *resp)
{
	uint32_t data = oft;
	swap32(&data);
	if (pex89000_chime_write(bus, addr, BRCM_CHIME_AXI_CSR_ADDR, (uint8_t *)&data,
				 sizeof(data))) {
		return 1;
	}
	data = 0x2; // CHIME_to_AXI_CSR Control Status: read command
	swap32(&data);
	if (pex89000_chime_write(bus, addr, BRCM_CHIME_AXI_CSR_CTL, (uint8_t *)&data,
				 sizeof(data))) {
		return 1;
	}

	k_msleep(10);
	if (pend_for_read_valid(bus, addr)) {
		printf("read data invaild\n");
		return 1;
	}

	if (pex89000_chime_read(bus, addr, BRCM_CHIME_AXI_CSR_DATA, (uint8_t *)resp,
				sizeof(resp))) {
		return 1;
	}

	swap32(resp);

	return 0;
}

uint8_t pex89000_die_temp(uint8_t bus, uint8_t addr, sensor_val *val)
{
	if (!val)
		return 1;

	uint32_t resp = 0;

	//check 0xFFE78504 value
	if (!pex89000_chime_to_axi_read(bus, addr, BRCM_TEMP_SENSOR0_CTL_REG1, &resp)) {
		if (resp != BRCM_TEMP_SENSOR0_CTL_REG1_RESET) {
			printf("ADC temperature control register1 check fail!\n");
			return 1;
		}
		resp = 0;
	} else {
		printf("CHIME to AXI Read 0xFFE78504 fail!\n");
		return 1;
	}

	//Write 0xFFE78504 = 200653E8
	if (pex89000_chime_to_axi_write(bus, addr, BRCM_TEMP_SENSOR0_CTL_REG1,
					BRCM_TEMP_SENSOR0_CTL_REG1_RESET)) {
		printf("CHIME to AXI Write 0xFFE78504 fail!\n");
		return 1;
	}

	//Read 0xFFE78538
	if (!pex89000_chime_to_axi_read(bus, addr, BRCM_TEMP_SENSOR0_STAT_REG0, &resp)) {
		float tmp = (resp & 0xFFFF) / 128;
		val->integer = (int)tmp;
		val->fraction = (int)(tmp * 1000) % 1000;
		return 0;
	}

	return 1;
}

uint8_t pex89000_read(uint8_t sensor_num, int *reading)
{
	if (reading == NULL)
		return SENSOR_UNSPECIFIED_ERROR;

	pex89000_init_arg *init_arg =
		(pex89000_init_arg *)sensor_config[SensorNum_SensorCfg_map[sensor_num]].init_args;
	uint8_t rc = SENSOR_UNSPECIFIED_ERROR;
	int ret = k_mutex_lock(&init_arg->brcm_pciesw, K_MSEC(5000)); //  TBD: timeout 5s

	if (ret) {
		printk("pex89000_die_temp: mutex[%d] get fail status: %x\n", init_arg->idx, ret);
		return rc;
	}

	switch (sensor_config[SensorNum_SensorCfg_map[sensor_num]].offset) {
	case PEX89000_TEMP_OFFSET:
		if (pex89000_die_temp(sensor_config[SensorNum_SensorCfg_map[sensor_num]].port,
				      sensor_config[SensorNum_SensorCfg_map[sensor_num]].slave_addr,
				      (sensor_val *)reading)) {
			printf("sensor pex89000_read read temp fail!\n");
			rc = SENSOR_FAIL_TO_ACCESS;
			goto exit;
		}
		break;
	default:
		printf("sensor pex89000_read type fail!\n");
		rc = SENSOR_UNSPECIFIED_ERROR;
		goto exit;
	}

	rc = SENSOR_READ_SUCCESS;

exit:
	k_mutex_unlock(&init_arg->brcm_pciesw);
	return rc;
}

bool pex89000_init(uint8_t sensor_num)
{
	sensor_config[SensorNum_SensorCfg_map[sensor_num]].read = pex89000_read;

	if (sensor_config[SensorNum_SensorCfg_map[sensor_num]].init_args == NULL) {
		printk("pex89000_init: init_arg is NULL\n");
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}

	pex89000_init_arg *init_arg =
		(pex89000_init_arg *)sensor_config[SensorNum_SensorCfg_map[sensor_num]].init_args;
	if (k_mutex_init(&init_arg->brcm_pciesw)) {
		printf("pex89000 mutex %d init fail\n", init_arg->idx);
		init_arg->is_init = false;
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}

	init_arg->is_init = true;
	return SENSOR_INIT_SUCCESS;
}
