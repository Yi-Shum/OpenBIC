
#include <stdio.h>
#include <string.h>
#include "hal_i2c.h"
#include "apml.h"

#define RETRY_MAX 3
#define WAIT_TIME_MS 10

struct k_msgq apml_msgq;
struct k_thread apml_thread;
char __aligned(4) apml_msgq_buffer[APML_BUF_LEN * sizeof(apml_msg)];
K_KERNEL_STACK_MEMBER(APML_HANDLER_stack, APML_HANDLER_STACK_SIZE);

/****************** TSI *********************/

bool TSI_read(uint8_t bus, uint8_t addr, uint8_t command, uint8_t *read_data)
{
	uint8_t retry = 5;
	I2C_MSG msg;
	msg.bus = bus;
	msg.target_addr = addr;
	msg.tx_len = 1;
	msg.rx_len = 1;
	msg.data[0] = command;

	if (i2c_master_read(&msg, retry)) {
		return false;
	}
	*read_data = msg.data[0];
	return true;
}

bool TSI_write(uint8_t bus, uint8_t addr, uint8_t command, uint8_t write_data)
{
	uint8_t retry = 5;
	I2C_MSG msg;
	msg.bus = bus;
	msg.target_addr = addr;
	msg.tx_len = 1;
	msg.rx_len = 0;
	msg.data[0] = command;

	if (i2c_master_write(&msg, retry)) {
		return false;
	}
	return true;
}

bool TSI_set_temperature_throttle(uint8_t bus, uint8_t addr, uint8_t temp_threshold, uint8_t rate,
				  bool alert_comparator_mode)
{
	/* 
	 * 1. Set high temperature threshold.
	 * 2. Set 'alert comparator mode' enable/disable.
	 * 3. Set update rate.
	 */
	if (!TSI_write(bus, addr, TSI_HIGH_TEMP_INT, temp_threshold)) {
		return false;
	}

	if (alert_comparator_mode) {
		if (TSI_write(bus, addr, TSI_ALERT_CONFIG, 0x01)) {
			return false;
		}
	} else {
		if (TSI_write(bus, addr, TSI_ALERT_CONFIG, 0x00)) {
			return false;
		}
	}

	if (TSI_write(bus, addr, TSI_UPDATE_RATE, rate)) {
		return false;
	}
	return true;
}

/****************** RMI *********************/

uint8_t RMI_read(uint8_t bus, uint8_t addr, uint8_t offset, uint8_t *read_data)
{
	uint8_t retry = 5;
	I2C_MSG msg;
	msg.bus = bus;
	msg.target_addr = addr;
	msg.tx_len = 1;
	msg.data[0] = offset;
	msg.rx_len = 1;

	if (i2c_master_read(&msg, retry)) {
		return false;
	} else {
		*read_data = msg.data[0];
		return true;
	}
}

uint8_t RMI_write(uint8_t bus, uint8_t addr, uint8_t offset, uint8_t write_data)
{
	uint8_t retry = 5;
	I2C_MSG msg;
	msg.bus = bus;
	msg.target_addr = addr;
	msg.tx_len = 2;
	msg.data[0] = offset;
	msg.data[1] = write_data;

	if (i2c_master_write(&msg, retry)) {
		return false;
	} else {
		return true;
	}
}

/****************** MCA *********************/

bool MSR_read(uint8_t bus, uint8_t addr, uint8_t threadAddr, uint8_t *MSRAddr, uint8_t *rawReading)
{
	return false;
}

/****************** CPUID *******************/

bool CPUIDRead(uint8_t bus, uint8_t addr, uint8_t threadAddr, uint8_t *CPUIDAddr, uint8_t ECXValue,
	       uint8_t *rawReading)
{
	return false;
}

/****************** RMI Mailbox**************/

bool check_mailbox_command_complete(mailbox_msg *msg)
{
	uint8_t read_data;
	if (!RMI_read(msg->bus, msg->target_addr, SBRMI_SOFTWARE_INTERRUPT, &read_data)) {
		return false;
	}
	return (read_data & 0x01) ? false : true;
}

bool write_mailbox_request(mailbox_msg *msg)
{
	/* initialize */
	uint8_t read_data;

	if (!RMI_read(msg->bus, msg->target_addr, SBRMI_INBANDMSG_INST7, &read_data)) {
		return false;
	}
	if (!(read_data & 0x80)) {
		if (!RMI_write(msg->bus, msg->target_addr, SBRMI_INBANDMSG_INST7, 0x80)) {
			return false;
		}
	}

	/* write command */
	if (!RMI_write(msg->bus, msg->target_addr, SBRMI_INBANDMSG_INST0, msg->command)) {
		return false;
	}

	/* write data */
	for (uint8_t offset = SBRMI_INBANDMSG_INST1, i = 0; offset <= SBRMI_INBANDMSG_INST4;
	     offset++, i++) {
		uint8_t write_data = msg->data_in[i];
		if (!RMI_write(msg->bus, msg->target_addr, offset, write_data)) {
			return false;
		}
	}

	return true;
}

bool read_mailbox_response(mailbox_msg *msg)
{
	int i = 0;
	for (; i < RETRY_MAX; i++) {
		if (!check_mailbox_command_complete(msg)) {
			k_msleep(WAIT_TIME_MS);
			continue;
		}
	}
	if (i == RETRY_MAX) {
		printf("[%s] mailbox command not complete after retry %d.\n", __func__, RETRY_MAX);
		return false;
	}

	if (!RMI_read(msg->bus, msg->target_addr, SBRMI_OUTBANDMSG_INST0, &msg->response_command)) {
		return false;
	}
	for (uint8_t offset = SBRMI_OUTBANDMSG_INST1, i = 0; offset <= SBRMI_OUTBANDMSG_INST4;
	     offset++, i++) {
		uint8_t read_data;
		if (!RMI_read(msg->bus, msg->target_addr, offset, &read_data)) {
			printf("[%s] read offset 0x%02x fail.\n", __func__, offset);
			return false;
		}
		msg->data_out[i] |= (read_data << (8 * i));
	}
	return true;
}

static void access_RMI_mailbox(mailbox_msg *msg)
{
	int i = 0;
	for (; i < RETRY_MAX; i++) {
		if (check_mailbox_command_complete(msg)) {
			break;
		}
		k_msleep(WAIT_TIME_MS);
	}
	if (i == RETRY_MAX) {
		printf("[%s] apml mailbox busy.\n", __func__);
		return;
	}

	if (!write_mailbox_request(msg)) {
		printf("[%s] mailbox writet request fail.\n", __func__);
		return;
	}
	/* write 0x01 to software interrupt to perform request */
	if (!RMI_write(msg->bus, msg->target_addr, SBRMI_SOFTWARE_INTERRUPT, 0x01)) {
		printf("[%s] mailbox writet software interrupt fail.\n", __func__);
		return;
	}

	if (!read_mailbox_response(msg)) {
		printf("[%s] read mailbox response fail\n", __func__);
		return;
	}
	/* clear software alert status */
	if (!RMI_write(msg->bus, msg->target_addr, SBRMI_STATUS, 0x02)) {
		printf("[%s] clear software alert fail\n", __func__);
		return;
	}

	if (msg->cb_fn) {
		msg->cb_fn(msg);
	}
}

void apml_handler(void *arvg0, void *arvg1, void *arvg2)
{
	apml_msg msg_cfg;

	while (1) {
		memset(&msg_cfg, 0, sizeof(apml_msg));
		k_msgq_get(&apml_msgq, &msg_cfg, K_FOREVER);

		switch (msg_cfg.msg_type) {
		case APML_MAILBOX:
			access_RMI_mailbox(&msg_cfg.data.mailbox);
			break;
		case APML_CPUID:
			break;
		case APML_MCA:
			break;
		default:
			break;
		}
	}
}

void apml_init()
{
	printf("apml_init\n");
	k_msgq_init(&apml_msgq, apml_msgq_buffer, sizeof(apml_msg), APML_BUF_LEN);

	k_thread_create(&apml_thread, APML_HANDLER_stack, K_THREAD_STACK_SIZEOF(APML_HANDLER_stack),
			apml_handler, NULL, NULL, NULL, CONFIG_MAIN_THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&apml_thread, "kcs_polling");
}
