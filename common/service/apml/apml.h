#ifndef APML_H
#define APML_H

#define APML_SUCCESS 0
#define APML_ERROR 1

#define APML_HANDLER_STACK_SIZE 2048
#define APML_BUF_LEN 5
#define APML_BUFF_SIZE 4
#define APML_MAILBOX_TIMEOUT_MS 100
#define APML_RESP_BUFF_SIZE 10

enum APML_MSG_TYPE {
	APML_MSG_TYPE_MAILBOX,
	APML_MSG_TYPE_CPUID,
	APML_MSG_TYPE_MCA,
};

enum SBRMI_MAILBOX_CMD {
	SBRMI_MAILBOX_PKGPWR = 0x01,
	SBRMI_MAILBOX_GET_DIMM_PWR = 0x47,
	SBRMI_MAILBOX_GET_DIMM_TEMP = 0x48,
};

enum SBTSI_REGISTER {
	SBTSI_CPU_TEMP_INT = 0x01,
	SBTSI_STATUS = 0x02,
	SBTSI_CONFIG = 0x03,
	SBTSI_UPDATE_RATE = 0x04,
	SBTSI_HIGH_TEMP_INT = 0x07,
	SBTSI_CPU_TEMP_DEC = 0x10,
	SBTSI_ALERT_CONFIG = 0xBF,
};

enum SBRMI_REGISTER {
	SBRMI_STATUS = 0x02,
	SBRMI_OUTBANDMSG_INST0 = 0x30,
	SBRMI_OUTBANDMSG_INST1 = 0x31,
	SBRMI_OUTBANDMSG_INST2 = 0x32,
	SBRMI_OUTBANDMSG_INST3 = 0x33,
	SBRMI_OUTBANDMSG_INST4 = 0x34,
	SBRMI_OUTBANDMSG_INST5 = 0x35,
	SBRMI_OUTBANDMSG_INST6 = 0x36,
	SBRMI_OUTBANDMSG_INST7 = 0x37,
	SBRMI_INBANDMSG_INST0 = 0x38,
	SBRMI_INBANDMSG_INST1 = 0x39,
	SBRMI_INBANDMSG_INST2 = 0x3A,
	SBRMI_INBANDMSG_INST3 = 0x3B,
	SBRMI_INBANDMSG_INST4 = 0x3C,
	SBRMI_INBANDMSG_INST5 = 0x3D,
	SBRMI_INBANDMSG_INST6 = 0x3E,
	SBRMI_INBANDMSG_INST7 = 0x3F,
	SBRMI_SOFTWARE_INTERRUPT = 0x40,
	SBRMI_RAS_STATUS = 0x4C,
};

enum SBRMI_MAILBOX_ERR_CODE {
	SBRMI_MAILBOX_NO_ERR = 0x00,
	SBRMI_MAILBOX_CMD_ABORT = 0x01,
	SBRMI_MAILBOX_UNKNOWN_CMD = 0x02,
	SBRMI_MAILBOX_INVALID_CORE = 0x03,
};

typedef __aligned(4) struct _mailbox_msg_ {
	uint8_t bus;
	uint8_t target_addr;
	uint8_t command;
	uint8_t data_in[4];
	uint8_t response_command;
	uint8_t data_out[4];
	uint8_t error_code;
} __packed mailbox_msg;

typedef __aligned(4) struct __cpuid_msg_ {
	uint8_t bus;
	uint8_t target_addr;
	uint8_t thread;
	uint8_t WrData[4];
	uint8_t exc_value;
	uint8_t status;
	uint8_t RdData[8];
} __packed cpuid_msg;

typedef __aligned(4) struct __mca_msg_ {
	uint8_t bus;
	uint8_t target_addr;
	uint8_t thread;
	uint8_t WrData[4];
	uint8_t status;
	uint8_t RdData[8];
} __packed mca_msg;

typedef __aligned(4) struct _apml_msg_ {
	uint8_t msg_type;
	union {
		mailbox_msg mailbox;
		cpuid_msg cpuid;
		mca_msg mca;
	} data;
	void (*cb_fn)(struct _apml_msg_ *msg);
	void *ptr_arg;
	uint32_t ui32_arg;
} __packed apml_msg;

uint8_t TSI_read(uint8_t bus, uint8_t addr, uint8_t command, uint8_t *read_data);
uint8_t TSI_write(uint8_t bus, uint8_t addr, uint8_t command, uint8_t write_data);
uint8_t TSI_set_temperature_throttle(uint8_t bus, uint8_t addr, uint8_t temp_threshold,
				     uint8_t rate, bool alert_comparator_mode);
uint8_t RMI_read(uint8_t bus, uint8_t addr, uint8_t offset, uint8_t *read_data);
uint8_t RMI_write(uint8_t bus, uint8_t addr, uint8_t offset, uint8_t write_data);
void callback_store_response(apml_msg *msg);
uint8_t get_apml_response(uint8_t *data, uint8_t array_size, uint8_t *data_len);
uint8_t apml_read(apml_msg *msg);
void apml_init();

#endif
