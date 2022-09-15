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

#include "libipmi.h"
#include "kcs.h"
#include "power_status.h"
#include "sensor.h"
#include "snoop.h"
#include "plat_gpio.h"
#include "plat_ipmi.h"
#include "plat_sensor_table.h"
#include "oem_1s_handler.h"
#include "hal_gpio.h"
#include "util_sys.h"
#include "pex89000.h"
#include "pldm.h"
#include "plat_mctp.h"
#include "plat_hook.h"
void dc_on_init_component()
{
	uint8_t pex_sensor_num_table[PEX_MAX_NUMBER] = { SENSOR_NUM_BB_TEMP_PEX_0,
							 SENSOR_NUM_BB_TEMP_PEX_1,
							 SENSOR_NUM_BB_TEMP_PEX_2,
							 SENSOR_NUM_BB_TEMP_PEX_3 };

	for (int i = 0; i < PEX_MAX_NUMBER; i++) {
		uint8_t sensor_num = pex_sensor_num_table[i];
		sensor_cfg *cfg = &sensor_config[sensor_config_index_map[sensor_num]];
		pex89000_init_arg *init_arg = (pex89000_init_arg *)cfg->init_args;

		/* Only need initial when not initial yet */
		if (init_arg->is_init == false) {
			if (cfg->pre_sensor_read_hook) {
				if (cfg->pre_sensor_read_hook(cfg->num,
							      cfg->pre_sensor_read_args) == false) {
					printf("[%s] sensor 0x%x pre sensor read failed!\n",
					       __func__, cfg->num);
					continue;
				} else {
					/* pre_read success call PEX initial function */
					if (pex89000_init(sensor_num) != SENSOR_INIT_SUCCESS)
						printf("[%s] sensor 0x%x init fail \n", __func__,
						       cfg->num);
				}
			}
			if (cfg->post_sensor_read_hook) {
				if (cfg->post_sensor_read_hook(sensor_num,
							       cfg->post_sensor_read_args,
							       NULL) == false) {
					printf("[%s]sensor number 0x%x post_read fail\n", __func__,
					       sensor_num);
				}
			}
		}
	}
	/* Call function to set endpoint and get parameters for the device, the
   	 * function description is as defined by zephyr but argument is not used in
   	 * this function so put NULL here.
   	 */
	send_cmd_to_dev(NULL);
}

K_WORK_DELAYABLE_DEFINE(set_DC_on_5s_work, dc_on_init_component);
#define DC_ON_5_SECOND 5

void ISR_DC_ON()
{
	/* Check whether DC on to send work to initial PEX */
	if (is_mb_dc_on())
		k_work_schedule(&set_DC_on_5s_work, K_SECONDS(DC_ON_5_SECOND));
}
