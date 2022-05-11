#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sensor.h"
#include "hal_i2c.h"
#include "libutil.h"
#include "pmbus.h"

#define I2C_DATA_SIZE 5

uint8_t mp5990_read(uint8_t sensor_num, int *reading)
{
	if ((reading == NULL) || (sensor_num > SENSOR_NUM_MAX) ||
	    (sensor_config[sensor_config_index_map[sensor_num]].init_args == NULL)) {
		return SENSOR_UNSPECIFIED_ERROR;
	}

	uint8_t retry = 5;
	double val;
	I2C_MSG msg = { 0 };

	sensor_cfg *cfg = &sensor_config[sensor_config_index_map[sensor_num]];

	msg.bus = cfg->port;
	msg.target_addr = cfg->target_addr;
	msg.tx_len = 1;
	msg.rx_len = 2;
	msg.data[0] = cfg->offset;

	if (i2c_master_read(&msg, retry))
		return SENSOR_FAIL_TO_ACCESS;

	switch (cfg->offset) {
	case PMBUS_READ_VIN:
	case PMBUS_READ_VOUT:
		/* 31.25 mv/LSB */
		val = ((msg.data[1] << 8) | msg.data[0]) * 31.25 / 1000;
		break;
	case PMBUS_READ_IOUT:
		/* 62.5 mA/LSB */
		val = ((msg.data[1] << 8) | msg.data[0]) * 62.5 / 1000;
		break;
	case PMBUS_READ_TEMPERATURE_1:
		/* 1 degree c/LSB */
		val = msg.data[0];
		break;
	case PMBUS_READ_POUT:
	case PMBUS_READ_PIN:
		/* 1 W/LSB */
		val = ((msg.data[1] << 8) | msg.data[0]);
		break;
	default:
		return SENSOR_NOT_FOUND;
	}

	sensor_val *sval = (sensor_val *)reading;
	memset(sval, 0, sizeof(*sval));

	sval->integer = (int32_t)val;
	sval->fraction = (int32_t)(val * 1000) % 1000;

	return SENSOR_READ_SUCCESS;
}

uint8_t mp5990_init(uint8_t sensor_num)
{
	if (sensor_num > SENSOR_NUM_MAX) {
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}

	sensor_cfg *cfg = &sensor_config[sensor_config_index_map[sensor_num]];
	mp5990_init_arg *init_args = (mp5990_init_arg *)cfg->init_args;

	if (!init_args) {
		printf("<error> MP5990 init args are not provided!\n");
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}

	if (init_args->is_init)
		goto skip_init;

	uint8_t retry = 5;
	I2C_MSG msg;
	char *data = (uint8_t *)malloc(I2C_DATA_SIZE * sizeof(uint8_t));

	uint8_t bus = cfg->port;
	uint8_t target_addr = cfg->target_addr;
	uint8_t tx_len = 3;
	uint8_t rx_len = 0;

	data[0] = PMBUS_IOUT_CAL_GAIN;
	data[1] = init_args->iout_cal_gain & 0xFF;
	data[2] = (init_args->iout_cal_gain >> 8) & 0xFF;
	msg = construct_i2c_message(bus, target_addr, tx_len, data, rx_len);
	if (i2c_master_write(&msg, retry) != 0) {
		printf("Failed to write MP5990 register(0x%x)\n", data[0]);
		SAFE_FREE(data);
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}

	tx_len = 3;
	rx_len = 0;
	data[0] = PMBUS_IOUT_OC_FAULT_LIMIT;
	data[1] = init_args->iout_oc_fault_limit & 0xFF;
	data[2] = (init_args->iout_oc_fault_limit >> 8) & 0xFF;
	msg = construct_i2c_message(bus, target_addr, tx_len, data, rx_len);
	if (i2c_master_write(&msg, retry) != 0) {
		printf("Failed to write MP5990 register(0x%x)\n", data[0]);
		SAFE_FREE(data);
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}
	init_args->is_init = true;
	SAFE_FREE(data);

skip_init:
	cfg->read = mp5990_read;
	return SENSOR_INIT_SUCCESS;
}
