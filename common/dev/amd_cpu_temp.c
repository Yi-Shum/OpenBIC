#include <stdio.h>
#include <string.h>
#include "sensor.h"
#include "apml.h"

#define READ_ORDER_BIT 5

uint8_t amd_cpu_temp_read(uint8_t sensor_num, int *reading)
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

	uint8_t first_val;
	if (TSI_read(cfg->port, cfg->target_addr,
		     (read_order == 0) ? SBTSI_CPU_TEMP_INT : SBTSI_CPU_TEMP_DEC, &first_val)) {
		return SENSOR_UNSPECIFIED_ERROR;
	}

	uint8_t second_val;
	if (TSI_read(cfg->port, cfg->target_addr,
		     (read_order == 0) ? SBTSI_CPU_TEMP_DEC : SBTSI_CPU_TEMP_INT, &second_val)) {
		return SENSOR_UNSPECIFIED_ERROR;
	}

	sensor_val *sval = (sensor_val *)reading;
	memset(sval, 0, sizeof(sensor_val));
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
