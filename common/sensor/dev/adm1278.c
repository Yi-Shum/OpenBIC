#include <stdio.h>
#include <string.h>
#include "sensor.h"
#include "hal_i2c.h"

#define REG_PWR_MT_CFG 0xD4

static sys_slist_t priv_data_list = SYS_SLIST_STATIC_INIT();

typedef struct _adm1278_priv_data {
	sys_snode_t node;
	uint8_t index;
	float r_sense;
} adm1278_priv_data;

uint8_t adm1278_read(uint8_t sensor_num, int *reading)
{
	if ((reading == NULL) ||
	    (sensor_config[SensorNum_SensorCfg_map[sensor_num]].init_args == NULL)) {
		return SENSOR_UNSPECIFIED_ERROR;
	}

	adm1278_priv_data *priv_data =
		(adm1278_priv_data *)sensor_config[SensorNum_SensorCfg_map[sensor_num]].priv_data;

	if (!priv_data->r_sense) {
		printk("<error> adm1278_read: Rsense hasn't given\n");
		return SENSOR_UNSPECIFIED_ERROR;
	}

	float Rsense = priv_data->r_sense;
	float val;
	uint8_t retry = 5;
	I2C_MSG msg;

	msg.bus = sensor_config[SensorNum_SensorCfg_map[sensor_num]].port;
	msg.slave_addr = sensor_config[SensorNum_SensorCfg_map[sensor_num]].slave_addr;
	msg.tx_len = 1;
	msg.data[0] = sensor_config[SensorNum_SensorCfg_map[sensor_num]].offset;
	msg.rx_len = 2;

	if (i2c_master_read(&msg, retry))
		return SENSOR_FAIL_TO_ACCESS;

	switch (sensor_config[SensorNum_SensorCfg_map[sensor_num]].offset) {
	case PMBUS_READ_VIN:
		// m = +19599, b = 0, R = -2
		val = (float)(((msg.data[1] << 8) | msg.data[0]) * 100 / 19599);
		break;

	case PMBUS_READ_IOUT:
		// m = +800 * Rsense(mohm), b = +20475, R = -1
		val = (float)(((msg.data[1] << 8) | msg.data[0]) * 10 - 20475) / (800 * Rsense);
		break;

	case PMBUS_READ_TEMPERATURE_1:
		// m = +42, b = +31880, R = -1
		val = (float)(((msg.data[1] << 8) | msg.data[0]) * 10 - 31880) / 42;
		break;

	case PMBUS_READ_PIN:
		// m = +6123 * Rsense(mohm), b = 0, R = -2
		val = (float)(((msg.data[1] << 8) | msg.data[0]) * 100) / (6123 * Rsense);
		break;

	default:
		printf("<error> adm1278_read: Invalid offset 0x%x\n", sensor_num);
		return SENSOR_UNSPECIFIED_ERROR;
	}

	sensor_val *sval = (sensor_val *)reading;
	sval->integer = (int)val & 0xFFFF;
	sval->fraction = (val - sval->integer) * 1000;

	return SENSOR_READ_SUCCESS;
}

uint8_t adm1278_init(uint8_t sensor_num)
{
	if (!sensor_config[SensorNum_SensorCfg_map[sensor_num]].init_args) {
		printk("<error> adm1278_init: init args not provide!\n");
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}

	adm1278_init_arg *init_args =
		(adm1278_init_arg *)sensor_config[SensorNum_SensorCfg_map[sensor_num]].init_args;
	if (init_args->is_init) {
		sys_snode_t *node;
		SYS_SLIST_FOR_EACH_NODE (&priv_data_list, node) {
			adm1278_priv_data *p;
			p = CONTAINER_OF(node, adm1278_priv_data, node);
			if (p->index == init_args->index)
				sensor_config[SensorNum_SensorCfg_map[sensor_num]].priv_data = p;
		}
		goto skip_init;
	}

	uint8_t retry = 5;
	I2C_MSG msg;
	msg.bus = sensor_config[SensorNum_SensorCfg_map[sensor_num]].port;
	msg.slave_addr = sensor_config[SensorNum_SensorCfg_map[sensor_num]].slave_addr;
	msg.tx_len = 3;
	msg.data[0] = REG_PWR_MT_CFG;
	msg.data[1] = init_args->config.value & 0xFF;
	msg.data[2] = (init_args->config.value >> 8) & 0xFF;

	if (i2c_master_write(&msg, retry)) {
		printf("<error> adm1278_init: initail failed while i2c writing\n");
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}

	memset(&msg, 0, sizeof(msg));
	msg.bus = sensor_config[SensorNum_SensorCfg_map[sensor_num]].port;
	msg.slave_addr = sensor_config[SensorNum_SensorCfg_map[sensor_num]].slave_addr;
	msg.tx_len = 1;
	msg.data[0] = REG_PWR_MT_CFG;
	msg.rx_len = 2;

	if (i2c_master_read(&msg, retry)) {
		printf("<error> adm1278_init: initail failed while i2c reading\n");
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}

	if ((msg.data[0] != (init_args->config.value & 0xFF)) ||
	    (msg.data[1] != ((init_args->config.value >> 8) & 0xFF))) {
		printf("<error> adm1278_init: initail failed with wrong reading data\n");
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}

	/* allocate priv_data */
	adm1278_priv_data *new_priv_data =
		(adm1278_priv_data *)malloc(sizeof(adm1278_priv_data));
	if (!new_priv_data) {
		printk("<error> adm1278_init: adm1278_priv_data malloc fail\n");
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}
	/* set value of priv_data and append it to linked list */
	new_priv_data->index = init_args->index;
	new_priv_data->r_sense = init_args->r_sense;
	sys_slist_append(&priv_data_list, &new_priv_data->node);
	sensor_config[SensorNum_SensorCfg_map[sensor_num]].priv_data = new_priv_data;

	init_args->is_init = true;

skip_init:
	/* NOTE: read function should given after init success! */
	sensor_config[SensorNum_SensorCfg_map[sensor_num]].read = adm1278_read;
	return SENSOR_INIT_SUCCESS;
}
