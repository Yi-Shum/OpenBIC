#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sensor.h"
#include "plat_def.h"
#ifdef ENABLE_APML
#include "apml.h"

typedef struct _dimm_temp_priv_data {
	float ts0_temp;
} dimm_temp_priv_data;

void apml_read_fail_cb(apml_msg *msg)
{
	if ((msg == NULL) || (msg->ptr_arg == NULL)) {
		return;
	}
	sensor_cfg *cfg = (sensor_cfg *)msg->ptr_arg;
	cfg->cache_status = SENSOR_UNSPECIFIED_ERROR;
}

void cpu_power_write(apml_msg *msg)
{
	if ((msg == NULL) || (msg->ptr_arg == NULL)) {
		return;
	}
	sensor_cfg *cfg = (sensor_cfg *)msg->ptr_arg;
	mailbox_RdData *rddata = (mailbox_RdData *)msg->RdData;
	if (rddata->error_code != SBRMI_MAILBOX_NO_ERR) {
		printf("[%s] Read cpu power failed, sensor number 0x%x, error code %d\n", __func__,
		       cfg->num, rddata->error_code);
		cfg->cache_status = SENSOR_UNSPECIFIED_ERROR;
		return;
	}

	uint32_t raw_data = (rddata->data_out[3] << 24) | (rddata->data_out[2] << 16) |
			    (rddata->data_out[1] << 8) | rddata->data_out[0];
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
	mailbox_RdData *rddata = (mailbox_RdData *)msg->RdData;
	if (rddata->error_code != SBRMI_MAILBOX_NO_ERR) {
		printf("[%s] Read dimm power failed, sensor number 0x%x, error code %d\n", __func__,
		       cfg->num, rddata->error_code);
		cfg->cache_status = SENSOR_UNSPECIFIED_ERROR;
		return;
	}
	uint16_t raw_data = (rddata->data_out[3] << 7) | (rddata->data_out[2] >> 1);
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
	mailbox_RdData *rddata = (mailbox_RdData *)msg->RdData;
	if (rddata->error_code != SBRMI_MAILBOX_NO_ERR) {
		printf("[%s] Read dimm temperature failed, sensor number 0x%x, error code %d\n",
		       __func__, cfg->num, rddata->error_code);
		cfg->cache_status = SENSOR_UNSPECIFIED_ERROR;
		return;
	}
	uint16_t raw_data = (rddata->data_out[3] << 3) | (rddata->data_out[2] >> 5);
	float temp = 0.25 * raw_data;
	bool is_ts1 = rddata->data_out[0] & 0x40;
	if (is_ts1) {
		float *TS0_temp = &(((dimm_temp_priv_data *)(cfg->priv_data))->ts0_temp);
		if (temp < *TS0_temp) {
			temp = *TS0_temp;
		}
		*TS0_temp = 0;
		sensor_val sval;
		sval.integer = (int16_t)temp;
		sval.fraction = (temp - sval.integer) * 1000;
		memcpy(&cfg->cache, &sval, sizeof(sensor_val));
		cfg->cache_status = SENSOR_READ_4BYTE_ACUR_SUCCESS;
	} else {
		if (cfg->priv_data) {
			float *TS0_temp = &(((dimm_temp_priv_data *)(cfg->priv_data))->ts0_temp);
			*TS0_temp = temp;
		} else {
			printf("[%s] private data is NULL!\n", __func__);
		}
	}
}

uint8_t apml_mailbox_read(uint8_t sensor_num, int *reading)
{
	if (reading == NULL || (sensor_num > SENSOR_NUM_MAX)) {
		return SENSOR_UNSPECIFIED_ERROR;
	}

	sensor_cfg *cfg = &sensor_config[sensor_config_index_map[sensor_num]];
	apml_mailbox_init_arg *init_arg = (apml_mailbox_init_arg *)cfg->init_args;

	apml_msg mailbox_msg;
	memset(&mailbox_msg, 0, sizeof(apml_msg));
	mailbox_msg.msg_type = APML_MSG_TYPE_MAILBOX;
	mailbox_msg.bus = cfg->port;
	mailbox_msg.target_addr = cfg->target_addr;
	mailbox_msg.error_cb_fn = apml_read_fail_cb;
	mailbox_msg.ptr_arg = cfg;

	mailbox_WrData *wrdata = (mailbox_WrData *)mailbox_msg.WrData;
	wrdata->command = cfg->offset;

	switch (cfg->offset) {
	case SBRMI_MAILBOX_PKGPWR:
		mailbox_msg.cb_fn = cpu_power_write;
		apml_read(&mailbox_msg);
		break;

	case SBRMI_MAILBOX_GET_DIMM_PWR:
		mailbox_msg.cb_fn = dimm_pwr_write;
		wrdata->data_in[0] = init_arg->data & 0xFF;
		apml_read(&mailbox_msg);
		break;

	case SBRMI_MAILBOX_GET_DIMM_TEMP:
		mailbox_msg.cb_fn = dimm_temp_write;
		wrdata->data_in[0] = init_arg->data & 0xFF;
		apml_read(&mailbox_msg);

		mailbox_msg.cb_fn = dimm_temp_write;
		wrdata->data_in[0] = (init_arg->data & 0xFF) | 0x40;
		apml_read(&mailbox_msg);
		break;

	default:
		break;
	}

	return cfg->cache_status;
}

uint8_t apml_mailbox_init(uint8_t sensor_num)
{
	if (sensor_num > SENSOR_NUM_MAX) {
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}
	sensor_cfg *cfg = &sensor_config[sensor_config_index_map[sensor_num]];

	if (cfg->offset == SBRMI_MAILBOX_GET_DIMM_TEMP) {
		cfg->priv_data = malloc(sizeof(dimm_temp_priv_data));
		if (cfg->priv_data == NULL) {
			printf("[%s] Allocate private data failed.\n", __func__);
			return SENSOR_INIT_UNSPECIFIED_ERROR;
		}
	}
	cfg->read = apml_mailbox_read;
	return SENSOR_INIT_SUCCESS;
}
#endif
