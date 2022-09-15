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

#include "hal_gpio.h"
#include "hal_peci.h"
#include "power_status.h"
#include "util_sys.h"
#include "plat_class.h"
#include "plat_gpio.h"
#include "plat_pmic.h"

SCU_CFG scu_cfg[] = {
	//register    value
	/* Set GPIOC0-C1 and GPIOC6-C7 to passthrough mode after gpio init */
	{ 0x7e6e24b0, 0x00C30000 },
	/* Set GPIOA/B/C/D internal pull-up/down after gpio init */
	{ 0x7e6e2610, 0xFFFFFFFF },
	/* Set GPIOF/G/H internal pull-up/down after gpio init */
	{ 0x7e6e2614, 0xFFFFFFFF },
	/* Set GPIOJ/K/L internal pull-up/down after gpio init */
	{ 0x7e6e2618, 0xF0000000 },
	/* Set GPIOM/N/O/P internal pull-up/down after gpio init */
	{ 0x7e6e261c, 0x0000000B },
	/* Set GPIOQ/R/S/T internal pull-up/down after gpio init */
	{ 0x7e6e2630, 0xFF000000 },
	/* Set GPIOU/V/X internal pull-up/down after gpio init */
	{ 0x7e6e2634, 0x000000FD },
};

/* port80 bypass signal to GPIOF(postcode led) */
static void postcode_led_ctl()
{
	uint32_t val = 0;

	/* LPC config - set address */
	val = sys_read32(0x7e789090);
	sys_write32((val & 0xFFFFFF00) | 0x80, 0x7e789090);

	/* LPC config - enable snoop */
	val = sys_read32(0x7e789080);
	sys_write32((val & 0xFFFFFFFE) | 0x1, 0x7e789080);

	/* GPIO config - set GPIOF to output */
	val = sys_read32(0x7e780024);
	sys_write32((val & 0xFFFF00FF) | 0xFF00, 0x7e780024);

	/* GPIO config - set GPIOF source #0 */
	val = sys_read32(0x7e780068);
	sys_write32((val & 0xFFFFFEFF) | 0x100, 0x7e780068);

	/* GPIO config - set GPIOF source #1 */
	val = sys_read32(0x7e78006C);
	sys_write32((val & 0xFFFFFEFF) | 0x0, 0x7e78006C);

	/* Super IO config - set SIOR7_30h to 0x80 */
	// These configs should set by BIOS

	/* Super IO config - set SIOR7_38h to 0x5(GPIOF) */
	// These configs should set by BIOS
}

/* control mb host status led */
static void mb_pwr_led_ctl()
{
	if (get_DC_status())
		gpio_set(BMC_PWR_LED, GPIO_HIGH);
	else
		gpio_set(BMC_PWR_LED, GPIO_LOW);
}

void pal_pre_init()
{
	init_platform_config();
	disable_PRDY_interrupt();
	scu_init(scu_cfg, sizeof(scu_cfg) / sizeof(SCU_CFG));
	postcode_led_ctl();
}

void pal_device_init()
{
	init_me_firmware();
	start_monitor_pmic_error_thread();
}

void pal_set_sys_status()
{
	set_DC_status(PWRGD_SYS_PWROK);
	set_DC_on_delayed_status();
	set_DC_off_delayed_status();
	set_post_status(FM_BIOS_POST_CMPLT_BIC_N);
	set_CPU_power_status(PWRGD_CPU_LVC3);
	set_post_thread();
	set_sys_ready_pin(FM_BIC_READY);
	mb_pwr_led_ctl();
}

#define DEF_PROJ_GPIO_PRIORITY 78

DEVICE_DEFINE(PRE_DEF_PROJ_GPIO, "PRE_DEF_PROJ_GPIO_NAME", &gpio_init, NULL, NULL, NULL,
	      POST_KERNEL, DEF_PROJ_GPIO_PRIORITY, NULL);
