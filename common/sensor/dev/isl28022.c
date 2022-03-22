#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sensor.h"
#include "hal_i2c.h"
#include "isl28022.h"

static sys_slist_t priv_data_list = SYS_SLIST_STATIC_INIT(&priv_data_list);

typedef struct _isl28022_priv_data {
	sys_snode_t node;
	uint8_t ID;
	float current_LSB;
	union {
		uint16_t value;
		struct {
			uint16_t MODE : 3;
			uint16_t SADC : 4;
			uint16_t BADC : 4;
			uint16_t PG : 2;
			uint16_t BRNG : 2;
			uint16_t RST : 1;
		} fields;
	} config;
} isl28022_priv_data;

uint8_t isl28022_read(uint8_t sensor_num, int *reading)
{
	if ((reading == NULL) ||
	    (sensor_config[SensorNum_SensorCfg_map[sensor_num]].priv_data == NULL)) {
		return SENSOR_UNSPECIFIED_ERROR;
	}

	uint8_t retry = 5;
	I2C_MSG msg;
	isl28022_priv_data *priv_data =
		sensor_config[SensorNum_SensorCfg_map[sensor_num]].priv_data;
	uint8_t offset = sensor_config[SensorNum_SensorCfg_map[sensor_num]].offset;
	sensor_val *sval = (sensor_val *)reading;
	memset(sval, 0, sizeof(sensor_val));

	msg.bus = sensor_config[SensorNum_SensorCfg_map[sensor_num]].port;
	msg.slave_addr = sensor_config[SensorNum_SensorCfg_map[sensor_num]].slave_addr;
	msg.tx_len = 1;
	msg.rx_len = 2;
	msg.data[0] = offset;
	if (i2c_master_read(&msg, retry)) {
		/* read fail */
		return SENSOR_FAIL_TO_ACCESS;
	}

	if (offset == ISL28022_BUS_VOLTAGE_REG) {
		/* unsigned */
		uint16_t read_mv;

		if ((priv_data->config.fields.BRNG == 0b11) ||
		    (priv_data->config.fields.BRNG == 0b10)) {
			read_mv = ((msg.data[0] << 6) | (msg.data[1] >> 2)) * 4;
		} else if (priv_data->config.fields.BRNG == 0b01) {
			read_mv = ((msg.data[0] << 5) | (msg.data[1] >> 3)) * 4;
		} else {
			read_mv = (((msg.data[0] & BIT_MASK(7)) << 5) | (msg.data[1] >> 3)) * 4;
		}
		sval->integer = read_mv / 1000;
		sval->fraction = read_mv % 1000;
	} else if (offset == ISL28022_CURRENT_REG) {
		/* signed */
		float read_current =
			((int16_t)(msg.data[0] << 8) | msg.data[1]) * priv_data->current_LSB;
		sval->integer = read_current;
		sval->fraction = (read_current - sval->integer) * 1000;
	} else if (offset == ISL28022_POWER_REG) {
		/* unsigned */
		float read_power = ((msg.data[0] << 8) | msg.data[1]) * priv_data->current_LSB * 20;
		/* if BRNG is set to 60V, power	is multiplied to 2 */
		if ((priv_data->config.fields.BRNG == 0b11) ||
		    (priv_data->config.fields.BRNG == 0b10)) {
			read_power *= 2;
		}
		sval->integer = read_power;
		sval->fraction = ((read_power - sval->integer) * 1000);
	} else {
		return SENSOR_FAIL_TO_ACCESS;
	}

	return SENSOR_READ_SUCCESS;
}

uint8_t isl28022_init(uint8_t sensor_num)
{
	if (sensor_config[SensorNum_SensorCfg_map[sensor_num]].init_args == NULL) {
		printk("isl28022_init: init_arg is NULL\n");
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}

	isl28022_init_arg *init_arg =
		(isl28022_init_arg *)sensor_config[SensorNum_SensorCfg_map[sensor_num]].init_args;

	if (init_arg->is_init) {
		sys_snode_t *node;
		isl28022_priv_data *p;
		SYS_SLIST_FOR_EACH_NODE (&priv_data_list, node) {
			p = CONTAINER_OF(node, isl28022_priv_data, node);
			if (p->ID == init_arg->ID) {
				sensor_config[SensorNum_SensorCfg_map[sensor_num]].priv_data = p;
			}
		}
		goto skip_init;
	}

	I2C_MSG msg;
	uint8_t retry = 5;

	/* set configuration register */
	msg.bus = sensor_config[SensorNum_SensorCfg_map[sensor_num]].port;
	msg.slave_addr = sensor_config[SensorNum_SensorCfg_map[sensor_num]].slave_addr;
	msg.tx_len = 3;
	msg.data[0] = ISL28022_CONFIG_REG;
	msg.data[1] = (init_arg->config.value >> 8) & 0xFF;
	msg.data[2] = init_arg->config.value & 0xFF;
	if (i2c_master_write(&msg, retry)) {
		printk("isl28022_init, set configuration register fail\n");
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}

	/* calculate and set calibration */
	uint16_t v_shunt_fs, adc_res, calibration;
	float current_LSB;

	v_shunt_fs = 40 << (init_arg->config.fields.PG);
	if (!(init_arg->config.fields.SADC & BIT(3)) &&
	    ((init_arg->config.fields.SADC & BIT_MASK(2)) < 3)) {
		adc_res = 1 << (12 + (init_arg->config.fields.SADC & BIT_MASK(2)));
	} else {
		adc_res = 32768;
	}
	current_LSB = (float)v_shunt_fs / (init_arg->r_shunt * adc_res);
	calibration = (40.96 / (current_LSB * init_arg->r_shunt));
	calibration = calibration << 1; /* 15 bits, bit[0] is fix to 0 */

	msg.bus = sensor_config[SensorNum_SensorCfg_map[sensor_num]].port;
	msg.slave_addr = sensor_config[SensorNum_SensorCfg_map[sensor_num]].slave_addr;
	msg.tx_len = 3;
	msg.data[0] = ISL28022_CALIBRATION_REG;
	msg.data[1] = (calibration >> 8) & 0xFF;
	msg.data[2] = calibration & 0xFF;
	if (i2c_master_write(&msg, retry)) {
		printk("isl28022_init, set calibration register fail\n");
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}

	/* allocate priv_data */
	isl28022_priv_data *new_priv_data =
		(isl28022_priv_data *)malloc(sizeof(isl28022_priv_data));
	if (!new_priv_data) {
		printk("isl28022_init: isl28022_priv_data malloc fail\n");
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}
	/* set value of priv_data and append it to linked list */
	new_priv_data->ID = init_arg->ID;
	new_priv_data->current_LSB = current_LSB;
	new_priv_data->config.value = init_arg->config.value;
	sys_slist_append(&priv_data_list, &new_priv_data->node);
	sensor_config[SensorNum_SensorCfg_map[sensor_num]].priv_data = new_priv_data;
	init_arg->is_init = true;

skip_init:
	sensor_config[SensorNum_SensorCfg_map[sensor_num]].read = isl28022_read;
	return SENSOR_INIT_SUCCESS;
}
