#include "plat_ipmi.h"
#include "ipmb.h"
#include "oem_1s_handler.h"
#include "util_spi.h"

#define BIOS_UPDATE_MAX_OFFSET 0x4000000
#define BIC_UPDATE_MAX_OFFSET 0x50000
#define CPLD_SPI_OOB_FROM_CPU 0x02
#define CPLD_SPI_OOB_FROM_BIC 0x0B

void OEM_1S_GET_FW_VERSION(ipmi_msg *msg)
{
	if (msg == NULL) {
		return;
	}

	if (msg->data_len != 1) {
		msg->completion_code = CC_INVALID_LENGTH;
		return;
	}

	uint8_t component;
	component = msg->data[0];

	switch (component) {
	case COMPNT_CPLD:
		msg->completion_code = CC_UNSPECIFIED_ERROR;
		break;
	case COMPNT_BIC:
		msg->data[0] = BIC_FW_YEAR_MSB;
		msg->data[1] = BIC_FW_YEAR_LSB;
		msg->data[2] = BIC_FW_WEEK;
		msg->data[3] = BIC_FW_VER;
		msg->data[4] = BIC_FW_platform_0;
		msg->data[5] = BIC_FW_platform_1;
		msg->data[6] = BIC_FW_platform_2;
		msg->data_len = 7;
		msg->completion_code = CC_SUCCESS;
		break;
	default:
		msg->completion_code = CC_UNSPECIFIED_ERROR;
		break;
	}
	return;
}

void OEM_1S_FW_UPDATE(ipmi_msg *msg)
{
	if (msg == NULL) {
		return;
	}

	/*********************************
	* Request Data
	*
	* Byte   0: [6:0] fw update target, [7] indicate last packet
	* Byte 1-4: offset, lsb first
	* Byte 5-6: length, lsb first
	* Byte 7-N: data
	***********************************/
	if (msg->data_len < 8) {
		msg->completion_code = CC_INVALID_LENGTH;
		return;
	}

	uint8_t target = msg->data[0];
	uint8_t status = -1;
	uint32_t offset =
		((msg->data[4] << 24) | (msg->data[3] << 16) | (msg->data[2] << 8) | msg->data[1]);
	uint16_t length = ((msg->data[6] << 8) | msg->data[5]);

	if ((length == 0) || (length != msg->data_len - 7)) {
		msg->completion_code = CC_INVALID_LENGTH;
		return;
	}

	if (target == BIOS_UPDATE) {
		// BIOS size maximum 64M bytes
		if (offset > BIOS_UPDATE_MAX_OFFSET) {
			msg->completion_code = CC_PARAM_OUT_OF_RANGE;
			return;
		}
		int pos = pal_get_bios_flash_position();
		if (pos == -1) {
			msg->completion_code = CC_INVALID_PARAM;
			return;
		}

		// Switch GPIO(BIOS SPI Selection Pin) to BIC
		bool ret = pal_switch_bios_spi_mux(CPLD_SPI_OOB_FROM_BIC);
		if (!ret) {
			msg->completion_code = CC_UNSPECIFIED_ERROR;
			return;
		}

		status = fw_update(offset, length, &msg->data[7], (target & IS_SECTOR_END_MASK),
				   pos);

		// Switch GPIO(BIOS SPI Selection Pin) to PCH
		ret = pal_switch_bios_spi_mux(CPLD_SPI_OOB_FROM_CPU);
		if (!ret) {
			msg->completion_code = CC_UNSPECIFIED_ERROR;
			return;
		}

	} else if ((target == BIC_UPDATE) || (target == (BIC_UPDATE | IS_SECTOR_END_MASK))) {
		// Expect BIC firmware size not bigger than 320k
		if (offset > BIC_UPDATE_MAX_OFFSET) {
			msg->completion_code = CC_PARAM_OUT_OF_RANGE;
			return;
		}
		status = fw_update(offset, length, &msg->data[7], (target & IS_SECTOR_END_MASK),
				   DEVSPI_FMC_CS0);
	} else {
		msg->completion_code = CC_INVALID_DATA_FIELD;
		return;
	}

	msg->data_len = 0;

	switch (status) {
	case FWUPDATE_SUCCESS:
		msg->completion_code = CC_SUCCESS;
		break;
	case FWUPDATE_OUT_OF_HEAP:
		msg->completion_code = CC_LENGTH_EXCEEDED;
		break;
	case FWUPDATE_OVER_LENGTH:
		msg->completion_code = CC_OUT_OF_SPACE;
		break;
	case FWUPDATE_REPEATED_UPDATED:
		msg->completion_code = CC_INVALID_DATA_FIELD;
		break;
	case FWUPDATE_UPDATE_FAIL:
		msg->completion_code = CC_TIMEOUT;
		break;
	case FWUPDATE_ERROR_OFFSET:
		msg->completion_code = CC_PARAM_OUT_OF_RANGE;
		break;
	default:
		msg->completion_code = CC_UNSPECIFIED_ERROR;
		break;
	}
	if (status != FWUPDATE_SUCCESS) {
		printf("firmware (0x%02X) update failed cc: %x\n", target, msg->completion_code);
	}

	return;
}