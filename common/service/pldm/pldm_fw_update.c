#include "pldm.h"
#include "ipmi.h"
#include <logging/log.h>
#include <string.h>
#include <sys/printk.h>
#include <sys/slist.h>
#include <sys/util.h>
#include <zephyr.h>

LOG_MODULE_DECLARE(pldm, LOG_LEVEL_DBG);

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

	resp_p->fw_dev_meta_data_len = 0x0000; // currently not support meta data

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

static pldm_cmd_handler pldm_fw_update_cmd_tbl[] = {
	{ PLDM_FW_UPDATE_CMD_CODE_QUERY_DEVICE_IDENTIFIERS, query_device_identifiers },
	{ PLDM_FW_UPDATE_CMD_CODE_GET_FIRMWARE_PARAMETERS, get_firmware_parameters },
	{ PLDM_FW_UPDATE_CMD_CODE_REQUEST_UPDATE, request_update },
	{ PLDM_FW_UPDATE_CMD_CODE_PASS_COMPONENT_TABLE, pass_component_table },
	{ PLDM_FW_UPDATE_CMD_CODE_UPDATE_COMPONENT, update_component },
	{ PLDM_FW_UPDATE_CMD_CODE_ACTIVE_FIRMWARE, activate_firmware },
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
