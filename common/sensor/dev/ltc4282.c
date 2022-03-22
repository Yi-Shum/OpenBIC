#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sensor.h"
#include "hal_i2c.h"
#include "sensor_def.h"
#include "ltc4282.h"

static sys_slist_t priv_data_list = SYS_SLIST_STATIC_INIT(&priv_data_list);

typedef struct _ltc4282_priv_data {
	sys_snode_t node;
	uint8_t ID;
	float r_sense;
} ltc4282_priv_data;

uint8_t ltc4282_read(uint8_t sensor_num, int *reading)
{
	if ((reading == NULL) ||
	    (sensor_config[SensorNum_SensorCfg_map[sensor_num]].priv_data == NULL)) {
		return SENSOR_UNSPECIFIED_ERROR;
	}

	ltc4282_priv_data *priv_data =
		(ltc4282_priv_data *)sensor_config[SensorNum_SensorCfg_map[sensor_num]].priv_data;

	if (!priv_data->r_sense) {
		printf("%s, Rsense hasn't given\n", __func__);
		return SENSOR_UNSPECIFIED_ERROR;
	}

	float Rsense = priv_data->r_sense;
	uint8_t retry = 5;
	double val = 0;
	I2C_MSG msg = { 0 };

	sensor_cfg *cfg = &sensor_config[SensorNum_SensorCfg_map[sensor_num]];

	msg.bus = cfg->port;
	msg.slave_addr = cfg->slave_addr;
	msg.tx_len = 1;
	msg.data[0] = cfg->offset;
	msg.rx_len = 2;

	if (i2c_master_read(&msg, retry) != 0)
		return SENSOR_FAIL_TO_ACCESS;

	switch (cfg->offset) {
	case LTC4282_VSENSE_OFFSET:
		val = (((msg.data[0] << 8) | msg.data[1]) * 0.4 / 65535 / Rsense);
		break;
	case LTC4282_POWER_OFFSET:
		val = (((msg.data[0] << 8) | msg.data[1]) * 16.64 * 0.4 / 65535 / Rsense);
		break;
	case LTC4282_VSOURCE_OFFSET:
		val = (((msg.data[0] << 8) | msg.data[1]) * 16.64 / 65535);
		break;
	default:
		printf("Invalid sensor 0x%x offset 0x%x\n", sensor_num, cfg->offset);
		return SENSOR_NOT_FOUND;
	}

	sensor_val *sval = (sensor_val *)reading;
	memset(sval, 0, sizeof(*sval));

	sval->integer = (int)val & 0xFFFF;
	sval->fraction = (val - sval->integer) * 1000;

	return SENSOR_READ_SUCCESS;
}

uint8_t ltc4282_init(uint8_t sensor_num)
{
	if (!sensor_config[SensorNum_SensorCfg_map[sensor_num]].init_args) {
		printk("<error> ltc4282_init args not provide!\n");
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}

	ltc4282_init_arg *init_args =
		(ltc4282_init_arg *)sensor_config[SensorNum_SensorCfg_map[sensor_num]].init_args;
	if (init_args->is_init) {
		sys_snode_t *node;
		SYS_SLIST_FOR_EACH_NODE (&priv_data_list, node) {
			ltc4282_priv_data *p;
			p = CONTAINER_OF(node, ltc4282_priv_data, node);
			if (p->ID == init_args->ID)
				sensor_config[SensorNum_SensorCfg_map[sensor_num]].priv_data = p;
		}
	} else {
		/* allocate priv_data */
		ltc4282_priv_data *new_priv_data =
			(ltc4282_priv_data *)malloc(sizeof(ltc4282_priv_data));
		if (!new_priv_data) {
			printk("<error> ltc4282_init: adm1278_priv_data malloc fail\n");
			return SENSOR_INIT_UNSPECIFIED_ERROR;
		}
		/* set value of priv_data and append it to linked list */
		new_priv_data->ID = init_args->ID;
		new_priv_data->r_sense = init_args->r_sense;
		sys_slist_append(&priv_data_list, &new_priv_data->node);
		sensor_config[SensorNum_SensorCfg_map[sensor_num]].priv_data = new_priv_data;

		init_args->is_init = true;
	}
	sensor_config[SensorNum_SensorCfg_map[sensor_num]].read = ltc4282_read;
	return SENSOR_INIT_SUCCESS;
}
