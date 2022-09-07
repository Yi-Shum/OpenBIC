#include <stdio.h>
#include <stdlib.h>
#include "ipmi.h"
#include "plat_ipmi.h"
#include "plat_class.h"
#include "plat_i2c.h"
#include "hal_i3c_master.h"

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
	if (bus_7bit < 13) {
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
	} else if (bus_7bit == 13) {
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

		memcpy(&i3c_msg.data[0], &msg->data[3], i3c_msg.tx_len);
		msg->data_len = i3c_msg.rx_len;
		util_i3c_bus_init(3);
		k_msleep(5);
		if (i3c_msg.rx_len == 0) {
			if (!i3c_master_write(&i3c_msg, retry)) {
				msg->completion_code = CC_SUCCESS;
			} else {
				msg->completion_code = CC_I2C_BUS_ERROR;
			}
		} else {
			if (!i3c_master_read(&i3c_msg, retry)) {
				memcpy(&msg->data[0], &i3c_msg.data, i3c_msg.rx_len);
				msg->completion_code = CC_SUCCESS;
			} else {
				msg->completion_code = CC_I2C_BUS_ERROR;
			}
		}
	} else {
		printf("Accessing invalid bus with IPMI master write read\n");
		msg->completion_code = CC_PARAM_OUT_OF_RANGE;
	}
	return;
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
