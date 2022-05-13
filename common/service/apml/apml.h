#ifndef APML_H
#define APML_H

#endif
#ifndef IPMB_H
#define IPMB_H

#define APML_HANDLER_STACK_SIZE 2048
#define APML_BUF_LEN 5
#define APML_BUFF_SIZE 4
#define APML_MAILBOX_TIMEOUT_MS 100

#define APML_SUCCESS 0
#define APML_ERROR 1

enum {
	TSI_CPU_TEMP_INT = 0x01,
	TSI_CONFIG = 0x03,
	TSI_UPDATE_RATE = 0x04,
	TSI_HIGH_TEMP_INT = 0x07,
	TSI_CPU_TEMP_DEC = 0x10,
	TSI_ALERT_CONFIG = 0xBF,
};

enum {
	APML_MAILBOX,
	APML_CPUID,
	APML_MCA,
};

enum {
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

typedef struct _mailbox_msg_ {
	uint8_t bus;
	uint8_t target_addr;
	void (*cb_fn)(struct _mailbox_msg_ *msg);
	uint8_t command;
	uint8_t data_in[4];
	uint8_t response_command;
	uint8_t data_out[4];
	uint8_t error_code;
} __packed __aligned(4) mailbox_msg;

typedef struct __cpuid_msg_ {
} __packed __aligned(4) cpuid_msg;

typedef struct __mca_msg_ {
} __packed __aligned(4) mca_msg;

typedef struct _apml_msg_ {
	uint8_t msg_type;
	union {
		mailbox_msg mailbox;
		cpuid_msg cpuid;
		mca_msg mca;
	} data;
} __packed __aligned(4) apml_msg;

bool TSI_read(uint8_t bus, uint8_t addr, uint8_t command, uint8_t *read_data);
bool TSI_write(uint8_t bus, uint8_t addr, uint8_t command, uint8_t write_data);
bool TSI_set_temperature_throttle(uint8_t bus, uint8_t addr, uint8_t temp_threshold, uint8_t rate,
				  bool alert_comparator_mode);

#endif
