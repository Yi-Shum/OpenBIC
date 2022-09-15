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

#include <zephyr.h>
#include <stdio.h>
#include <string.h>
#include "hal_gpio.h"
#include "util_sys.h"

#define STACK_SIZE 2048

static const struct device *dev_gpio[GPIO_GROUP_NUM];
static struct gpio_callback callbacks[TOTAL_GPIO_NUM];
static struct k_work_q gpio_work_queue;
static K_THREAD_STACK_DEFINE(gpio_work_stack, GPIO_STACK_SIZE);

struct k_work gpio_work[TOTAL_GPIO_NUM];

uint8_t gpio_ind_to_num_table[200];
uint8_t gpio_ind_to_num_table_cnt;

__weak const char *const gpio_name[] = {};
GPIO_CFG gpio_cfg[GPIO_CFG_SIZE] = {
	//  chip,      number,   is_init, direction,    status,     property,    int_type,           int_cb
	//  Defalut              DISABLE  GPIO_INPUT    LOW         PUSH_PULL    GPIO_INT_DISABLE    NULL
};

uint32_t GPIO_GROUP_REG_ACCESS[GPIO_GROUP_NUM] = {
	REG_GPIO_BASE + 0x00, /* GPIO_A/B/C/D Data Value Register */
	REG_GPIO_BASE + 0x20, /* GPIO_E/F/G/H Data Value Register */
	REG_GPIO_BASE + 0x70, /* GPIO_I/J/K/L Data Value Register */
	REG_GPIO_BASE + 0x78, /* GPIO_M/N/O/P Data Value Register */
	REG_GPIO_BASE + 0x80, /* GPIO_Q/R/S/T Data Value Register */
	REG_GPIO_BASE + 0x88, /* GPIO_U Data Value Register */
};

uint32_t GPIO_MULTI_FUNC_PIN_CTL_REG_ACCESS[] = {
	REG_SCU + 0x410, /* Multi-function pin ctl #1 */
	REG_SCU + 0x414, /* Multi-function pin ctl #2 */
	REG_SCU + 0x418, /* Multi-function pin ctl #3 */
	REG_SCU + 0x41C, /* Multi-function pin ctl #4 */
	REG_SCU + 0x430, /* Multi-function pin ctl #5 */
	REG_SCU + 0x434, /* Multi-function pin ctl #6 */
	REG_SCU + 0x438, /* Multi-function pin ctl #7 */
	REG_SCU + 0x450, /* Multi-function pin ctl #9 */
	REG_SCU + 0x454, /* Multi-function pin ctl #10 */
	REG_SCU + 0x458, /* Multi-function pin ctl #11 */
	REG_SCU + 0x4B0, /* Multi-function pin ctl #13 */
	REG_SCU + 0x4B4, /* Multi-function pin ctl #14 */
	REG_SCU + 0x4B8, /* Multi-function pin ctl #15 */
	REG_SCU + 0x4BC, /* Multi-function pin ctl #16 */
	REG_SCU + 0x4D4, /* Multi-function pin ctl #18 */
	REG_SCU + 0x4D8, /* Multi-function pin ctl #19 */
	REG_SCU + 0x510, /* Hardware Strap2 Register */
	REG_SCU + 0x51C, /* Hardware Strap2 Clear Register */
};
const int GPIO_MULTI_FUNC_CFG_SIZE = ARRAY_SIZE(GPIO_MULTI_FUNC_PIN_CTL_REG_ACCESS);

void irq_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	uint8_t group, index, gpio_num;

	if (dev == dev_gpio[GPIO_A_D]) {
		group = GPIO_A_D;
	} else if (dev == dev_gpio[GPIO_E_H]) {
		group = GPIO_E_H;
	} else if (dev == dev_gpio[GPIO_I_L]) {
		group = GPIO_I_L;
	} else if (dev == dev_gpio[GPIO_M_P]) {
		group = GPIO_M_P;
	} else if (dev == dev_gpio[GPIO_Q_T]) {
		group = GPIO_Q_T;
	} else if (dev == dev_gpio[GPIO_U_V]) {
		group = GPIO_U_V;
	} else {
		printf("invalid dev group for isr cb\n");
		return;
	}

	for (index = 0; index < GPIO_GROUP_SIZE; index++) {
		if ((pins >> index) & 0x1) {
			pins = index;
			break;
		}
	}
	if (index == GPIO_GROUP_SIZE) {
		printf("irq_callback: pin %x not found\n", pins);
		return;
	}
	gpio_num = (group * GPIO_GROUP_SIZE) + pins;

	if (gpio_cfg[gpio_num].int_cb == NULL) {
		printf("Callback function pointer NULL for gpio num %d\n", gpio_num);
		return;
	}

	k_work_submit_to_queue(&gpio_work_queue, &gpio_work[gpio_num]);
}

static void gpio_init_cb(uint8_t gpio_num)
{
	gpio_init_callback(&callbacks[gpio_num], irq_callback, BIT(gpio_num % GPIO_GROUP_SIZE));
	return;
}

static int gpio_add_cb(uint8_t gpio_num)
{
	return gpio_add_callback(dev_gpio[gpio_num / GPIO_GROUP_SIZE], &callbacks[gpio_num]);
}
/*
interrupt type:
  GPIO_INT_DISABLE
  GPIO_INT_EDGE_RISING
  GPIO_INT_EDGE_FALLING
  GPIO_INT_EDGE_BOTH
  GPIO_INT_LEVEL_LOW
  GPIO_INT_LEVEL_HIGH
*/
int gpio_interrupt_conf(uint8_t gpio_num, gpio_flags_t flags)
{
	return gpio_pin_interrupt_configure(dev_gpio[gpio_num / GPIO_GROUP_SIZE],
					    (gpio_num % GPIO_GROUP_SIZE), flags);
}

void gpio_cb_irq_init(uint8_t gpio_num, gpio_flags_t flags)
{
	gpio_init_cb(gpio_num);
	gpio_add_cb(gpio_num);
	gpio_interrupt_conf(gpio_num, flags);

	k_work_init(&gpio_work[gpio_num], gpio_cfg[gpio_num].int_cb);
}

uint8_t gpio_conf(uint8_t gpio_num, int dir)
{
	return gpio_pin_configure(dev_gpio[gpio_num / GPIO_GROUP_SIZE],
				  (gpio_num % GPIO_GROUP_SIZE), dir);
}

int gpio_get_direction(uint8_t gpio_num)
{
	uint8_t dir = 0xFF;

	if (gpio_num >= TOTAL_GPIO_NUM) {
		printf("getting invalid gpio num %d", gpio_num);
		return dir;
	}

	uint32_t g_dir = sys_read32(GPIO_GROUP_REG_ACCESS[gpio_num / 32] + 0x4);
	if (g_dir & BIT(gpio_num % 32))
		dir = 0x01;
	else
		dir = 0x00;

	return dir;
}

int gpio_get(uint8_t gpio_num)
{
	if (gpio_num >= TOTAL_GPIO_NUM) {
		printf("getting invalid gpio num %d", gpio_num);
		return false;
	}

	return gpio_pin_get(dev_gpio[gpio_num / GPIO_GROUP_SIZE], (gpio_num % GPIO_GROUP_SIZE));
	//  return gpio[0].get(&gpio[0], gpio_num);
}

int gpio_set(uint8_t gpio_num, uint8_t status)
{
	if (gpio_num >= TOTAL_GPIO_NUM) {
		printf("setting invalid gpio num %d", gpio_num);
		return false;
	}

	if (gpio_cfg[gpio_num].property == OPEN_DRAIN) { // should release gpio ctrl for OD high
		if (status) {
			return gpio_conf(gpio_num, GPIO_INPUT);
		} else {
			gpio_conf(gpio_num, GPIO_OUTPUT);
			return gpio_pin_set(dev_gpio[gpio_num / GPIO_GROUP_SIZE],
					    (gpio_num % GPIO_GROUP_SIZE), status);
		}
	} else {
		return gpio_pin_set(dev_gpio[gpio_num / GPIO_GROUP_SIZE],
				    (gpio_num % GPIO_GROUP_SIZE), status);
	}
}

void gpio_index_to_num(void)
{
	uint8_t i = 0;
	for (uint8_t j = 0; j < TOTAL_GPIO_NUM; j++) {
		if (gpio_cfg[j].is_init == ENABLE) {
			gpio_ind_to_num_table[i++] = gpio_cfg[j].number;
		}
	}
	gpio_ind_to_num_table_cnt = i;
}

void init_gpio_dev(void)
{
#ifdef DEV_GPIO_A_D
	dev_gpio[GPIO_A_D] = device_get_binding("GPIO0_A_D");
#endif
#ifdef DEV_GPIO_E_H
	dev_gpio[GPIO_E_H] = device_get_binding("GPIO0_E_H");
#endif
#ifdef DEV_GPIO_I_L
	dev_gpio[GPIO_I_L] = device_get_binding("GPIO0_I_L");
#endif
#ifdef DEV_GPIO_M_P
	dev_gpio[GPIO_M_P] = device_get_binding("GPIO0_M_P");
#endif
#ifdef DEV_GPIO_Q_T
	dev_gpio[GPIO_Q_T] = device_get_binding("GPIO0_Q_T");
#endif
#ifdef DEV_GPIO_U_V
	dev_gpio[GPIO_U_V] = device_get_binding("GPIO0_U_V");
#endif
}

void scu_init(SCU_CFG cfg[], size_t size)
{
	for (int i = 0; i < size; ++i) {
		sys_write32(cfg[i].value, cfg[i].reg);
	}
}

__weak bool pal_load_gpio_config(void)
{
	return true;
}

int gpio_init(const struct device *args)
{
	uint8_t i;

	pal_load_gpio_config();
	init_gpio_dev();
	gpio_index_to_num();

	bool ac_lost = is_ac_lost();

	k_work_queue_start(&gpio_work_queue, gpio_work_stack, GPIO_STACK_SIZE,
			   K_PRIO_PREEMPT(CONFIG_MAIN_THREAD_PRIORITY), NULL);
	k_thread_name_set(&gpio_work_queue.thread, "gpio_workq");

	for (i = 0; i < TOTAL_GPIO_NUM; i++) {
		if (gpio_cfg[i].is_init == ENABLE) {
			if ((gpio_cfg[i].is_latch == ENABLE) && (!ac_lost)) {
				continue;
			}
			if (gpio_cfg[i].chip == CHIP_GPIO) {
				gpio_set(gpio_cfg[i].number, gpio_cfg[i].status);
				if (gpio_cfg[i].property ==
				    PUSH_PULL) { // OD config is set during status set
					gpio_conf(gpio_cfg[i].number, gpio_cfg[i].direction);
				}
				gpio_set(gpio_cfg[i].number, gpio_cfg[i].status);
				if ((gpio_cfg[i].int_type == GPIO_INT_EDGE_RISING) ||
				    (gpio_cfg[i].int_type == GPIO_INT_EDGE_FALLING) ||
				    (gpio_cfg[i].int_type == GPIO_INT_EDGE_BOTH) ||
				    (gpio_cfg[i].int_type == GPIO_INT_LEVEL_LOW) ||
				    (gpio_cfg[i].int_type == GPIO_INT_LEVEL_HIGH)) {
					gpio_cb_irq_init(gpio_cfg[i].number, gpio_cfg[i].int_type);
				}
			} else {
				printf("TODO: add sgpio handler\n");
			}
		}
	}

	return true;
}
