#include "plat_ipmi.h"

#include <stdio.h>
#include <stdlib.h>

#include "libutil.h"
#include "ipmi.h"
#include "plat_class.h"
#include "plat_ipmb.h"
#include "hal_i3c_master.h"
#include "plat_i2c.h"

void APP_MASTER_WRITE_READ(ipmi_msg *msg)
{
	if (msg == NULL) {
		return;
	}

	uint8_t retry = 3;
	uint8_t bus_7bit;

	// at least include bus, addr, rx_len, offset
	if (msg->data_len < 4) {
		msg->completion_code = CC_INVALID_LENGTH;
		return;
	}

	// should ignore bit0, all bus public
	bus_7bit = msg->data[0] >> 1;
	if (bus_7bit < I2C_BUS_MAX_NUM) {
		I2C_MSG i2c_msg;
		i2c_msg.bus = bus_7bit;
		// 8 bit address to 7 bit
		i2c_msg.target_addr = msg->data[1] >> 1;
		i2c_msg.rx_len = msg->data[2];
		i2c_msg.tx_len = msg->data_len - 3;

		if (i2c_msg.tx_len == 0) {
			msg->completion_code = CC_INVALID_DATA_FIELD;
			return;
		}

		memcpy(&i2c_msg.data[0], &msg->data[3], i2c_msg.tx_len);
		msg->data_len = i2c_msg.rx_len;

		if (i2c_msg.rx_len == 0) {
			if (!i2c_master_write(&i2c_msg, retry)) {
				msg->completion_code = CC_SUCCESS;
			} else {
				msg->completion_code = CC_I2C_BUS_ERROR;
			}
		} else {
			if (!i2c_master_read(&i2c_msg, retry)) {
				memcpy(&msg->data[0], &i2c_msg.data, i2c_msg.rx_len);
				msg->completion_code = CC_SUCCESS;
			} else {
				msg->completion_code = CC_I2C_BUS_ERROR;
			}
		}
	} else if ((bus_7bit == 13) || (bus_7bit == 14)) {
		I3C_MSG i3c_msg;
		i3c_msg.bus = 3;
		// 8 bit address to 7 bit
		i3c_msg.target_addr = msg->data[1] >> 1;
		i3c_msg.rx_len = msg->data[2];
		i3c_msg.tx_len = msg->data_len - 3;
		if (i3c_msg.tx_len == 0) {
			msg->completion_code = CC_INVALID_DATA_FIELD;
			return;
		}

		I2C_MSG i2c_msg;
		i2c_msg.bus = I2C_BUS1;
		i2c_msg.target_addr = 0x21;
		i2c_msg.rx_len = 0;
		i2c_msg.tx_len = 2;
		i2c_msg.data[0] = 0xb;
		if (bus_7bit == 13) {
			i2c_msg.data[1] = 0x01;
		} else if (bus_7bit == 14) {
			i2c_msg.data[1] = 0x03;
		}
		if (i2c_master_write(&i2c_msg, retry)) {
			printf("[%s] switch mux failed.\n", __func__);
			msg->completion_code = CC_UNSPECIFIED_ERROR;
		}

		memcpy(&i3c_msg.data[0], &msg->data[3], i3c_msg.tx_len);
		msg->data_len = i3c_msg.rx_len;
		util_i3c_bus_init(3);
		if (i3c_msg.rx_len == 0) {
			if (!i3c_master_write(&i3c_msg, retry)) {
				msg->completion_code = CC_SUCCESS;
			} else {
				printf("[%s] i3c master write failed.\n", __func__);
				msg->completion_code = CC_I2C_BUS_ERROR;
			}
		} else {
			if (!i3c_master_read(&i3c_msg, retry)) {
				memcpy(&msg->data[0], &i3c_msg.data, i3c_msg.rx_len);
				msg->completion_code = CC_SUCCESS;
			} else {
				printf("[%s] i3c master read failed.\n", __func__);
				msg->completion_code = CC_I2C_BUS_ERROR;
			}
		}
		i2c_msg.bus = I2C_BUS1;
		i2c_msg.target_addr = 0x21;
		i2c_msg.rx_len = 0;
		i2c_msg.tx_len = 2;
		i2c_msg.data[0] = 0xb;
		i2c_msg.data[1] = 0x00;

		if (i2c_master_write(&i2c_msg, retry)) {
			printf("[%s] switch mux failed.\n", __func__);
			msg->completion_code = CC_UNSPECIFIED_ERROR;
		}

	} else {
		printf("Accessing invalid bus with IPMI master write read\n");
		msg->completion_code = CC_PARAM_OUT_OF_RANGE;
	}
	return;
}

bool add_sel_evt_record(addsel_msg_t *sel_msg)
{
	ipmb_error status;
	ipmi_msg *msg;
	uint8_t system_event_record = 0x02; // IPMI spec definition
	uint8_t evt_msg_version = 0x04; // IPMI spec definition
	static uint16_t record_id = 0x1;

	// According to IPMI spec, record id 0h and FFFFh is reserved for special usage
	if ((record_id == 0) || (record_id == 0xFFFF)) {
		record_id = 0x1;
	}

	msg = (ipmi_msg *)malloc(sizeof(ipmi_msg));
	if (msg == NULL) {
		printf("add_sel_evt_record malloc fail\n");
		return false;
	}
	memset(msg, 0, sizeof(ipmi_msg));

	msg->data_len = 16;
	msg->InF_source = SELF;
	msg->InF_target = BMC_IPMB;
	msg->netfn = NETFN_STORAGE_REQ;
	msg->cmd = CMD_STORAGE_ADD_SEL;

	msg->data[0] = (record_id & 0xFF); // record id byte 0, lsb
	msg->data[1] = ((record_id >> 8) & 0xFF); // record id byte 1
	msg->data[2] = system_event_record; // record type
	msg->data[3] = 0x00; // timestamp, bmc would fill up for bic
	msg->data[4] = 0x00; // timestamp, bmc would fill up for bic
	msg->data[5] = 0x00; // timestamp, bmc would fill up for bic
	msg->data[6] = 0x00; // timestamp, bmc would fill up for bic
	msg->data[7] = (SELF_I2C_ADDRESS << 1); // generator id
	msg->data[8] = 0x00; // generator id
	msg->data[9] = evt_msg_version; // event message format version
	msg->data[10] = sel_msg->sensor_type; // sensor type, TBD
	msg->data[11] = sel_msg->sensor_number; // sensor number
	msg->data[12] = sel_msg->event_type; // event dir/event type
	msg->data[13] = sel_msg->event_data1; // sensor data 1
	msg->data[14] = sel_msg->event_data2; // sensor data 2
	msg->data[15] = sel_msg->event_data3; // sensor data 3
	record_id++;

	status = ipmb_read(msg, IPMB_inf_index_map[msg->InF_target]);
	SAFE_FREE(msg);
	if (status == IPMB_ERROR_FAILURE) {
		printf("Fail to post msg to txqueue for addsel\n");
		return false;
	} else if (status == IPMB_ERROR_GET_MESSAGE_QUEUE) {
		printf("No response from bmc for addsel\n");
		return false;
	}

	return true;
}

void OEM_1S_GET_CARD_TYPE(ipmi_msg *msg)
{
	if (msg == NULL) {
		printf("[%s] Failed due to parameter *msg is NULL\n", __func__);
		return;
	}

	if (msg->data_len != 1) {
		msg->completion_code = CC_INVALID_LENGTH;
		return;
	}

	CARD_STATUS _1ou_status = get_1ou_status();
	CARD_STATUS _2ou_status = get_2ou_status();
	switch (msg->data[0]) {
	case GET_1OU_CARD_TYPE:
		msg->data_len = 2;
		msg->completion_code = CC_SUCCESS;
		msg->data[0] = GET_1OU_CARD_TYPE;
		if (_1ou_status.present) {
			msg->data[1] = _1ou_status.card_type;
		} else {
			msg->data[1] = TYPE_1OU_ABSENT;
		}
		break;
	case GET_2OU_CARD_TYPE:
		msg->data_len = 2;
		msg->completion_code = CC_SUCCESS;
		msg->data[0] = GET_2OU_CARD_TYPE;
		if (_2ou_status.present) {
			msg->data[1] = _2ou_status.card_type;
		} else {
			msg->data[1] = TYPE_2OU_ABSENT;
		}
		break;
	default:
		msg->data_len = 0;
		msg->completion_code = CC_INVALID_DATA_FIELD;
		break;
	}

	return;
}
