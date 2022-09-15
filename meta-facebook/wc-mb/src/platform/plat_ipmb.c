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
#include "cmsis_os2.h"
#include <string.h>
#include "plat_i2c.h"
#include "plat_ipmb.h"
#include "plat_ipmi.h"
#include "plat_class.h"

IPMB_config pal_IPMB_config_table[] = {
	// index, interface, channel, bus, channel_target_address, enable_status, self_address,
	// rx_thread_name, tx_thread_name
	{ BMC_IPMB_IDX, I2C_IF, BMC_IPMB, IPMB_BMC_BUS, BMC_I2C_ADDRESS, ENABLE, SELF_I2C_ADDRESS,
	  "RX_BMC_IPMB_TASK", "TX_BMC_IPMB_TASK" },
	{ ME_IPMB_IDX, I2C_IF, ME_IPMB, IPMB_ME_BUS, ME_I2C_ADDRESS, ENABLE, SELF_I2C_ADDRESS,
	  "RX_ME_IPMB_TASK", "TX_ME_IPMB_TASK" },
	{ RESERVED_IDX, RESERVED_IF, RESERVED, RESERVED_BUS, RESERVED_ADDRESS, DISABLE,
	  RESERVED_ADDRESS, "RESERVED_ATTR", "RESERVED_ATTR" },
};

bool pal_load_ipmb_config(void)
{
	memcpy(&IPMB_config_table[0], &pal_IPMB_config_table[0], sizeof(pal_IPMB_config_table));
	return true;
};