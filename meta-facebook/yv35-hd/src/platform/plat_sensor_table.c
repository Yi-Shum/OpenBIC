#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "plat_sensor_table.h"
#include "sensor.h"
#include "ast_adc.h"
#include "plat_hook.h"
#include "pmbus.h"
#include "plat_i2c.h"
#include "apml.h"

sensor_cfg plat_sensor_config[] = {
	/* number, type, port, address, offset, access check, arg0, arg1, cache, cache_status, 
	   pre_sensor_read_fn, pre_sensor_read_args, post_sensor_read_fn, post_sensor_read_fn,
	   init_arg */

	/* temperature */
	{ SENSOR_NUM_TEMP_TMP75_IN, sensor_dev_tmp75, I2C_BUS2, TMP75_IN_ADDR, TMP75_TEMP_OFFSET,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL,
	  NULL },
	{ SENSOR_NUM_TEMP_TMP75_OUT, sensor_dev_tmp75, I2C_BUS2, TMP75_OUT_ADDR, TMP75_TEMP_OFFSET,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL,
	  NULL },
	{ SENSOR_NUM_TEMP_TMP75_FIO, sensor_dev_tmp75, I2C_BUS2, TMP75_FIO_ADDR, TMP75_TEMP_OFFSET,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL,
	  NULL },

	/* NVME */
	{ SENSOR_NUM_TEMP_SSD, sensor_dev_nvme, I2C_BUS2, SSD_ADDR, SSD_TEMP_OFFSET, post_access, 0,
	  0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, pre_nvme_read, &mux_conf_addr_0xe2[1],
	  NULL, NULL, NULL },

	/* CPU */
	{ SENSOR_NUM_TEMP_CPU, sensor_dev_amd_tsi, I2C_BUS14, TSI_ADDR, NONE, post_access, 0, 0,
	  SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, NULL },
	{ SENSOR_NUM_TEMP_DIMM_A, sensor_dev_amd_apml_rmi, I2C_BUS14, APML_ADDR,
	  SBRMI_MAILBOX_GET_DIMM_TEMP, post_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, &amd_apml_rmi_init_args[0] },
	{ SENSOR_NUM_TEMP_DIMM_B, sensor_dev_amd_apml_rmi, I2C_BUS14, APML_ADDR,
	  SBRMI_MAILBOX_GET_DIMM_TEMP, post_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, &amd_apml_rmi_init_args[1] },
	{ SENSOR_NUM_TEMP_DIMM_C, sensor_dev_amd_apml_rmi, I2C_BUS14, APML_ADDR,
	  SBRMI_MAILBOX_GET_DIMM_TEMP, post_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, &amd_apml_rmi_init_args[2] },
	{ SENSOR_NUM_TEMP_DIMM_E, sensor_dev_amd_apml_rmi, I2C_BUS14, APML_ADDR,
	  SBRMI_MAILBOX_GET_DIMM_TEMP, post_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, &amd_apml_rmi_init_args[3] },
	{ SENSOR_NUM_TEMP_DIMM_G, sensor_dev_amd_apml_rmi, I2C_BUS14, APML_ADDR,
	  SBRMI_MAILBOX_GET_DIMM_TEMP, post_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, &amd_apml_rmi_init_args[4] },
	{ SENSOR_NUM_TEMP_DIMM_H, sensor_dev_amd_apml_rmi, I2C_BUS14, APML_ADDR,
	  SBRMI_MAILBOX_GET_DIMM_TEMP, post_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, &amd_apml_rmi_init_args[5] },
	{ SENSOR_NUM_TEMP_DIMM_I, sensor_dev_amd_apml_rmi, I2C_BUS14, APML_ADDR,
	  SBRMI_MAILBOX_GET_DIMM_TEMP, post_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, &amd_apml_rmi_init_args[6] },
	{ SENSOR_NUM_TEMP_DIMM_K, sensor_dev_amd_apml_rmi, I2C_BUS14, APML_ADDR,
	  SBRMI_MAILBOX_GET_DIMM_TEMP, post_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, &amd_apml_rmi_init_args[7] },
	{ SENSOR_NUM_PWR_CPU, sensor_dev_amd_apml_rmi, I2C_BUS14, APML_ADDR, SBRMI_MAILBOX_PKGPWR,
	  post_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL,
	  NULL },
	{ SENSOR_NUM_PWR_DIMM_A, sensor_dev_amd_apml_rmi, I2C_BUS14, APML_ADDR,
	  SBRMI_MAILBOX_GET_DIMM_PWR, post_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, &amd_apml_rmi_init_args[0] },
	{ SENSOR_NUM_PWR_DIMM_B, sensor_dev_amd_apml_rmi, I2C_BUS14, APML_ADDR,
	  SBRMI_MAILBOX_GET_DIMM_PWR, post_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, &amd_apml_rmi_init_args[1] },
	{ SENSOR_NUM_PWR_DIMM_C, sensor_dev_amd_apml_rmi, I2C_BUS14, APML_ADDR,
	  SBRMI_MAILBOX_GET_DIMM_PWR, post_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, &amd_apml_rmi_init_args[2] },
	{ SENSOR_NUM_PWR_DIMM_E, sensor_dev_amd_apml_rmi, I2C_BUS14, APML_ADDR,
	  SBRMI_MAILBOX_GET_DIMM_PWR, post_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, &amd_apml_rmi_init_args[3] },
	{ SENSOR_NUM_PWR_DIMM_G, sensor_dev_amd_apml_rmi, I2C_BUS14, APML_ADDR,
	  SBRMI_MAILBOX_GET_DIMM_PWR, post_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, &amd_apml_rmi_init_args[4] },
	{ SENSOR_NUM_PWR_DIMM_H, sensor_dev_amd_apml_rmi, I2C_BUS14, APML_ADDR,
	  SBRMI_MAILBOX_GET_DIMM_PWR, post_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, &amd_apml_rmi_init_args[5] },
	{ SENSOR_NUM_PWR_DIMM_I, sensor_dev_amd_apml_rmi, I2C_BUS14, APML_ADDR,
	  SBRMI_MAILBOX_GET_DIMM_PWR, post_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, &amd_apml_rmi_init_args[6] },
	{ SENSOR_NUM_PWR_DIMM_K, sensor_dev_amd_apml_rmi, I2C_BUS14, APML_ADDR,
	  SBRMI_MAILBOX_GET_DIMM_PWR, post_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, &amd_apml_rmi_init_args[7] },

	/* adc voltage */
	{ SENSOR_NUM_VOL_P12V_STBY, sensor_dev_ast_adc, ADC_PORT0, NONE, NONE, stby_access, 66, 10,
	  SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL,
	  &ast_adc_init_args[0] },
	{ SENSOR_NUM_VOL_PVDD18_S5, sensor_dev_ast_adc, ADC_PORT1, NONE, NONE, dc_access, 1, 1,
	  SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL,
	  &ast_adc_init_args[0] },
	{ SENSOR_NUM_VOL_P3V3_STBY, sensor_dev_ast_adc, ADC_PORT2, NONE, NONE, stby_access, 2, 1,
	  SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL,
	  &ast_adc_init_args[0] },
	{ SENSOR_NUM_VOL_PVDD11_S3, sensor_dev_ast_adc, ADC_PORT3, NONE, NONE, dc_access, 1, 1,
	  SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL,
	  &ast_adc_init_args[0] },
	{ SENSOR_NUM_VOL_P3V_BAT, sensor_dev_ast_adc, ADC_PORT4, NONE, NONE, stby_access, 3, 1,
	  SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, pre_vol_bat3v_read, NULL,
	  post_vol_bat3v_read, NULL, &ast_adc_init_args[0] },
	{ SENSOR_NUM_VOL_PVDD33_S5, sensor_dev_ast_adc, ADC_PORT5, NONE, NONE, dc_access, 2, 1,
	  SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL,
	  &ast_adc_init_args[0] },
	{ SENSOR_NUM_VOL_P5V_STBY, sensor_dev_ast_adc, ADC_PORT14, NONE, NONE, stby_access, 711,
	  200, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL,
	  &ast_adc_init_args[0] },
	{ SENSOR_NUM_VOL_P12V_MEM_1, sensor_dev_ast_adc, ADC_PORT13, NONE, NONE, dc_access, 66, 10,
	  SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL,
	  &ast_adc_init_args[0] },
	{ SENSOR_NUM_VOL_P12V_MEM_0, sensor_dev_ast_adc, ADC_PORT12, NONE, NONE, dc_access, 66, 10,
	  SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL,
	  &ast_adc_init_args[0] },
	{ SENSOR_NUM_VOL_P1V2_STBY, sensor_dev_ast_adc, ADC_PORT10, NONE, NONE, stby_access, 1, 1,
	  SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL,
	  &ast_adc_init_args[0] },
	{ SENSOR_NUM_VOL_P3V3_M2, sensor_dev_ast_adc, ADC_PORT9, NONE, NONE, dc_access, 2, 1,
	  SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL,
	  &ast_adc_init_args[0] },
	{ SENSOR_NUM_VOL_P1V8_STBY, sensor_dev_ast_adc, ADC_PORT8, NONE, NONE, stby_access, 1, 1,
	  SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL,
	  &ast_adc_init_args[0] },

	/* VR voltage */
	{ SENSOR_NUM_VOL_PVDDCR_CPU0_VR, sensor_dev_raa229621, I2C_BUS5, PVDDCR_CPU0_ADDR,
	  PMBUS_READ_VOUT, vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS,
	  pre_raa229621_read, &raa229621_pre_read_args[0], NULL, NULL, NULL },
	{ SENSOR_NUM_VOL_PVDDCR_SOC_VR, sensor_dev_raa229621, I2C_BUS5, PVDDCR_SOC_ADDR,
	  PMBUS_READ_VOUT, vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS,
	  pre_raa229621_read, &raa229621_pre_read_args[1], NULL, NULL, NULL },
	{ SENSOR_NUM_VOL_PVDDCR_CPU1_VR, sensor_dev_raa229621, I2C_BUS5, PVDDCR_CPU1_ADDR,
	  PMBUS_READ_VOUT, vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS,
	  pre_raa229621_read, &raa229621_pre_read_args[0], NULL, NULL, NULL },
	{ SENSOR_NUM_VOL_PVDDIO_VR, sensor_dev_raa229621, I2C_BUS5, PVDDIO_ADDR, PMBUS_READ_VOUT,
	  vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, pre_raa229621_read,
	  &raa229621_pre_read_args[1], NULL, NULL, NULL },
	{ SENSOR_NUM_VOL_PVDD11_S3_VR, sensor_dev_raa229621, I2C_BUS5, PVDD11_S3_ADDR,
	  PMBUS_READ_VOUT, vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS,
	  pre_raa229621_read, &raa229621_pre_read_args[0], NULL, NULL, NULL },

	/* VR current */
	{ SENSOR_NUM_CUR_PVDDCR_CPU0_VR, sensor_dev_raa229621, I2C_BUS5, PVDDCR_CPU0_ADDR,
	  PMBUS_READ_IOUT, vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS,
	  pre_raa229621_read, &raa229621_pre_read_args[0], NULL, NULL, NULL },
	{ SENSOR_NUM_CUR_PVDDCR_SOC_VR, sensor_dev_raa229621, I2C_BUS5, PVDDCR_SOC_ADDR,
	  PMBUS_READ_IOUT, vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS,
	  pre_raa229621_read, &raa229621_pre_read_args[1], NULL, NULL, NULL },
	{ SENSOR_NUM_CUR_PVDDCR_CPU1_VR, sensor_dev_raa229621, I2C_BUS5, PVDDCR_CPU1_ADDR,
	  PMBUS_READ_IOUT, vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS,
	  pre_raa229621_read, &raa229621_pre_read_args[0], NULL, NULL, NULL },
	{ SENSOR_NUM_CUR_PVDDIO_VR, sensor_dev_raa229621, I2C_BUS5, PVDDIO_ADDR, PMBUS_READ_IOUT,
	  vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, pre_raa229621_read,
	  &raa229621_pre_read_args[1], NULL, NULL, NULL },
	{ SENSOR_NUM_CUR_PVDD11_S3_VR, sensor_dev_raa229621, I2C_BUS5, PVDD11_S3_ADDR,
	  PMBUS_READ_IOUT, vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS,
	  pre_raa229621_read, &raa229621_pre_read_args[0], NULL, NULL, NULL },

	/* VR temperature */
	{ SENSOR_NUM_TEMP_PVDDCR_CPU0_VR, sensor_dev_raa229621, I2C_BUS5, PVDDCR_CPU0_ADDR,
	  PMBUS_READ_TEMPERATURE_1, vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS,
	  pre_raa229621_read, &raa229621_pre_read_args[0], NULL, NULL, NULL },
	{ SENSOR_NUM_TEMP_PVDDCR_SOC_VR, sensor_dev_raa229621, I2C_BUS5, PVDDCR_SOC_ADDR,
	  PMBUS_READ_TEMPERATURE_1, vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS,
	  pre_raa229621_read, &raa229621_pre_read_args[1], NULL, NULL, NULL },
	{ SENSOR_NUM_TEMP_PVDDCR_CPU1_VR, sensor_dev_raa229621, I2C_BUS5, PVDDCR_CPU1_ADDR,
	  PMBUS_READ_TEMPERATURE_1, vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS,
	  pre_raa229621_read, &raa229621_pre_read_args[0], NULL, NULL, NULL },
	{ SENSOR_NUM_TEMP_PVDDIO_VR, sensor_dev_raa229621, I2C_BUS5, PVDDIO_ADDR,
	  PMBUS_READ_TEMPERATURE_1, vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS,
	  pre_raa229621_read, &raa229621_pre_read_args[1], NULL, NULL, NULL },
	{ SENSOR_NUM_TEMP_PVDD11_S3_VR, sensor_dev_raa229621, I2C_BUS5, PVDD11_S3_ADDR,
	  PMBUS_READ_TEMPERATURE_1, vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS,
	  pre_raa229621_read, &raa229621_pre_read_args[0], NULL, NULL, NULL },

	/* VR power */
	{ SENSOR_NUM_PWR_PVDDCR_CPU0_VR, sensor_dev_raa229621, I2C_BUS5, PVDDCR_CPU0_ADDR,
	  PMBUS_READ_POUT, vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS,
	  pre_raa229621_read, &raa229621_pre_read_args[0], NULL, NULL, NULL },
	{ SENSOR_NUM_PWR_PVDDCR_SOC_VR, sensor_dev_raa229621, I2C_BUS5, PVDDCR_SOC_ADDR,
	  PMBUS_READ_POUT, vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS,
	  pre_raa229621_read, &raa229621_pre_read_args[1], NULL, NULL, NULL },
	{ SENSOR_NUM_PWR_PVDDCR_CPU1_VR, sensor_dev_raa229621, I2C_BUS5, PVDDCR_CPU1_ADDR,
	  PMBUS_READ_POUT, vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS,
	  pre_raa229621_read, &raa229621_pre_read_args[0], NULL, NULL, NULL },
	{ SENSOR_NUM_PWR_PVDDIO_VR, sensor_dev_raa229621, I2C_BUS5, PVDDIO_ADDR, PMBUS_READ_POUT,
	  vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, pre_raa229621_read,
	  &raa229621_pre_read_args[1], NULL, NULL, NULL },
	{ SENSOR_NUM_PWR_PVDD11_S3_VR, sensor_dev_raa229621, I2C_BUS5, PVDD11_S3_ADDR,
	  PMBUS_READ_POUT, vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS,
	  pre_raa229621_read, &raa229621_pre_read_args[0], NULL, NULL, NULL },

	/* HSC */
	{ SENSOR_NUM_TEMP_HSC, sensor_dev_nct7718w, I2C_BUS5, TEMP_HSC_ADDR,
	  NCT7718W_REMOTE_TEMP_MSB_OFFSET, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, NULL },
	{ SENSOR_NUM_VOL_HSCIN, sensor_dev_adm1278, I2C_BUS5, HSC_ADDR, PMBUS_READ_VIN, stby_access,
	  0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL,
	  &adm1278_init_args[0] },
	{ SENSOR_NUM_CUR_HSCOUT, sensor_dev_adm1278, I2C_BUS5, HSC_ADDR, PMBUS_READ_IOUT,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, NULL, NULL,
	  post_adm1278_cur_read, NULL, &adm1278_init_args[0] },
	{ SENSOR_NUM_PWR_HSCIN, sensor_dev_adm1278, I2C_BUS5, HSC_ADDR, PMBUS_READ_PIN, stby_access,
	  0, 0, SAMPLE_COUNT_DEFAULT, 0, SENSOR_INIT_STATUS, NULL, NULL, post_adm1278_pwr_read,
	  NULL, &adm1278_init_args[0] },

};

uint8_t plat_get_config_size()
{
	return ARRAY_SIZE(plat_sensor_config);
}

void load_sensor_config(void)
{
	memcpy(sensor_config, plat_sensor_config, sizeof(plat_sensor_config));
	sensor_config_count = ARRAY_SIZE(plat_sensor_config);
}
