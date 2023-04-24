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

#include <drivers/uart.h>
#include <sys/ring_buffer.h>
#include "logging/log.h"
#include "plat_uart.h"
#include "ipmi.h"
#include "libipmi.h"
#include "plat_sensor_table.h"

LOG_MODULE_REGISTER(plat_uart);

struct k_thread uart_thread;
K_KERNEL_STACK_MEMBER(uart_handler_stack, UART_HANDLER_STACK_SIZE);
RING_BUF_DECLARE(uart_ring_buffer, UART_RING_BUFFER_SIZE);
static struct k_sem uart_handler_sem;

const struct uart_config uart_cfg = { .baudrate = 57600,
				      .parity = UART_CFG_PARITY_NONE,
				      .stop_bits = UART_CFG_STOP_BITS_1,
				      .data_bits = UART_CFG_DATA_BITS_8,
				      .flow_ctrl = UART_CFG_FLOW_CTRL_NONE };

static void uart_pending_callback(const struct device *dev, void *user_data)
{
	ARG_UNUSED(user_data);

	// int num_tries = 0;
	uint8_t rx_buffer[UART_RX_BUFF_SIZE];

	while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
		// if (!uart_irq_rx_ready(dev)) {
		// 	if (num_tries < MAX_NUM_TRIES) {
		// 		num_tries++;
		// 		continue;
		// 	} else {
		// 		break;
		// 	}
		// }

		int recv_len = uart_fifo_read(dev, rx_buffer, sizeof(rx_buffer));
		if (recv_len) {
			uint32_t rx_len = ring_buf_put(&uart_ring_buffer, rx_buffer, recv_len);
			if (rx_len < recv_len) {
				LOG_ERR("Drop %u bytes", recv_len - rx_len);
			}
			k_sem_give(&uart_handler_sem);
		}
	}
}

static void check_prot_error(char *str_p, int str_len)
{
	char *cur_p = str_p;
	char *ret;
	while (1) {
		ret = strstr(cur_p, PROT_ERR_STR_PREFIX);
		if (ret && ((ret - str_p + PROT_ERR_STR_LEN) <= str_len) &&
		    ret[PROT_ERR_STR_LEN - 1] == PROT_ERR_STR_SUFFIX) {
			cur_p = ret + 1;

			common_addsel_msg_t sel_msg;
			sel_msg.InF_target = BMC_IPMB;
			sel_msg.sensor_type = IPMI_OEM_SENSOR_TYPE_PROT;
			sel_msg.event_type = IPMI_EVENT_TYPE_SENSOR_SPECIFIC;
			sel_msg.sensor_number = SENSOR_NUM_PROT_ERROR;
			sel_msg.event_data1 = ret[2];
			sel_msg.event_data2 = ret[3];
			sel_msg.event_data3 = ret[4];
			if (!common_add_sel_evt_record(&sel_msg))
				LOG_ERR("Failed to add PRoT SEL.");
		} else {
			return;
		}
	}
}

static void uart_handler(void *arug0, void *arug1, void *arug2)
{
	ARG_UNUSED(arug0);
	ARG_UNUSED(arug1);
	ARG_UNUSED(arug2);

	char rx_buff[UART_RX_BUFF_SIZE];
	int rx_len;
	uint8_t tmp_str_len = PROT_ERR_STR_LEN - 1;
	memset(rx_buff, 0x20, sizeof(rx_buff));

	while (1) {
		k_sem_take(&uart_handler_sem, K_FOREVER);

		while (1) {
			rx_len = ring_buf_get(&uart_ring_buffer, &rx_buff[tmp_str_len],
					      sizeof(rx_buff) - tmp_str_len - 1);
			if (!rx_len)
				break;

			check_prot_error(rx_buff, rx_len + tmp_str_len);

			strncpy(rx_buff, &rx_buff[rx_len], tmp_str_len);
			memset(&rx_buff[tmp_str_len], 0, sizeof(rx_buff) - tmp_str_len);
		}
		k_msleep(10);
	}
}

void uart_init()
{
	const struct device *uart_dev = device_get_binding(PROT_UART_PORT);
	if (!uart_dev) {
		LOG_ERR("Cannot get UART device.");
	}

	int ret = uart_configure(uart_dev, &uart_cfg);
	if (ret) {
		LOG_ERR("Failed to configure UART device.");
	}

	k_sem_init(&uart_handler_sem, 0, 1);

	k_thread_create(&uart_thread, uart_handler_stack, K_THREAD_STACK_SIZEOF(uart_handler_stack),
			uart_handler, NULL, NULL, NULL, CONFIG_MAIN_THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&uart_thread, "UART_handler");

	uart_irq_callback_set(uart_dev, uart_pending_callback);
	uart_irq_tx_disable(uart_dev);
	uart_irq_rx_enable(uart_dev);
}
