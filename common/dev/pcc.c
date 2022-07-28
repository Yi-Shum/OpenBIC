#ifdef CONFIG_PCC_ASPEED

#include <zephyr.h>
#include <device.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <drivers/misc/aspeed/pcc_aspeed.h>
#include "ipmb.h"
#include "ipmi.h"
#include "libutil.h"
#include "pcc.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(pcc_test, 4);

K_THREAD_STACK_DEFINE(pcc_thread, PCC_STACK_SIZE);
struct k_thread pcc_thread_handler;
k_tid_t pcc_tid;

const struct device *pcc_dev;
static uint32_t pcc_read_buffer[PCC_BUFFER_LEN];
static uint16_t pcc_read_len, pcc_read_index = 0;

uint16_t copy_pcc_read_buffer(uint16_t start, uint16_t length, uint8_t *buffer, uint16_t buffer_len)
{
	if (!buffer) {
		return 0;
	}
	if (buffer_len < (length * 4)) {
		return 0;
	}

	uint16_t start_index, current_index;
	if (start < pcc_read_index) {
		start_index = pcc_read_index - start - 1;
	} else {
		start_index = pcc_read_index + PCC_BUFFER_LEN - start - 1;
	}

	uint16_t i = 0;
	for (; (i < length) && ((i + start) < pcc_read_len); i++) {
		current_index = start_index--;
		if (start_index == 0) {
			start_index = PCC_BUFFER_LEN - 1;
		}

		buffer[4 * i] = pcc_read_buffer[current_index] & 0xFF;
		buffer[(4 * i) + 1] = (pcc_read_buffer[current_index] >> 8) & 0xFF;
		buffer[(4 * i) + 2] = (pcc_read_buffer[current_index] >> 16) & 0xFF;
		buffer[(4 * i) + 3] = (pcc_read_buffer[current_index] >> 24) & 0xFF;
	}
	return 4 * i;
}

void pcc_read()
{
	/* Known issue: Some post codes may be lost if host sent too frequently.
	 *
	 * HOST sends lots of post codes in a period of time.
	 * This caused the PCC driver to fail to allocate memory 
	 * because we didn't have time to read the data back.
	 * 
	 */
	int rc;
	uint8_t data, addr;
	uint32_t four_byte_data = 0;

	/* The sequence of read data from pcc driver is:
	 * data[7:0], addr[3:0], data[7:0], addr[3:0], ......
	 * so we expect to get data as following sequence
	 * 0x12, 0x40, 0x34, 0x41, 0x56, 0x42, 0x78, 0x43, ......
	 * 0x40, 0x41, 0x42, 0x43 means the data is get from address 0x80, 0x81, 0x82, 0x83
	 */
	while (1) {
		rc = pcc_aspeed_read(pcc_dev, &data, true);
		if (rc) {
			LOG_ERR("[%s] pcc read data fail\n", __func__);
			continue;
		}
		rc = pcc_aspeed_read(pcc_dev, &addr, true);
		if (rc) {
			LOG_ERR("[%s] pcc read addr fail\n", __func__);
			continue;
		}

		while ((addr < 0x40) || (addr > 0x43)) {
			LOG_ERR("[%s] wrong sequence, read addr again\n", __func__);
			data = addr;
			rc = pcc_aspeed_read(pcc_dev, &addr, true);
			if (rc) {
				LOG_ERR("[%s] pcc read addr fail\n", __func__);
				continue;
			}
		}

		four_byte_data |= data << (8 * (addr & 0x0F));
		if ((addr & 0x0F) == 0x03) {
			pcc_read_buffer[pcc_read_index] = four_byte_data;
			four_byte_data = 0;
			if (pcc_read_len < PCC_BUFFER_LEN) {
				pcc_read_len++;
			}
			pcc_read_index++;
			if (pcc_read_index == PCC_BUFFER_LEN) {
				pcc_read_index = 0;
			}
		}
	}
}

void pcc_init()
{
	/* set registers to read 4-byte postcode */
	pcc_dev = device_get_binding(DT_LABEL(DT_NODELABEL(pcc)));
	if (!pcc_dev) {
		printf("No pcc device found\n");
		return;
	}
	uint32_t reg_data;
	sys_write32(0x00820080, 0x7E789090);

	reg_data = sys_read32(0x7E789100);
	sys_write32(reg_data | 0x0000C000, 0x7E789100);

	reg_data = sys_read32(0x7E789084);
	sys_write32((reg_data | 0x00080000) & ~0x0001ffff, 0x7E789084);

	return;
}

void init_pcc_thread()
{
	pcc_read_len = 0;
	if (pcc_tid != NULL && strcmp(k_thread_state_str(pcc_tid), "dead") != 0) {
		return;
	}
	pcc_tid = k_thread_create(&pcc_thread_handler, pcc_thread,
				  K_THREAD_STACK_SIZEOF(pcc_thread), pcc_read, NULL, NULL, NULL,
				  CONFIG_MAIN_THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&pcc_thread_handler, "pcc_thread");
}

void send_4byte_post_code_to_BMC()
{
	uint16_t send_len, end_index, send_index = 0;

	ipmi_msg *msg;
	ipmb_error status;

	while (1) {
		k_msleep(100); // send post code to BMC once 100 ms
		end_index = pcc_read_index;

		if (end_index == send_index) {
			continue;
		} else if (end_index < send_index) {
			send_len = end_index + PCC_BUFFER_LEN - send_index;
		} else {
			send_len = end_index - send_index;
		}

		if (send_len > 60) {
			send_len = 60;
			end_index = (send_len + 60) % PCC_BUFFER_LEN;
		}

		msg = (ipmi_msg *)malloc(sizeof(ipmi_msg));
		if (msg == NULL) {
			printf("[%s] alloc ipmi_msg failed\n", __func__);
			continue;
		}

		memset(msg, 0, sizeof(ipmi_msg));
		msg->InF_source = SELF;
		msg->InF_target = BMC_IPMB;
		msg->netfn = NETFN_OEM_1S_REQ;
		msg->cmd = CMD_OEM_1S_SEND_4BYTE_POST_CODE_TO_BMC;
		msg->data_len = (send_len * 4) + 4;
		msg->data[0] = IANA_ID & 0xFF;
		msg->data[1] = (IANA_ID >> 8) & 0xFF;
		msg->data[2] = (IANA_ID >> 16) & 0xFF;
		msg->data[3] = send_len;
		copy_pcc_read_buffer(send_index, msg->data[3], &msg->data[4],
				     IPMI_MSG_MAX_LENGTH - 4);

		status = ipmb_read(msg, IPMB_inf_index_map[msg->InF_target]);
		SAFE_FREE(msg);
		if (status) {
			printf("[%s] Failed to send post code from index %d to %d, ret %d\n",
			       __func__, send_index, end_index, status);
			continue;
		}
		send_index = end_index;
	}
}

#endif
