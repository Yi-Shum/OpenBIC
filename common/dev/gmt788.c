#include <stdio.h>
#include <stdlib.h>
#include "sensor.h"
#include "hal_i2c.h"

uint8_t gmt788_read(uint8_t sensor_num, int *reading)
{
	if (!reading || (sensor_num > SENSOR_NUM_MAX)) {
		return SENSOR_UNSPECIFIED_ERROR;
	}

	uint8_t retry = 5;
	I2C_MSG msg = { 0 };
	uint16_t integer = 0, fraction = 0;

	msg.bus = sensor_config[sensor_config_index_map[sensor_num]].port;
	msg.target_addr = sensor_config[sensor_config_index_map[sensor_num]].target_addr;
	msg.tx_len = 1;
	msg.rx_len = 1;
	uint8_t offset = sensor_config[sensor_config_index_map[sensor_num]].offset;

	if (offset == GMT788_LOCAL_TEMPERATRUE) {
		msg.data[0] = GMT788_LOCAL_TEMPERATRUE;
		if (i2c_master_read(&msg, retry)) {
			return SENSOR_FAIL_TO_ACCESS;
		}
		integer = msg.data[0];
	} else if (offset == GMT788_REMOTE_TEMPERATRUE) {
		msg.data[0] = GMT788_REMOTE_TEMPERATRUE;
		if (i2c_master_read(&msg, retry)) {
			return SENSOR_FAIL_TO_ACCESS;
		}
		integer = msg.data[0];
		msg.data[0] = GMT788_REMOTE_TEMPERATRUE_EXT;
		if (i2c_master_read(&msg, retry)) {
			return SENSOR_FAIL_TO_ACCESS;
		}
		fraction = 125 * (msg.data[0] >> 5);
	} else {
		printf("[%s] Unknown offset(%d)\n", __func__, offset);
		return SENSOR_UNSPECIFIED_ERROR;
	}

	sensor_val *sval = (sensor_val *)reading;
	sval->integer = (int16_t)integer;
	sval->fraction = fraction;
	return SENSOR_READ_SUCCESS;
}

uint8_t gmt788_init(uint8_t sensor_num)
{
	if (sensor_num > SENSOR_NUM_MAX) {
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}
	sensor_config[sensor_config_index_map[sensor_num]].read = gmt788_read;
	return SENSOR_INIT_SUCCESS;
}
