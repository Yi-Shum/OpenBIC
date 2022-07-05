#include <stdio.h>
#include <string.h>
#include "sensor.h"
#include "apml.h"

#define READ_ORDER_BIT 5

uint8_t amd_tsi_read(uint8_t sensor_num, int *reading)
{
	if (reading == NULL || (sensor_num > SENSOR_NUM_MAX)) {
		return SENSOR_UNSPECIFIED_ERROR;
	}

	sensor_cfg *cfg = &sensor_config[sensor_config_index_map[sensor_num]];

	/* check read order */
	uint8_t read_val;
	if (TSI_read(cfg->port, cfg->target_addr, SBTSI_CONFIG, &read_val)) {
		return SENSOR_UNSPECIFIED_ERROR;
	}

	bool read_order = (read_val & BIT(READ_ORDER_BIT)) ? 1 : 0;

	/* Atomic Read Mechanism
	 * if read order is 0, read integer first and it will latch decimal until next read of integer part.
	 * if read order is 1, read order is reversed. 
	 */
	uint8_t integer, decimal;
	if (read_order) {
		if (TSI_read(cfg->port, cfg->target_addr, SBTSI_CPU_TEMP_DEC, &decimal) ||
		    TSI_read(cfg->port, cfg->target_addr, SBTSI_CPU_TEMP_INT, &integer)) {
			return SENSOR_UNSPECIFIED_ERROR;
		}
	} else {
		if (TSI_read(cfg->port, cfg->target_addr, SBTSI_CPU_TEMP_INT, &integer) ||
		    TSI_read(cfg->port, cfg->target_addr, SBTSI_CPU_TEMP_DEC, &decimal)) {
			return SENSOR_UNSPECIFIED_ERROR;
		}
	}

	sensor_val *sval = (sensor_val *)reading;
	memset(sval, 0, sizeof(sensor_val));
	sval->integer = integer;
	sval->fraction = (decimal >> 5) * 125;

	return SENSOR_READ_SUCCESS;
}

uint8_t amd_tsi_init(uint8_t sensor_num)
{
	if (sensor_num > SENSOR_NUM_MAX) {
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}

	sensor_config[sensor_config_index_map[sensor_num]].read = amd_tsi_read;
	return SENSOR_INIT_SUCCESS;
}
