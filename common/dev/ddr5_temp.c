/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <logging/log.h>
#include "sensor.h"
#include "hal_i2c.h"

#define TS0_LID 0b0010
#define TS1_LID 0b0110
#define SPD_LID 0b1010
#define SPD_DEVICE_CONFIG_OFFSET 0x0B
#define TS_SENSE_OFFSET 0x31

LOG_MODULE_REGISTER(dev_ddr5_temp);

uint8_t ddr5_temp_read(uint8_t sensor_num, int *reading)
{
	if (!reading || (sensor_num > SENSOR_NUM_MAX)) {
		return SENSOR_UNSPECIFIED_ERROR;
	}

	sensor_cfg *cfg = &sensor_config[sensor_config_index_map[sensor_num]];
	if (cfg->init_args == NULL) {
		return SENSOR_UNSPECIFIED_ERROR;
	}
	ddr5_init_temp_arg *init_arg = (ddr5_init_temp_arg *)cfg->init_args;
	uint8_t retry = 5;
	I2C_MSG msg = { 0 };

	msg.bus = cfg->port;
	msg.target_addr = ((TS0_LID << 3) | (init_arg->HID_code & 0x07));
	msg.tx_len = 1;
	msg.rx_len = 2;
	msg.data[0] = TS_SENSE_OFFSET;

	if (i2c_master_read(&msg, retry)) {
		LOG_WRN("i2c read failed.\n");
		return SENSOR_FAIL_TO_ACCESS;
	}

	float tmp;
	// 0.25 degree C/LSB
	if (msg.data[1] & BIT(4)) { // negative
		tmp = -0.25 * (((~((msg.data[1] << 8) | msg.data[0]) >> 2) + 1) & 0x3FF);
	} else {
		tmp = 0.25 * ((((msg.data[1] << 8) | msg.data[0]) >> 2) & 0x3FF);
	}
	init_arg->ts0_temp = tmp;

	msg.bus = cfg->port;
	msg.target_addr = ((TS1_LID << 3) | (init_arg->HID_code & 0x07));
	msg.tx_len = 1;
	msg.rx_len = 2;
	msg.data[0] = TS_SENSE_OFFSET;

	if (i2c_master_read(&msg, retry)) {
		LOG_WRN("i2c read failed.\n");
		return SENSOR_FAIL_TO_ACCESS;
	}

	if (msg.data[1] & BIT(4)) { // negative
		tmp = -0.25 * (((~((msg.data[1] << 8) | msg.data[0]) >> 2) + 1) & 0x3FF);
	} else {
		tmp = 0.25 * ((((msg.data[1] << 8) | msg.data[0]) >> 2) & 0x3FF);
	}
	init_arg->ts1_temp = tmp;

	msg.bus = cfg->port;
	msg.target_addr = ((SPD_LID << 3) | (init_arg->HID_code & 0x07));
	msg.tx_len = 2;
	msg.rx_len = 0;
	msg.data[0] = SPD_DEVICE_CONFIG_OFFSET;
	msg.data[1] = 0x00;

	if (i2c_master_write(&msg, retry)) {
		LOG_WRN("i2c read failed.\n");
		return SENSOR_FAIL_TO_ACCESS;
	}

	msg.bus = cfg->port;
	msg.target_addr = ((SPD_LID << 3) | (init_arg->HID_code & 0x07));
	msg.tx_len = 1;
	msg.rx_len = 2;
	msg.data[0] = TS_SENSE_OFFSET;

	if (i2c_master_read(&msg, retry)) {
		LOG_WRN("i2c read failed.\n");
		return SENSOR_FAIL_TO_ACCESS;
	}

	if (msg.data[1] & BIT(4)) { // negative
		tmp = -0.25 * (((~((msg.data[1] << 8) | msg.data[0]) >> 2) + 1) & 0x3FF);
	} else {
		tmp = 0.25 * ((((msg.data[1] << 8) | msg.data[0]) >> 2) & 0x3FF);
	}
	init_arg->spd_temp = tmp;

	float val = MAX(MAX(init_arg->ts0_temp, init_arg->ts1_temp), init_arg->spd_temp);

	sensor_val *sval = (sensor_val *)reading;
	sval->integer = (int32_t)val;
	sval->fraction = (int32_t)(val * 1000) % 1000;
	return SENSOR_READ_SUCCESS;
}

uint8_t ddr5_temp_init(uint8_t sensor_num)
{
	if (sensor_num > SENSOR_NUM_MAX) {
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}

	sensor_config[sensor_config_index_map[sensor_num]].read = ddr5_temp_read;
	return SENSOR_INIT_SUCCESS;
}
