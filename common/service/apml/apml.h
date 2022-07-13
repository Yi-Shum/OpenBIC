#ifndef APML_H
#define APML_H

#define APML_SUCCESS 0
#define APML_ERROR 1

enum APML_MSG_TYPE {
	APML_MSG_TYPE_MAILBOX,
	APML_MSG_TYPE_CPUID,
	APML_MSG_TYPE_MCA,
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

typedef struct _mailbox_WrData_ {
	uint8_t command;
	uint8_t data_in[4];
} mailbox_WrData;

typedef struct _mailbox_RdData_ {
	uint8_t command;
	uint8_t data_out[4];
	uint8_t error_code;
} mailbox_RdData;

typedef struct _cpuid_WrData_ {
	uint8_t thread;
	uint8_t cpuid_func[4];
	uint8_t ecx_value;
} cpuid_WrData;

typedef struct _cpuid_RdData_ {
	uint8_t status;
	uint8_t data_out[8];
} cpuid_RdData;

typedef struct _mca_WrData_ {
	uint8_t thread;
	uint8_t register_addr[4];
} mca_WrData;

typedef struct _mca_RdData_ {
	uint8_t status;
	uint8_t data_out[8];
} mca_RdData;

typedef __aligned(4) struct _apml_msg_ {
	uint8_t msg_type;
	uint8_t bus;
	uint8_t target_addr;
	uint8_t WrData[7];
	uint8_t RdData[9];
	void (*cb_fn)(struct _apml_msg_ *msg);
	void *ptr_arg;
	uint32_t ui32_arg;
} __packed apml_msg;

typedef struct _apml_buffer_ {
	uint8_t index;
	apml_msg msg;
} apml_buffer;

uint8_t TSI_read(uint8_t bus, uint8_t addr, uint8_t offset, uint8_t *read_data);
uint8_t TSI_write(uint8_t bus, uint8_t addr, uint8_t offset, uint8_t write_data);
uint8_t RMI_read(uint8_t bus, uint8_t addr, uint8_t offset, uint8_t *read_data);
uint8_t RMI_write(uint8_t bus, uint8_t addr, uint8_t offset, uint8_t write_data);
void request_apml_callback_fn(apml_msg *msg);
uint8_t get_apml_response_by_index(apml_msg *msg, uint8_t index);
uint8_t apml_read(apml_msg *msg);
void apml_init();

#endif
