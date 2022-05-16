#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sensor.h"
#include "apml.h"

#define READ_ORDER_BIT 5

void amd_cpu_pwr_write(mailbox_msg *msg)
{
	if (msg != NULL && msg->cb_arg != NULL) {
		return;
	}
	sensor_cfg *cfg = (sensor_cfg *)msg->cb_arg;
	uint32_t raw_data = (msg->data_out[3] << 24) | (msg->data_out[2] << 16) |
			    (msg->data_out[1] << 8) | msg->data_out[0];

	sensor_val sval;
	sval.integer = raw_data / 1000;
	sval.fraction = raw_data % 1000;

	memcpy(&cfg->cache, &sval, sizeof(sensor_val));
	cfg->cache_status = SENSOR_READ_4BYTE_ACUR_SUCCESS;
}

uint8_t amd_cpu_pwr_read(uint8_t sensor_num, int *reading)
{
	if (reading == NULL || (sensor_num > SENSOR_NUM_MAX)) {
		return SENSOR_UNSPECIFIED_ERROR;
	}

	sensor_cfg *cfg = &sensor_config[sensor_config_index_map[sensor_num]];

	apml_msg pwr_read_msg;
	pwr_read_msg.msg_type = APML_MAILBOX;

	mailbox_msg *mailbox_data = &pwr_read_msg.data.mailbox;
	mailbox_data->bus = cfg->port;
	mailbox_data->target_addr = cfg->target_addr;
	mailbox_data->command = SBRMI_MAILBOX_PKGPWR;
	memset(mailbox_data->data_in, 0, 4);
	mailbox_data->cb_fn = amd_cpu_pwr_write;
	mailbox_data->cb_arg = cfg;

	return SENSOR_READ_SUCCESS;
}

uint8_t amd_cpu_pwr_init(uint8_t sensor_num)
{
	if (sensor_num > SENSOR_NUM_MAX) {
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}

	sensor_config[sensor_config_index_map[sensor_num]].read = amd_cpu_pwr_read;
	return SENSOR_INIT_SUCCESS;
}
