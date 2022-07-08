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
static uint8_t apml_resp_len, apml_resp_buf[APML_RESP_BUFF_SIZE];

/****************** TSI *********************/

uint8_t TSI_read(uint8_t bus, uint8_t addr, uint8_t command, uint8_t *read_data)
{
	uint8_t retry = 5;
	I2C_MSG msg;
	msg.bus = bus;
	msg.target_addr = addr;
	msg.tx_len = 1;
	msg.rx_len = 1;
	msg.data[0] = command;

	if (i2c_master_read(&msg, retry)) {
		return APML_ERROR;
	}
	*read_data = msg.data[0];
	return APML_SUCCESS;
}

uint8_t TSI_write(uint8_t bus, uint8_t addr, uint8_t command, uint8_t write_data)
{
	uint8_t retry = 5;
	I2C_MSG msg;
	msg.bus = bus;
	msg.target_addr = addr;
	msg.tx_len = 2;
	msg.rx_len = 0;
	msg.data[0] = command;
	msg.data[1] = write_data;

	if (i2c_master_write(&msg, retry)) {
		return APML_ERROR;
	}
	return APML_SUCCESS;
}

/****************** RMI *********************/

uint8_t RMI_read(uint8_t bus, uint8_t addr, uint8_t offset, uint8_t *read_data)
{
	uint8_t retry = 5;
	I2C_MSG msg;
	msg.bus = bus;
	msg.target_addr = addr;
	msg.tx_len = 1;
	msg.rx_len = 1;
	msg.data[0] = offset;

	if (i2c_master_read(&msg, retry)) {
		return APML_ERROR;
	} else {
		*read_data = msg.data[0];
		return APML_SUCCESS;
	}
}

uint8_t RMI_write(uint8_t bus, uint8_t addr, uint8_t offset, uint8_t write_data)
{
	uint8_t retry = 5;
	I2C_MSG msg;
	msg.bus = bus;
	msg.target_addr = addr;
	msg.tx_len = 2;
	msg.rx_len = 0;
	msg.data[0] = offset;
	msg.data[1] = write_data;

	if (i2c_master_write(&msg, retry)) {
		return APML_ERROR;
	} else {
		return APML_SUCCESS;
	}
}

/****************** MCA *********************/

static uint8_t write_MCA_request(mca_msg *msg)
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
		return APML_ERROR;
	}
	return APML_SUCCESS;
}

static uint8_t read_MCA_response(mca_msg *msg)
{
	uint8_t retry = 5;
	I2C_MSG i2c_msg;
	i2c_msg.bus = msg->bus;
	i2c_msg.target_addr = msg->target_addr;
	i2c_msg.tx_len = 1;
	i2c_msg.rx_len = 10;
	i2c_msg.data[0] = 0x73;
	if (i2c_master_read(&i2c_msg, retry)) {
		return APML_ERROR;
	}

	msg->status = i2c_msg.data[1];
	memcpy(msg->RdData, &i2c_msg.data[2], 8);
	return APML_SUCCESS;
}

static void access_MCA(mca_msg *msg)
{
	/* write request */
	if (write_MCA_request(msg)) {
		printf("[%s] write MCA request fail.\n", __func__);
		return;
	}

	/* wait HwAlert */
	int i = 0;
	uint8_t read_data;
	for (; i < RETRY_MAX; i++) {
		if (!RMI_read(msg->bus, msg->target_addr, SBRMI_STATUS, &read_data)) {
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

	/* read response */
	if (read_MCA_response(msg)) {
		printf("[%s] read response fail.\n", __func__);
		return;
	}

	/* clear HwAlert */
	if (RMI_write(msg->bus, msg->target_addr, SBRMI_STATUS, 0x80)) {
		printf("[%s] clear HwAlert fail.\n", __func__);
		return;
	}
}

/****************** CPUID *******************/

static uint8_t write_CPUID_request(cpuid_msg *msg)
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
		return APML_ERROR;
	}
	return APML_SUCCESS;
}

static uint8_t read_CPUID_response(cpuid_msg *msg)
{
	uint8_t retry = 5;
	I2C_MSG i2c_msg;
	i2c_msg.bus = msg->bus;
	i2c_msg.target_addr = msg->target_addr;
	i2c_msg.tx_len = 1;
	i2c_msg.rx_len = 10;
	i2c_msg.data[0] = 0x73;
	if (i2c_master_read(&i2c_msg, retry)) {
		return APML_ERROR;
	}

	msg->status = i2c_msg.data[1];
	memcpy(msg->RdData, &i2c_msg.data[2], 8);
	return APML_SUCCESS;
}

static void access_CPUID(cpuid_msg *msg)
{
	/* block write request */
	if (write_CPUID_request(msg)) {
		printf("[%s] write CPUID request fail.\n", __func__);
		return;
	}

	/* wait HwAlert */
	int i = 0;
	uint8_t read_data;
	for (; i < RETRY_MAX; i++) {
		if (!RMI_read(msg->bus, msg->target_addr, SBRMI_STATUS, &read_data)) {
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
	if (read_CPUID_response(msg)) {
		printf("[%s] read CPUID response fail.\n", __func__);
		return;
	}

	/* clear HwAlert */
	if (RMI_write(msg->bus, msg->target_addr, SBRMI_STATUS, 0x80)) {
		printf("[%s] clear HwAlert fail.\n", __func__);
		return;
	}
}

/****************** RMI Mailbox**************/

static bool check_mailbox_command_complete(mailbox_msg *msg)
{
	uint8_t read_data;
	if (RMI_read(msg->bus, msg->target_addr, SBRMI_SOFTWARE_INTERRUPT, &read_data)) {
		return false;
	}
	return (read_data & 0x01) ? false : true;
}

static uint8_t write_mailbox_request(mailbox_msg *msg)
{
	/* initialize */
	uint8_t read_data;

	if (RMI_read(msg->bus, msg->target_addr, SBRMI_INBANDMSG_INST7, &read_data)) {
		return APML_ERROR;
	}
	if (!(read_data & 0x80)) {
		if (RMI_write(msg->bus, msg->target_addr, SBRMI_INBANDMSG_INST7, 0x80)) {
			return APML_ERROR;
		}
	}

	/* write command */
	if (RMI_write(msg->bus, msg->target_addr, SBRMI_INBANDMSG_INST0, msg->command)) {
		printf("[%s] write commad fail.\n", __func__);
		return APML_ERROR;
	}

	/* write data */
	for (uint8_t offset = SBRMI_INBANDMSG_INST1, i = 0; offset <= SBRMI_INBANDMSG_INST4;
	     offset++, i++) {
		if (RMI_write(msg->bus, msg->target_addr, offset, msg->data_in[i])) {
			printf("[%s] write data of offset 0x%02x fail.\n", __func__, offset);
			return APML_ERROR;
		}
	}

	/* write 0x01 to software interrupt to perform request */
	if (RMI_write(msg->bus, msg->target_addr, SBRMI_SOFTWARE_INTERRUPT, 0x01)) {
		printf("[%s] write software interrupt fail.\n", __func__);
		return APML_ERROR;
	}

	return APML_SUCCESS;
}

static uint8_t read_mailbox_response(mailbox_msg *msg)
{
	if (RMI_read(msg->bus, msg->target_addr, SBRMI_OUTBANDMSG_INST0, &msg->response_command)) {
		printf("[%s] read offset 0x%02x fail.\n", __func__, SBRMI_OUTBANDMSG_INST0);
		return APML_ERROR;
	}
	for (uint8_t offset = SBRMI_OUTBANDMSG_INST1, i = 0; offset <= SBRMI_OUTBANDMSG_INST4;
	     offset++, i++) {
		if (RMI_read(msg->bus, msg->target_addr, offset, &msg->data_out[i])) {
			printf("[%s] read offset 0x%02x fail.\n", __func__, offset);
			return APML_ERROR;
		}
	}

	if (RMI_read(msg->bus, msg->target_addr, SBRMI_OUTBANDMSG_INST7, &msg->error_code)) {
		printf("[%s] read offset 0x%02x fail.\n", __func__, SBRMI_OUTBANDMSG_INST7);
		return APML_ERROR;
	}

	return APML_SUCCESS;
}

static bool access_RMI_mailbox(mailbox_msg *msg)
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
		return false;
	}

	if (write_mailbox_request(msg)) {
		printf("[%s] mailbox writet request fail.\n", __func__);
		return false;
	}

	/* wait complete */
	for (i = 0; i < RETRY_MAX; i++) {
		if (check_mailbox_command_complete(msg)) {
			break;
		}
		k_msleep(WAIT_TIME_MS);
	}
	if (i == RETRY_MAX) {
		printf("[%s] mailbox command not complete after retry %d.\n", __func__, RETRY_MAX);
		return false;
	}

	if (read_mailbox_response(msg)) {
		printf("[%s] read mailbox response fail\n", __func__);
		return false;
	}

	uint8_t status;
	/* check SwAlertSts is be set */
	for (i = 0; i < RETRY_MAX; i++) {
		if (RMI_read(msg->bus, msg->target_addr, SBRMI_STATUS, &status)) {
			printf("[%s] read status fail\n", __func__);
			return false;
		}
		if (status & 0x02) {
			break;
		}
		k_msleep(WAIT_TIME_MS);
	}
	if (i == RETRY_MAX) {
		printf("[%s] mailbox command wait SwAlertSts retry %d.\n", __func__, RETRY_MAX);
		return false;
	}

	/* clear software alert status */
	if (RMI_write(msg->bus, msg->target_addr, SBRMI_STATUS, 0x02)) {
		printf("[%s] clear software alert fail\n", __func__);
		return false;
	}
	return true;
}

void callback_store_response(apml_msg *msg)
{
	switch (msg->msg_type) {
	case APML_MSG_TYPE_MAILBOX: {
		mailbox_msg *mb_msg = &msg->data.mailbox;
		apml_resp_buf[0] = APML_MSG_TYPE_MAILBOX;
		apml_resp_buf[1] = mb_msg->response_command;
		memcpy(&apml_resp_buf[2], mb_msg->data_out, 4);
		apml_resp_buf[6] = mb_msg->error_code;
		apml_resp_len = 7;
		break;
	}
	case APML_MSG_TYPE_CPUID: {
		cpuid_msg *cpuid_msg = &msg->data.cpuid;
		apml_resp_buf[0] = APML_MSG_TYPE_CPUID;
		apml_resp_buf[1] = cpuid_msg->status;
		memcpy(&apml_resp_buf[2], cpuid_msg->RdData, 8);
		apml_resp_len = 10;
		break;
	}
	case APML_MSG_TYPE_MCA: {
		mca_msg *mca_msg = &msg->data.mca;
		apml_resp_buf[0] = APML_MSG_TYPE_MCA;
		apml_resp_buf[1] = mca_msg->status;
		memcpy(&apml_resp_buf[2], mca_msg->RdData, 8);
		apml_resp_len = 10;
		break;
	}
	default:
		break;
	}
}

uint8_t get_apml_response(uint8_t *data, uint8_t array_size, uint8_t *data_len)
{
	if ((data == NULL) || (apml_resp_len == 0) || apml_resp_len > array_size) {
		return APML_ERROR;
	}
	memcpy(data, apml_resp_buf, apml_resp_len);
	*data_len = apml_resp_len;
	apml_resp_len = 0;
	return APML_SUCCESS;
}

uint8_t apml_read(apml_msg *msg)
{
	if (msg == NULL) {
		printf("[%s] msg is NULL\n", __func__);
		return APML_ERROR;
	}

	if (k_msgq_put(&apml_msgq, msg, K_NO_WAIT)) {
		printf("[%s] put message to apml_msgq fail\n", __func__);
		return APML_ERROR;
	}
	return APML_SUCCESS;
}

static void apml_handler(void *arvg0, void *arvg1, void *arvg2)
{
	apml_msg msg_cfg;

	while (1) {
		memset(&msg_cfg, 0, sizeof(apml_msg));
		k_msgq_get(&apml_msgq, &msg_cfg, K_FOREVER);

		switch (msg_cfg.msg_type) {
		case APML_MSG_TYPE_MAILBOX:
			if (!access_RMI_mailbox(&msg_cfg.data.mailbox)) {
				continue;
			}
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
		k_msleep(10);
	}
}

void apml_init()
{
	printf("apml_init\n");
	k_msgq_init(&apml_msgq, apml_msgq_buffer, sizeof(apml_msg), APML_BUF_LEN);

	k_thread_create(&apml_thread, APML_HANDLER_stack, K_THREAD_STACK_SIZEOF(APML_HANDLER_stack),
			apml_handler, NULL, NULL, NULL, CONFIG_MAIN_THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&apml_thread, "APML_handler");
}
