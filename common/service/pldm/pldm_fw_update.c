#include "pldm.h"
#include "ipmi.h"
#include <logging/log.h>
#include <string.h>
#include <sys/printk.h>
#include <sys/slist.h>
#include <sys/util.h>
#include <zephyr.h>

LOG_MODULE_DECLARE(pldm, LOG_LEVEL_DBG);

static uint8_t cur_state = STAT_IDLE;

static uint8_t get_cur_status()
{
	return cur_state;
}

static uint8_t query_device_identifiers(void *mctp_inst, uint8_t *buf, uint16_t len, uint8_t *resp,
					uint16_t *resp_len, void *ext_params)
{
	if (!mctp_inst || !buf || !resp || !resp_len)
		return PLDM_ERROR;

	return PLDM_SUCCESS;
}

static uint8_t get_firmware_parameters(void *mctp_inst, uint8_t *buf, uint16_t len, uint8_t *resp,
				       uint16_t *resp_len, void *ext_params)
{
	if (!mctp_inst || !buf || !resp || !resp_len)
		return PLDM_ERROR;

	return PLDM_SUCCESS;
}

static uint8_t request_update(void *mctp_inst, uint8_t *buf, uint16_t len, uint8_t *resp,
			      uint16_t *resp_len, void *ext_params)
{
	if (!mctp_inst || !buf || !resp || !resp_len)
		return PLDM_ERROR;

	struct _req_update_req *req_p = (struct _req_update_req *)buf;
	struct _req_update_resp *resp_p = (struct _req_update_resp *)resp;

	resp_p->completion_code = PLDM_BASE_CODES_SUCCESS;
	*resp_len = 1;

	/* TBD: currently not support meta data */
	resp_p->fw_dev_meta_data_len = 0x0000;

	if (req_p->pkg_data_len)
		resp_p->fd_will_send_get_pkg_data_cmd = 0x01;
	else
		resp_p->fd_will_send_get_pkg_data_cmd = 0x00;

	*resp_len = sizeof(struct _req_update_resp);
	return PLDM_SUCCESS;
}

static uint8_t pass_component_table(void *mctp_inst, uint8_t *buf, uint16_t len, uint8_t *resp,
				    uint16_t *resp_len, void *ext_params)
{
	if (!mctp_inst || !buf || !resp || !resp_len)
		return PLDM_ERROR;

	struct _pass_comp_table_req *req_p = (struct _pass_comp_table_req *)buf;
	struct _pass_comp_table_resp *resp_p = (struct _pass_comp_table_resp *)resp;

	resp_p->completion_code = PLDM_BASE_CODES_SUCCESS;
	*resp_len = 1;

	/* TBD: Transfer flag may need to be varified here */

	/* TBD: Component classification may need to be varified here */

	if (req_p->comp_class == 0xFFFF) {
		if (req_p->comp_id > 0xFFF) {
			printf("%s: Invalid component id for downstream device.\n", __func__);
			resp_p->completion_code = PLDM_BASE_CODES_ERROR_INVALID_DATA;
			goto end;
		}
	}

	/* TBD: Component id may need to be varified here */

	if (req_p->comp_class == 0xFFFF) {
		if (req_p->comp_class_idx == 0xFF)
			/* TBD: Update 1 downstream device */
			;
		else if (req_p->comp_class_idx == 0x00)
			/* TBD: Update all downstream device */
			;
		else {
			printf("%s: Invalid component classification index for downstream device.\n",
			       __func__);
			resp_p->completion_code = PLDM_BASE_CODES_ERROR_INVALID_DATA;
			goto end;
		}
	}

	/* TBD: Should be determined in future */
	resp_p->comp_resp_code = 0x00;

	if (!resp_p->comp_resp_code)
		resp_p->comp_resp = 0x00;
	else
		resp_p->comp_resp = 0x01;

	*resp_len = sizeof(struct _pass_comp_table_resp);

end:
	return PLDM_SUCCESS;
}

static uint8_t update_component(void *mctp_inst, uint8_t *buf, uint16_t len, uint8_t *resp,
				uint16_t *resp_len, void *ext_params)
{
	if (!mctp_inst || !buf || !resp || !resp_len)
		return PLDM_ERROR;

	return PLDM_SUCCESS;
}

static uint8_t activate_firmware(void *mctp_inst, uint8_t *buf, uint16_t len, uint8_t *resp,
				 uint16_t *resp_len, void *ext_params)
{
	if (!mctp_inst || !buf || !resp || !resp_len)
		return PLDM_ERROR;

	return PLDM_SUCCESS;
}

static uint8_t get_status(void *mctp_inst, uint8_t *buf, uint16_t len, uint8_t *resp,
			  uint16_t *resp_len, void *ext_params)
{
	if (!mctp_inst || !buf || !resp || !resp_len)
		return PLDM_ERROR;

	struct _get_status_resp *resp_p = (struct _get_status_resp *)resp;
	*resp_len = 1;

	static uint8_t pre_state = STAT_IDLE;
	resp_p->pre_state = pre_state;
	resp_p->cur_state = get_cur_status();
	resp_p->aux_state = 1; //TBD: not support now
	resp_p->aux_state_status = 0x00; //TBD: not support now
	resp_p->prog_percent = 0x00; //TBD: not support now
	resp_p->reason_code = 0x00; //TBD: not support now
	resp_p->update_op_flags_en = 0x01; //force update enable

	pre_state = resp_p->cur_state;

	*resp_len = sizeof(struct _get_status_resp);
	return PLDM_SUCCESS;
}

static pldm_cmd_handler pldm_fw_update_cmd_tbl[] = {
	{ PLDM_FW_UPDATE_CMD_CODE_QUERY_DEVICE_IDENTIFIERS, query_device_identifiers },
	{ PLDM_FW_UPDATE_CMD_CODE_GET_FIRMWARE_PARAMETERS, get_firmware_parameters },
	{ PLDM_FW_UPDATE_CMD_CODE_REQUEST_UPDATE, request_update },
	{ PLDM_FW_UPDATE_CMD_CODE_PASS_COMPONENT_TABLE, pass_component_table },
	{ PLDM_FW_UPDATE_CMD_CODE_UPDATE_COMPONENT, update_component },
	{ PLDM_FW_UPDATE_CMD_CODE_ACTIVE_FIRMWARE, activate_firmware },
	{ PLDM_FW_UPDATE_CMD_CODE_GET_STATUS, get_status },
};

uint8_t pldm_fw_update_handler_query(uint8_t code, void **ret_fn)
{
	if (!ret_fn)
		return PLDM_ERROR;

	pldm_cmd_proc_fn fn = NULL;
	uint8_t i;

	for (i = 0; i < ARRAY_SIZE(pldm_fw_update_cmd_tbl); i++) {
		if (pldm_fw_update_cmd_tbl[i].cmd_code == code) {
			fn = pldm_fw_update_cmd_tbl[i].fn;
			break;
		}
	}

	*ret_fn = (void *)fn;
	return fn ? PLDM_SUCCESS : PLDM_ERROR;
}
