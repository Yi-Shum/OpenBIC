#include <stdio.h>
#include <string.h>
#include "sensor.h"
#include "hal_i2c.h"

#define READ_ORDER_BIT 5

enum {
	TSI_CPU_TEMP_INT = 0x01,
	TSI_CONFIG = 0x03,
	TSI_CPU_TEMP_DEC = 0x10,
};

uint8_t amd_cpu_temp_read(uint8_t sensor_num, int *reading)
{
	if (reading == NULL || (sensor_num > SENSOR_NUM_MAX)) {
		return SENSOR_UNSPECIFIED_ERROR;
	}

	uint8_t retry = 5;
	sensor_val *sval = (sensor_val *)reading;
	sensor_cfg *cfg = &sensor_config[sensor_config_index_map[sensor_num]];
	I2C_MSG msg;
	memset(sval, 0, sizeof(sensor_val));

	/* check read order */
	msg.bus = cfg->port;
	msg.target_addr = cfg->target_addr;
	msg.tx_len = 1;
	msg.rx_len = 1;
	msg.data[0] = TSI_CONFIG;

	if (i2c_master_read(&msg, retry)) {
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}

	bool read_order = msg.data[0] && BIT(READ_ORDER_BIT);

	/* Atomic Read Mechanism
	 * if read order is 0, read integer first and it will latch decimal until next read of integer part.
	 * if read order is 1, read order is reversed. 
	 */
	msg.bus = cfg->port;
	msg.target_addr = cfg->target_addr;
	msg.tx_len = 1;
	msg.rx_len = 1;
	msg.data[0] = (read_order == 0) ? TSI_CPU_TEMP_INT : TSI_CPU_TEMP_DEC;

	if (i2c_master_read(&msg, retry)) {
		return SENSOR_FAIL_TO_ACCESS;
	}
	uint8_t first_val = msg.data[0];

	msg.bus = cfg->port;
	msg.target_addr = cfg->target_addr;
	msg.tx_len = 1;
	msg.rx_len = 1;
	msg.data[0] = (read_order == 0) ? TSI_CPU_TEMP_DEC : TSI_CPU_TEMP_INT;

	if (i2c_master_read(&msg, retry)) {
		return SENSOR_FAIL_TO_ACCESS;
	}
	uint8_t second_val = msg.data[0];

	if (read_order == 0) {
		sval->integer = first_val;
		sval->fraction = (second_val >> 5) * 125;
	} else {
		sval->integer = second_val;
		sval->fraction = (first_val >> 5) * 125;
	}

	return SENSOR_READ_SUCCESS;
}

uint8_t amd_cpu_temp_init(uint8_t sensor_num)
{
	if (sensor_num > SENSOR_NUM_MAX) {
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}

	sensor_config[sensor_config_index_map[sensor_num]].read = amd_cpu_temp_read;
	return SENSOR_INIT_SUCCESS;
}
