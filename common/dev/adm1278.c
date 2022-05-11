#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sensor.h"
#include "hal_i2c.h"
#include "pmbus.h"

#define REG_PWR_MT_CFG 0xD4

#define ADM1278_EIN_ROLLOVER_CNT_MAX 0x10000
#define ADM1278_EIN_SAMPLE_CNT_MAX 0x1000000
#define ADM1278_EIN_ENERGY_CNT_MAX 0x800000

static sys_slist_t priv_data_list = SYS_SLIST_STATIC_INIT(&priv_data_list);

typedef struct _adm1278_priv_data {
	sys_snode_t node;
	uint8_t ID;
	float r_sense;
} adm1278_priv_data;

int adm1278_read_ein_ext(float rsense, float *val, uint8_t sensor_num)
{
	if ((val == NULL) || (sensor_num > SENSOR_NUM_MAX)) {
		return -1;
	}
	I2C_MSG msg;
	uint8_t retry = 5;
	uint32_t energy = 0, rollover = 0, sample = 0;
	uint32_t pre_energy = 0, pre_rollover = 0, pre_sample = 0;
	uint32_t sample_diff = 0;
	double energy_diff = 0;
	static uint32_t last_energy = 0, last_rollover = 0, last_sample = 0;
	static bool pre_ein = false;

	sensor_cfg *cfg = &sensor_config[sensor_config_index_map[sensor_num]];
	msg.bus = cfg->port;
	msg.target_addr = cfg->target_addr;
	msg.tx_len = 1;
	msg.data[0] = cfg->offset;
	msg.rx_len = 8;

	if (i2c_master_read(&msg, retry))
		return SENSOR_FAIL_TO_ACCESS;

	//record the previous data
	pre_energy = last_energy;
	pre_rollover = last_rollover;
	pre_sample = last_sample;

	//record the current data
	last_energy = energy = (msg.data[2] << 16) | (msg.data[1] << 8) | msg.data[0];
	last_rollover = rollover = (msg.data[4] << 8) | msg.data[3];
	last_sample = sample = (msg.data[7] << 16) | (msg.data[6] << 8) | msg.data[5];

	//return since data isn't enough
	if (pre_ein == false) {
		pre_ein = true;
		return -1;
	}

	if ((pre_rollover > rollover) || ((pre_rollover == rollover) && (pre_energy > energy))) {
		rollover += ADM1278_EIN_ROLLOVER_CNT_MAX;
	}

	if (pre_sample > sample) {
		sample += ADM1278_EIN_SAMPLE_CNT_MAX;
	}

	energy_diff = (double)(rollover - pre_rollover) * ADM1278_EIN_ENERGY_CNT_MAX +
		      (double)energy - (double)pre_energy;
	if (energy_diff < 0) {
		return -1;
	}

	sample_diff = sample - pre_sample;
	if (sample_diff == 0) {
		return -1;
	}

	*val = (float)((energy_diff / sample_diff / 256) * 100) / (6123 * rsense);

	return 0;
}

uint8_t adm1278_read(uint8_t sensor_num, int *reading)
{
	if ((reading == NULL) || (sensor_num > SENSOR_NUM_MAX) ||
	    (sensor_config[sensor_config_index_map[sensor_num]].priv_data == NULL)) {
		return SENSOR_UNSPECIFIED_ERROR;
	}

	sensor_cfg *cfg = &sensor_config[sensor_config_index_map[sensor_num]];
	adm1278_priv_data *priv_data = (adm1278_priv_data *)cfg->priv_data;

	float Rsense = priv_data->r_sense;
	float val;
	uint8_t retry = 5;
	I2C_MSG msg;
	int ret = 0;
	uint8_t offset = cfg->offset;

	if (offset != ADM1278_EIN_EXT_OFFSET) {
		msg.bus = cfg->port;
		msg.target_addr = cfg->target_addr;
		msg.tx_len = 1;
		msg.data[0] = offset;
		msg.rx_len = 2;

		if (i2c_master_read(&msg, retry))
			return SENSOR_FAIL_TO_ACCESS;
	}

	switch (offset) {
	case PMBUS_READ_VIN:
		// m = +19599, b = 0, R = -2
		val = (float)((msg.data[1] << 8) | msg.data[0]) * 100 / 19599;
		break;

	case PMBUS_READ_IOUT:
	case ADM1278_PEAK_IOUT_OFFSET:
		// m = +800 * Rsense(mohm), b = +20475, R = -1
		val = (float)(((msg.data[1] << 8) | msg.data[0]) * 10 - 20475) / (800 * Rsense);
		break;

	case PMBUS_READ_TEMPERATURE_1:
		// m = +42, b = +31880, R = -1
		val = (float)(((msg.data[1] << 8) | msg.data[0]) * 10 - 31880) / 42;
		break;

	case PMBUS_READ_PIN:
	case ADM1278_PEAK_PIN_OFFSET:
		// m = +6123 * Rsense(mohm), b = 0, R = -2
		val = (float)(((msg.data[1] << 8) | msg.data[0]) * 100) / (6123 * Rsense);
		break;

	case ADM1278_EIN_EXT_OFFSET:
		ret = adm1278_read_ein_ext(Rsense, &val, sensor_num);
		if (ret != 0) {
			return SENSOR_UNSPECIFIED_ERROR;
		}
		break;

	default:
		printf("Invalid sensor 0x%x\n", sensor_num);
		return SENSOR_UNSPECIFIED_ERROR;
	}

	sensor_val *sval = (sensor_val *)reading;
	sval->integer = (int)val & 0xFFFF;
	sval->fraction = (val - sval->integer) * 1000;
	return SENSOR_READ_SUCCESS;
}

uint8_t adm1278_init(uint8_t sensor_num)
{
	if (sensor_num > SENSOR_NUM_MAX) {
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}

	sensor_cfg *cfg = &sensor_config[sensor_config_index_map[sensor_num]];

	if (!cfg->init_args) {
		printf("<error> ADM1278 init args not provide!\n");
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}

	adm1278_init_arg *init_args = (adm1278_init_arg *)cfg->init_args;
	if (init_args->is_init) {
		sys_snode_t *node;
		SYS_SLIST_FOR_EACH_NODE (&priv_data_list, node) {
			adm1278_priv_data *p;
			p = CONTAINER_OF(node, adm1278_priv_data, node);
			if (p->ID == init_args->ID)
				cfg->priv_data = p;
		}
		goto skip_init;
	}

	uint8_t retry = 5;
	I2C_MSG msg;
	msg.bus = cfg->port;
	msg.target_addr = cfg->target_addr;
	msg.tx_len = 3;
	msg.data[0] = REG_PWR_MT_CFG;
	msg.data[1] = init_args->config.value & 0xFF;
	msg.data[2] = (init_args->config.value >> 8) & 0xFF;

	if (i2c_master_write(&msg, retry)) {
		printf("<error> ADM1278 initial failed while i2c writing\n");
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}

	memset(&msg, 0, sizeof(msg));
	msg.bus = cfg->port;
	msg.target_addr = cfg->target_addr;
	msg.tx_len = 1;
	msg.data[0] = REG_PWR_MT_CFG;
	msg.rx_len = 2;

	if (i2c_master_read(&msg, retry)) {
		printf("<error> ADM1278 initial failed while i2c reading\n");
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}

	if ((msg.data[0] != (init_args->config.value & 0xFF)) ||
	    (msg.data[1] != ((init_args->config.value >> 8) & 0xFF))) {
		printf("<error> ADM1278 initial failed with wrong reading data\n");
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}

	/* allocate priv_data */
	adm1278_priv_data *new_priv_data = (adm1278_priv_data *)malloc(sizeof(adm1278_priv_data));
	if (!new_priv_data) {
		printk("<error> adm1278_init: adm1278_priv_data malloc fail\n");
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}
	/* set value of priv_data and append it to linked list */
	new_priv_data->ID = init_args->ID;
	new_priv_data->r_sense = init_args->r_sense;
	sys_slist_append(&priv_data_list, &new_priv_data->node);
	cfg->priv_data = new_priv_data;

	init_args->is_init = true;

skip_init:
	cfg->read = adm1278_read;
	return SENSOR_INIT_SUCCESS;
}
