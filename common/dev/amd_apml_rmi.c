#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sensor.h"
#include "apml.h"

void cpu_power_write(apml_msg *msg)
{
	if ((msg == NULL) || (msg->ptr_arg == NULL)) {
		return;
	}
	sensor_cfg *cfg = (sensor_cfg *)msg->ptr_arg;
	mailbox_msg *mb_msg = &msg->data.mailbox;
	if (mb_msg->error_code != SBRMI_MAILBOX_NO_ERR) {
		printf("[%s] Read cpu power fail, error code %d\n", __func__, mb_msg->error_code);
		cfg->cache_status = SENSOR_UNSPECIFIED_ERROR;
		return;
	}

	uint32_t raw_data = (mb_msg->data_out[3] << 24) | (mb_msg->data_out[2] << 16) |
			    (mb_msg->data_out[1] << 8) | mb_msg->data_out[0];
	sensor_val sval;
	sval.integer = raw_data / 1000;
	sval.fraction = raw_data % 1000;
	memcpy(&cfg->cache, &sval, sizeof(sensor_val));
	cfg->cache_status = SENSOR_READ_4BYTE_ACUR_SUCCESS;
}

void dimm_pwr_write(apml_msg *msg)
{
	if ((msg == NULL) || (msg->ptr_arg == NULL)) {
		return;
	}
	sensor_cfg *cfg = (sensor_cfg *)msg->ptr_arg;
	mailbox_msg *mb_msg = &msg->data.mailbox;
	if (mb_msg->error_code != SBRMI_MAILBOX_NO_ERR) {
		printf("[%s] Read cpu power fail, error code %d\n", __func__, mb_msg->error_code);
		cfg->cache_status = SENSOR_UNSPECIFIED_ERROR;
		return;
	}
	uint16_t raw_data = (mb_msg->data_out[3] << 7) | (mb_msg->data_out[2] >> 1);
	sensor_val sval;
	sval.integer = raw_data / 1000;
	sval.fraction = raw_data % 1000;
	memcpy(&cfg->cache, &sval, sizeof(sensor_val));
	cfg->cache_status = SENSOR_READ_4BYTE_ACUR_SUCCESS;
}

void dimm_temp_write(apml_msg *msg)
{
	if ((msg == NULL) || (msg->ptr_arg == NULL)) {
		return;
	}
	sensor_cfg *cfg = (sensor_cfg *)msg->ptr_arg;
	mailbox_msg *mb_msg = &msg->data.mailbox;
	if (mb_msg->error_code != SBRMI_MAILBOX_NO_ERR) {
		printf("[%s] Read cpu power fail, error code %d\n", __func__, mb_msg->error_code);
		cfg->cache_status = SENSOR_UNSPECIFIED_ERROR;
		return;
	}
	uint16_t raw_data = (mb_msg->data_out[3] << 3) | (mb_msg->data_out[2] >> 5);
	float temp = 0.25 * raw_data;
	sensor_val sval;
	sval.integer = (int16_t)temp;
	sval.fraction = (temp - sval.integer) * 1000;
	memcpy(&cfg->cache, &sval, sizeof(sensor_val));
	cfg->cache_status = SENSOR_READ_4BYTE_ACUR_SUCCESS;
}

uint8_t amd_apml_rmi_read(uint8_t sensor_num, int *reading)
{
	if (reading == NULL || (sensor_num > SENSOR_NUM_MAX)) {
		return SENSOR_UNSPECIFIED_ERROR;
	}

	sensor_cfg *cfg = &sensor_config[sensor_config_index_map[sensor_num]];
	amd_apml_rmi_init_arg *init_arg = (amd_apml_rmi_init_arg *)cfg->init_args;

	apml_msg pwr_read_msg;
	mailbox_msg *mailbox_data;
	pwr_read_msg.msg_type = APML_MSG_TYPE_MAILBOX;
	pwr_read_msg.ptr_arg = cfg;

	switch (cfg->offset) {
	case SBRMI_MAILBOX_PKGPWR:
		pwr_read_msg.cb_fn = cpu_power_write;
		mailbox_data = &pwr_read_msg.data.mailbox;
		mailbox_data->bus = cfg->port;
		mailbox_data->target_addr = cfg->target_addr;
		mailbox_data->command = SBRMI_MAILBOX_PKGPWR;
		memset(mailbox_data->data_in, 0, 4);
		apml_read(&pwr_read_msg);
		break;

	case SBRMI_MAILBOX_GET_DIMM_PWR:
		pwr_read_msg.cb_fn = dimm_pwr_write;
		mailbox_data = &pwr_read_msg.data.mailbox;
		mailbox_data->bus = cfg->port;
		mailbox_data->target_addr = cfg->target_addr;
		mailbox_data->command = cfg->offset;
		memset(mailbox_data->data_in, 0, 4);
		mailbox_data->data_in[0] = init_arg->data & 0xFF;
		apml_read(&pwr_read_msg);
		break;

	case SBRMI_MAILBOX_GET_DIMM_TEMP:
		pwr_read_msg.cb_fn = dimm_temp_write;
		mailbox_data = &pwr_read_msg.data.mailbox;
		mailbox_data->bus = cfg->port;
		mailbox_data->target_addr = cfg->target_addr;
		mailbox_data->command = cfg->offset;
		memset(mailbox_data->data_in, 0, 4);
		mailbox_data->data_in[0] = init_arg->data & 0xFF;
		apml_read(&pwr_read_msg);
		break;

	default:
		break;
	}

	return cfg->cache_status;
}

uint8_t amd_apml_rmi_init(uint8_t sensor_num)
{
	if (sensor_num > SENSOR_NUM_MAX) {
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}

	sensor_config[sensor_config_index_map[sensor_num]].read = amd_apml_rmi_read;
	return SENSOR_INIT_SUCCESS;
}
