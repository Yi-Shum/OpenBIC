#ifndef _PLDM_FW_UPDATE_H
#define _PLDM_FW_UPDATE_H

#include "pldm.h"
#include <stdint.h>
#include "plat_version.h"

enum desc_type {
	/* init */
	DESC_TYPE_PCI_VEND_ID = 0x0000,
	DESC_TYPE_IANA_ID = 0x0001,
	DESC_TYPE_UUID = 0x0002,
	DESC_TYPE_PNP_VEND_ID = 0x0003,
	DESC_TYPE_ACPI_VEND_ID = 0x0004,
	DESC_TYPE_IEEE_ASSIGN_COMPANY_ID = 0x0005,
	DESC_TYPE_SCSI_VEND_ID = 0x0006,

	/* additional */
	DESC_TYPE_PCI_DEV_ID = 0x0100,
	DESC_TYPE_PCI_SUB_VEND_ID = 0x0101,
	DESC_TYPE_PCI_SUB_ID = 0x0102,
	DESC_TYPE_PCI_REV_ID = 0x0103,
	DESC_TYPE_PNP_PROD_ID = 0x0104,
	DESC_TYPE_ACPI_PROD_ID = 0x0105,
	DESC_TYPE_ASCII_MODEL_NUM_L = 0x0106,
	DESC_TYPE_ASCII_MODEL_NUM_S = 0x0107,
	DESC_TYPE_SCSI_PROD_ID = 0x0108,
	DESC_TYPE_UBM_CTRL_DEV_CODE = 0x0109,
	DESC_TYPE_VEND_DEF = 0xFFFF,
};

typedef struct _descriptors {
	uint16_t desc_type;
	uint16_t desc_len;
	uint8_t *desc_data;
} __attribute__((packed)) desc_cfg_t;

#define BIC_FW_DESC_CNT 1
#define BIC_FW_DESC0_TYPE DESC_TYPE_IANA_ID

typedef enum {
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
} PLDM_FW_UPDATE_CMD;

typedef struct _req_fw_update_date {
	uint32_t offset;
	uint32_t length;
} req_fw_update_date;

enum cur_status {
	STATE_IDLE,
	STATE_LEARN_COMP,
	STATE_RDY_XFER,
	STATE_DOWNLOAD,
	STATE_VERIFY,
	STATE_APPLY,
	STATE_ACTIVATE,
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

struct _query_dev_id_resp {
	uint8_t completion_code;
	uint32_t dev_id_len;
	uint8_t desc_cnt;
	uint8_t descriptors;
} __attribute__((packed));

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

typedef struct _update_comp_req {
	uint16_t comp_class;
	uint16_t comp_identifier;
	uint8_t comp_class_idx;
	uint32_t comp_comparison_stamp;
	uint32_t comp_image_size;
	uint32_t update_option_flag;
	uint8_t comp_version_string_type;
	uint8_t comp_version_string_len;
	uint8_t comp_version_string;
} update_comp_req;

typedef struct _update_comp_resp {
	uint8_t completion_code;
	uint8_t comp_compatibility_resp;
	uint8_t comp_compatibility_resp_code;
	uint32_t update_option_flag_enabled;
	uint16_t estimate_time_before_update;
} update_comp_resp;

typedef struct _activate_firmware_resp {
	uint8_t completion_code;
	uint16_t estimate_time;
} activate_firmware_resp;

typedef struct _cancel_update_resp {
	uint8_t completion_code;
	uint8_t non_function_comp;
	uint64_t non_function_comp_bitmap;
} cancel_update_resp;

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
uint16_t pldm_fw_update_read(void *mctp_p, PLDM_FW_UPDATE_CMD cmd, uint8_t *req, uint16_t req_len,
			     uint8_t *rbuf, uint16_t rbuf_len);

#endif
