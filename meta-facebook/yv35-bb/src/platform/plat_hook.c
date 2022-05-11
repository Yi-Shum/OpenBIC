#include <stdio.h>
#include <string.h>
#include <sensor.h>
#include "plat_hook.h"
#include "plat_i2c.h"

#define VOLTAGE_SELECT_BIT (1 << 2)
#define ADC_16BIT_MODE_BIT (1 << 0)

#define ADC_16BIT_MODE_DELAY_MS 1100
#define ADC_12BIT_MODE_DELAY_MS 100

/**************************************************************************************************
 * INIT ARGS
**************************************************************************************************/
ast_adc_init_arg ast_adc_init_args[] = { [0] = { .is_init = false } };

adm1278_init_arg adm1278_init_args[] = {
	[0] = { .ID = 0, .is_init = false, .config = { 0x3F1C }, .r_sense = 0.25 }
};

ltc4282_init_arg ltc4282_init_args[] = { [0] = { .ID = 0, .r_sense = 0.0001875, .is_init = false } };

/**************************************************************************************************
 *  PRE-HOOK/POST-HOOK ARGS
 **************************************************************************************************/
ltc4282_pre_proc_arg ltc4282_pre_read_args[] = {
	[0] = { "VIN" },
	[1] = { "VOUT" },
};

ast_adc_post_proc_arg ast_adc_post_args[] = {
	[0] = { 3.68 },
	[1] = { 8.9 },
	[2] = { 2.435 },
};

/**************************************************************************************************
 *  PRE-HOOK/POST-HOOK FUNC
 **************************************************************************************************/
/* LTC4282 pre read function
 *
 * set Vsource
 *
 * @param sensor_num sensor number
 * @param args pointer to ltc4282_pre_proc_arg
 * @param reading pointer to reading from previous step
 * @retval true if setting is successful.
 * @retval false if setting failed.
 */
bool pre_ltc4282_read(uint8_t sensor_num, void *args)
{
	if (args == NULL) {
		return false;
	}

	ltc4282_pre_proc_arg *pre_proc_args = (ltc4282_pre_proc_arg *)args;
	uint8_t retry = 5;
	I2C_MSG msg;
	int val = 0;

	/* get adjust */
	msg.bus = sensor_config[sensor_config_index_map[sensor_num]].port;
	msg.target_addr = sensor_config[sensor_config_index_map[sensor_num]].target_addr;
	msg.tx_len = 1;
	msg.data[0] = LTC4282_ADJUST_OFFSET;
	msg.rx_len = 1;
	if (i2c_master_read(&msg, retry) != 0) {
		printf("%s, get voltage adjust register fail\n", __func__);
		return false;
	}

	if (strcmp(pre_proc_args->vsource_status, "VIN") == 0) {
		val = msg.data[0] | VOLTAGE_SELECT_BIT;

	} else if (strcmp(pre_proc_args->vsource_status, "VOUT") == 0) {
		val = msg.data[0] & (~VOLTAGE_SELECT_BIT);

	} else {
		printf("%s, unexpected vsource %s\n", __func__, pre_proc_args->vsource_status);
		return false;
	}

	msg.tx_len = 2;
	msg.data[0] = sensor_config[sensor_config_index_map[sensor_num]].offset;
	msg.data[1] = val;

	if (i2c_master_write(&msg, retry) != 0) {
		printf("%s, set vsource fail\n", __func__);
		return false;
	}

	if (val | ADC_16BIT_MODE_BIT) {
		k_msleep(ADC_16BIT_MODE_DELAY_MS);
	} else {
		k_msleep(ADC_12BIT_MODE_DELAY_MS);
	}

	return true;
}

bool post_ast_adc_read(uint8_t sensor_num, void *args, int *reading)
{
	if (!args || !reading)
		return false;

	sensor_val *sval = (sensor_val *)reading;
	ast_adc_post_proc_arg *post_arg = (ast_adc_post_proc_arg *)args;

	float val = (post_arg->amplification) * (sval->integer + ((float)sval->fraction / 1000));
	sval->integer = (int)val & 0xFFFF;
	sval->fraction = (val - sval->integer) * 1000;

	return true;
}
