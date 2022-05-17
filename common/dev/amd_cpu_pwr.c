#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sensor.h"
#include "apml.h"

#define READ_ORDER_BIT 5

void amd_cpu_pwr_write(apml_msg *msg)
{
	if ((msg != NULL) || (msg->ptr_arg != NULL)) {
		return;
	}
	sensor_cfg *cfg = (sensor_cfg *)msg->ptr_arg;
	mailbox_msg *mb_msg = &msg->data.mailbox;
	uint32_t raw_data = (mb_msg->data_out[3] << 24) | (mb_msg->data_out[2] << 16) |
			    (mb_msg->data_out[1] << 8) | mb_msg->data_out[0];

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
	pwr_read_msg.msg_type = APML_MSG_TYPE_MAILBOX;
	pwr_read_msg.cb_fn = amd_cpu_pwr_write;
	pwr_read_msg.ptr_arg = cfg;

	mailbox_msg *mailbox_data = &pwr_read_msg.data.mailbox;
	mailbox_data->bus = cfg->port;
	mailbox_data->target_addr = cfg->target_addr;
	mailbox_data->command = SBRMI_MAILBOX_PKGPWR;
	memset(mailbox_data->data_in, 0, 4);
	apml_read(&pwr_read_msg);

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
