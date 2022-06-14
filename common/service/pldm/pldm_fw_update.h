#ifndef _PLDM_FW_UPDATE_H
#define _PLDM_FW_UPDATE_H

#include "pldm.h"
#include <stdint.h>

enum {
	/* inventory commands */
	PLDM_FW_UPDATE_CMD_CODE_QUERY_DEVICE_IDENTIFIERS = 0x01,
	PLDM_FW_UPDATE_CMD_CODE_GET_FIRMWARE_PARAMETERS = 0x02,

	/* update commands */
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

enum cur_status {
	STAT_IDLE,
	STAT_LEARN_COMP,
	STAT_RDY_XFER,
	STAT_DOWNLOAD,
	STAT_VERIFY,
	STAT_APPLY,
	STAT_ACTIVATE,
};

enum comp_class {
	COMP_CLASS_UNKNOWN = 0x0000,
	COMP_CLASS_OTHER,
	COMP_CLASS_DRIVER,
	COMP_CLASS_CONFIG_SW,
	COMP_CLASS_APP_SW,
	COMP_CLASS_INST,
	COMP_CLASS_FW_BIOS,
	COMP_CLASS_DIA_SW,
	COMP_CLASS_OPE_SYS,
	COMP_CLASS_MID,
	COMP_CLASS_FW,
	COMP_CLASS_BIOS_FCODE,
	COMP_CLASS_SUP_SERV_PACK,
	COMP_CLASS_SW_BUND,

	/* 0x8000 ~ 0xfffe reserved for vendor */

	COMP_CLASS_DS_DEV = 0xFFFF,
};

enum str_type {
	STR_TYPE_UNKNOWN,
	STR_TYPE_ASCII,
	STR_TYPE_UTF8,
	STR_TYPE_UTF16_LE,
	STR_TYPE_UTF16_BE,
};

enum trans_flag {
	TRANS_START = 0x1,
	TRANS_MIDDLE = 0x2,
	TRANS_END = 0x4,
	TRANS_START_END = 0x5,
};

enum comp_resp_code {
	cc_00 = 0x00,
	cc_01,
	cc_02,
	cc_03,
	cc_04,
	cc_05,
	cc_06,
	cc_07,
	cc_08,
	cc_09,
	cc_0A,
	cc_0B,
};

struct _req_update_req {
	uint32_t max_trans_size;
	uint16_t num_of_comp;
	uint8_t max_ostd_trans_req;
	uint16_t pkg_data_len;
	uint8_t comp_img_set_ver_str_type;
	uint8_t comp_img_set_ver_str_len;
	char comp_img_set_ver_str[255];
} __attribute__((packed));

struct _req_update_resp {
	uint8_t completion_code;
	uint16_t fw_dev_meta_data_len;
	uint8_t fd_will_send_get_pkg_data_cmd;
} __attribute__((packed));

struct _pass_comp_table_req {
	uint8_t trans_flag;
	uint16_t comp_class;
	uint16_t comp_id;
	uint8_t comp_class_idx;
	uint32_t comp_comparison_stamp;
	uint8_t comp_ver_str_type;
	uint8_t comp_ver_str_len;
	char comp_ver_str[255];
} __attribute__((packed));

struct _pass_comp_table_resp {
	uint8_t completion_code;
	uint8_t comp_resp;
	uint8_t comp_resp_code;
} __attribute__((packed));

struct _get_status_resp {
	uint8_t completion_code;
	uint8_t cur_state;
	uint8_t pre_state;
	uint8_t aux_state;
	uint8_t aux_state_status;
	uint8_t prog_percent;
	uint8_t reason_code;
	uint32_t update_op_flags_en;
} __attribute__((packed));

uint8_t pldm_fw_update_handler_query(uint8_t code, void **ret_fn);

#endif
