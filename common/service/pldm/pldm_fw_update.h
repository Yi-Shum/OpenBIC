#ifndef _PLDM_FW_UPDATE_H
#define _PLDM_FW_UPDATE_H

#include "pldm.h"
#include <stdint.h>

enum {
	PLDM_FW_UPDATE_CMD_CODE_QUERY_DEVICE_IDENTIFIERS = 0x01,
	PLDM_FW_UPDATE_CMD_CODE_GET_FIRMWARE_PARAMETERS = 0x02,
	PLDM_FW_UPDATE_CMD_CODE_REQUEST_UPDATE = 0x10,
	PLDM_FW_UPDATE_CMD_CODE_PASS_COMPONENT_TABLE = 0x13,
	PLDM_FW_UPDATE_CMD_CODE_UPDATE_COMPONENT = 0x14,
	PLDM_FW_UPDATE_CMD_CODE_REQUEST_FIRMWARE_DATA = 0x15,
	PLDM_FW_UPDATE_CMD_CODE_TRANSFER_COMPLETE = 0x16,
	PLDM_FW_UPDATE_CMD_CODE_VERIFY_COMPLETE = 0x17,
	PLDM_FW_UPDATE_CMD_CODE_APPLY_COMPLETE = 0x18,
	PLDM_FW_UPDATE_CMD_CODE_ACTIVE_FIRMWARE = 0x1A,
	PLDM_FW_UPDATE_CMD_CODE_GET_STATUS = 0x1B,
	PLDM_FW_UPDATE_CMD_CODE_CANCEL_UPDATE_COMPONENT = 0x1C,
	PLDM_FW_UPDATE_CMD_CODE_CANCEL_UPDATE = 0x1D,
};

uint8_t pldm_fw_update_handler_query(uint8_t code, void **ret_fn);

#endif
