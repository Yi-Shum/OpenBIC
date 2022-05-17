
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
	if (!TSI_write(bus, addr, SBTSI_HIGH_TEMP_INT, temp_threshold)) {
		return false;
	}

	if (alert_comparator_mode) {
		if (TSI_write(bus, addr, SBTSI_ALERT_CONFIG, 0x01)) {
			return false;
		}
	} else {
		if (TSI_write(bus, addr, SBTSI_ALERT_CONFIG, 0x00)) {
			return false;
		}
	}

	if (TSI_write(bus, addr, SBTSI_UPDATE_RATE, rate)) {
		return false;
	}
	return true;
}

/****************** RMI *********************/

bool RMI_read(uint8_t bus, uint8_t addr, uint8_t offset, uint8_t *read_data)
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

bool RMI_write(uint8_t bus, uint8_t addr, uint8_t offset, uint8_t write_data)
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

static bool write_MCA_request(mca_msg *msg)
{
	uint8_t retry = 5;
	I2C_MSG i2c_msg;
	i2c_msg.bus = msg->bus;
	i2c_msg.target_addr = msg->target_addr;
	i2c_msg.tx_len = 9;
	i2c_msg.data[0] = 0x73;
	i2c_msg.data[1] = 0x07;
	i2c_msg.data[2] = 0x08;
	i2c_msg.data[3] = 0x86;
	i2c_msg.data[4] = msg->thread << 1;
	i2c_msg.data[5] = msg->WrData[0];
	i2c_msg.data[6] = msg->WrData[1];
	i2c_msg.data[7] = msg->WrData[2];
	i2c_msg.data[8] = msg->WrData[3];

	if (i2c_master_write(&i2c_msg, retry)) {
		return false;
	}
	return true;
}

static bool read_MCA_response(mca_msg *msg)
{
	uint8_t retry = 5;
	I2C_MSG i2c_msg;
	i2c_msg.bus = msg->bus;
	i2c_msg.target_addr = msg->target_addr;
	i2c_msg.tx_len = 1;
	i2c_msg.rx_len = 10;
	i2c_msg.data[0] = 0x73;
	if (i2c_master_read(&i2c_msg, retry)) {
		return false;
	}

	msg->status = i2c_msg.data[1];
	memcpy(msg->RdData, &i2c_msg.data[2], 8);
	return true;
}

static void access_MCA(mca_msg *msg)
{
	/* block write request */
	if (!write_MCA_request(msg)) {
		printf("[%s] write MCA request fail.\n", __func__);
		return;
	}

	/* wait HwAlert */
	int i = 0;
	uint8_t read_data;
	for (; i < RETRY_MAX; i++) {
		if (RMI_read(msg->bus, msg->target_addr, SBRMI_STATUS, &read_data)) {
			if (read_data & 0x80) {
				break;
			}
		}
		k_msleep(WAIT_TIME_MS);
	}
	if (i == RETRY_MAX) {
		printf("[%s] MCA wait HwAlert retry max.\n", __func__);
		return;
	}

	/* block read response */
	if (!read_MCA_response(msg)) {
		printf("[%s] read response fail.\n", __func__);
		return;
	}

	/* clear HwAlert */
	for (; i < RETRY_MAX; i++) {
		if (RMI_write(msg->bus, msg->target_addr, SBRMI_STATUS, 0x80)) {
			break;
		}
		k_msleep(WAIT_TIME_MS);
	}
	if (i == RETRY_MAX) {
		printf("[%s] CPUID clear HwAlert retry max.\n", __func__);
		return;
	}
}

/****************** CPUID *******************/

static bool write_CPUID_request(cpuid_msg *msg)
{
	uint8_t retry = 5;
	I2C_MSG i2c_msg;
	i2c_msg.bus = msg->bus;
	i2c_msg.target_addr = msg->target_addr;
	i2c_msg.tx_len = 10;
	i2c_msg.data[0] = 0x73;
	i2c_msg.data[1] = 0x08;
	i2c_msg.data[2] = 0x08;
	i2c_msg.data[3] = 0x91;
	i2c_msg.data[4] = msg->thread << 1;
	i2c_msg.data[5] = msg->WrData[0];
	i2c_msg.data[6] = msg->WrData[1];
	i2c_msg.data[7] = msg->WrData[2];
	i2c_msg.data[8] = msg->WrData[3];
	i2c_msg.data[9] = msg->exc_value;

	if (i2c_master_write(&i2c_msg, retry)) {
		return false;
	}
	return true;
}

static bool read_CPUID_response(cpuid_msg *msg)
{
	uint8_t retry = 5;
	I2C_MSG i2c_msg;
	i2c_msg.bus = msg->bus;
	i2c_msg.target_addr = msg->target_addr;
	i2c_msg.tx_len = 1;
	i2c_msg.rx_len = 10;
	i2c_msg.data[0] = 0x73;
	if (i2c_master_read(&i2c_msg, retry)) {
		return false;
	}

	msg->status = i2c_msg.data[1];
	memcpy(msg->RdData, &i2c_msg.data[2], 8);
	return true;
}

static void access_CPUID(cpuid_msg *msg)
{
	/* block write request */
	if (!write_CPUID_request(msg)) {
		printf("[%s] write CPUID request fail.\n", __func__);
		return;
	}

	/* wait HwAlert */
	int i = 0;
	uint8_t read_data;
	for (; i < RETRY_MAX; i++) {
		if (RMI_read(msg->bus, msg->target_addr, SBRMI_STATUS, &read_data)) {
			if (read_data & 0x80) {
				break;
			}
		}
		k_msleep(WAIT_TIME_MS);
	}
	if (i == RETRY_MAX) {
		printf("[%s] CPUID wait HwAlert retry max.\n", __func__);
		return;
	}

	/* block read response */
	if (!read_CPUID_response(msg)) {
		printf("[%s] read CPUID response fail.\n", __func__);
		return;
	}

	/* clear HwAlert */
	for (; i < RETRY_MAX; i++) {
		if (RMI_write(msg->bus, msg->target_addr, SBRMI_STATUS, 0x80)) {
			break;
		}
		k_msleep(WAIT_TIME_MS);
	}
	if (i == RETRY_MAX) {
		printf("[%s] CPUID clear HwAlert retry max.\n", __func__);
		return;
	}
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
		printf("[%s] read offset 0x%02x fail.\n", __func__, SBRMI_OUTBANDMSG_INST0);
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

	if (!RMI_read(msg->bus, msg->target_addr, SBRMI_OUTBANDMSG_INST7, &msg->error_code)) {
		printf("[%s] read offset 0x%02x fail.\n", __func__, SBRMI_OUTBANDMSG_INST7);
		return false;
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
}

bool apml_read(apml_msg *msg)
{
	if (msg == NULL) {
		printk("[%s] msg is NULL\n", __func__);
		return false;
	}

	if (k_msgq_put(&apml_msgq, msg, K_FOREVER)) {
		return false;
	}
	return true;
}

static void apml_handler(void *arvg0, void *arvg1, void *arvg2)
{
	apml_msg msg_cfg;

	while (1) {
		memset(&msg_cfg, 0, sizeof(apml_msg));
		k_msgq_get(&apml_msgq, &msg_cfg, K_FOREVER);

		switch (msg_cfg.msg_type) {
		case APML_MSG_TYPE_MAILBOX:
			access_RMI_mailbox(&msg_cfg.data.mailbox);
			break;
		case APML_MSG_TYPE_CPUID:
			access_CPUID(&msg_cfg.data.cpuid);
			break;
		case APML_MSG_TYPE_MCA:
			access_MCA(&msg_cfg.data.mca);
			break;
		default:
			break;
		}

		if (msg_cfg.cb_fn) {
			msg_cfg.cb_fn(&msg_cfg);
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
