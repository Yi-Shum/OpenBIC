#include "pldm.h"
#include "ipmi.h"
#include <logging/log.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <sys/printk.h>
#include <sys/slist.h>
#include <sys/util.h>
#include <zephyr.h>
#include "util_spi.h"
#include "libutil.h"

#define FW_UPDATE_BUF_SIZE 256
#define FW_UPDATE_SIZE 0xE0
#define PLDM_FW_UPDATE_STACK_SIZE 512
#define UPDATE_THREAD_DELAY_SECOND 1

LOG_MODULE_DECLARE(pldm, LOG_LEVEL_DBG);

k_tid_t fw_update_tid;
struct k_thread pldm_fw_update_thread;
K_KERNEL_STACK_MEMBER(pldm_fw_update_stack, PLDM_FW_UPDATE_STACK_SIZE);

static uint8_t cur_state = STATE_IDLE;
static uint8_t pre_state = STATE_IDLE;
static uint32_t comp_image_size;

desc_cfg_t bic_descriptor_config[] = {
	/* init desc: Should atlast have 1 */
	{ DESC_TYPE_IANA_ID, 4, (uint8_t *)IANA_ID },

	/* additional desc: Optional */
};

comp_parameters_t bic_parameters_config[] = {
	{ COMP_CLASS_FW,
	  0x0000,
	  0x00,
	  0x00000000,
	  STR_TYPE_ASCII,
	  MAX_VER_STR_LEN,
	  { 0x32, 0x30, 0x32, 0x32, 0x30, 0x36, 0x31, 0x36 }, //20220616
	  0x00000000,
	  STR_TYPE_UNKNOWN,
	  0x00,
	  { 0 },
	  0x8000,
	  0x00000000 },
};

version_str_t version_str_set_config = {
	{ 0x31, 0x00, 0x30, 0x00, 0x30 },
	{ 0 } // act: 1.0.0 pend: none
};

version_str_t version_str_config[] = {
	{ { 0x31, 0x00, 0x30, 0x00, 0x30 }, { 0 } }, // act: 1.0.0 pend: none
};

static void swaping(void *data, uint32_t num_byte)
{
	uint8_t *buff = data;
	for (int i = 0; i < (num_byte / 2); i++) {
		uint8_t tmp = *(buff + i);
		*(buff + i) = *(buff + (num_byte - 1 - i));
		*(buff + (num_byte - i - 1)) = tmp;
	}
}

static void udpate_fail_handler(void *mctp_p)
{
	uint8_t req_data[3];
	uint16_t read_len;
	uint16_t rbuf_len = 10;
	uint8_t *rbuf = malloc(sizeof(uint8_t) * 10);
	if (rbuf == NULL) {
		LOG_WRN("%s: Allocate memory failed", __func__);
		return;
	}

	switch (cur_state) {
	case STATE_DOWNLOAD:
		req_data[0] = 0x0A;
		read_len = pldm_fw_update_read(mctp_p, PLDM_FW_UPDATE_CMD_CODE_TRANSFER_COMPLETE,
					       req_data, 1, rbuf, rbuf_len);
		break;
	case STATE_VERIFY:
		req_data[0] = 0x0A;
		read_len = pldm_fw_update_read(mctp_p, PLDM_FW_UPDATE_CMD_CODE_VERIFY_COMPLETE,
					       req_data, 1, rbuf, rbuf_len);
		break;
	case STATE_APPLY:
		req_data[0] = 0x0A;
		req_data[1] = 0x00;
		req_data[2] = 0x00;
		read_len = pldm_fw_update_read(mctp_p, PLDM_FW_UPDATE_CMD_CODE_APPLY_COMPLETE,
					       req_data, 3, rbuf, rbuf_len);
		break;
	default:
		break;
	}
	SAFE_FREE(rbuf);
	return;
}

void req_fw_update_handler(void *mctp_p, void *arg1, void *arg2)
{
	int status;
	uint16_t read_len;
	uint8_t *resp_buf = malloc(sizeof(uint8_t) * FW_UPDATE_BUF_SIZE);
	if (resp_buf == NULL) {
		LOG_WRN("%s: read buffer allocate memory failed", __func__);
		goto error;
	}
	req_fw_update_date req;
	req.offset = 0;
	uint8_t update_flag = 0;

	while (req.offset < comp_image_size) {
		if (req.offset + FW_UPDATE_SIZE < comp_image_size) {
			req.length = FW_UPDATE_SIZE;
		} else {
			req.length = comp_image_size - req.offset;
			update_flag = (SECTOR_END_FLAG | NOT_RESET_FLAG);
		}
		read_len =
			pldm_fw_update_read(mctp_p, PLDM_FW_UPDATE_CMD_CODE_REQUEST_FIRMWARE_DATA,
					    (uint8_t *)&req, sizeof(req_fw_update_date), resp_buf,
					    FW_UPDATE_BUF_SIZE);
		if (read_len != req.length + 1) {
			LOG_WRN("Request firmware update fail, offset 0x%x, length 0x%x, return length 0x%x",
				req.offset, req.length, read_len);
			goto error;
		}
		status = fw_update(req.offset, req.length, &resp_buf[1], update_flag,
				   DEVSPI_FMC_CS0);

		if (status) {
			LOG_WRN("fw_update fail, offset 0x%x, length 0x%x, status %d", req.offset,
				req.length, status);
			goto error;
		}

		req.offset += req.length;
	}

	uint8_t req_buf[3] = { 0 };

	req_buf[0] = 0x00;
	read_len = pldm_fw_update_read(mctp_p, PLDM_FW_UPDATE_CMD_CODE_TRANSFER_COMPLETE, req_buf,
				       1, resp_buf, FW_UPDATE_BUF_SIZE);
	if (read_len != 1 || resp_buf[0] != PLDM_BASE_CODES_SUCCESS) {
		LOG_WRN("%s: Transfer complete fail", __func__);
		goto error;
	}
	pre_state = cur_state;
	cur_state = STATE_VERIFY;

	req_buf[0] = 0x00;
	read_len = pldm_fw_update_read(mctp_p, PLDM_FW_UPDATE_CMD_CODE_VERIFY_COMPLETE, req_buf, 1,
				       resp_buf, FW_UPDATE_BUF_SIZE);
	if (read_len != 1 || resp_buf[0] != PLDM_BASE_CODES_SUCCESS) {
		LOG_WRN("%s: Transfer complete fail", __func__);
		goto error;
	}
	pre_state = cur_state;
	cur_state = STATE_APPLY;

	req_buf[0] = 0x00;
	req_buf[1] = 0x00;
	req_buf[2] = 0x00;
	read_len = pldm_fw_update_read(mctp_p, PLDM_FW_UPDATE_CMD_CODE_APPLY_COMPLETE, req_buf, 3,
				       resp_buf, FW_UPDATE_BUF_SIZE);
	if (read_len != 1 || resp_buf[0] != PLDM_BASE_CODES_SUCCESS) {
		LOG_WRN("%s: Transfer complete fail", __func__);
		goto error;
	}
	pre_state = cur_state;
	cur_state = STATE_RDY_XFER;
	comp_image_size = 0;
	SAFE_FREE(resp_buf);
	return;

error:
	udpate_fail_handler(mctp_p);
	comp_image_size = 0;
	SAFE_FREE(resp_buf);
	return;
}

static uint8_t query_device_identifiers(void *mctp_inst, uint8_t *buf, uint16_t len, uint8_t *resp,
					uint16_t *resp_len, void *ext_params)
{
	if (!mctp_inst || !buf || !resp || !resp_len) {
		return PLDM_ERROR;
	}

	struct _query_dev_id_resp *resp_p = (struct _query_dev_id_resp *)resp;

	resp_p->completion_code = PLDM_BASE_CODES_SUCCESS;
	resp_p->desc_cnt = ARRAY_SIZE(bic_descriptor_config);
	resp_p->dev_id_len = 0;
	uint8_t *des_p = &resp_p->descriptors;

	for (int i = 0; i < resp_p->desc_cnt; i++) {
		*((uint16_t *)des_p) = bic_descriptor_config[i].desc_type;
		des_p += 2;
		*((uint16_t *)des_p) = bic_descriptor_config[i].desc_len;
		des_p += 2;
		memcpy(des_p, bic_descriptor_config[i].desc_data,
		       bic_descriptor_config[i].desc_len);
		des_p += bic_descriptor_config[i].desc_len;

		resp_p->dev_id_len += (4 + bic_descriptor_config[i].desc_len);
	}

	*resp_len = 6 + resp_p->dev_id_len;
	return PLDM_SUCCESS;
}

static uint8_t get_firmware_parameters(void *mctp_inst, uint8_t *buf, uint16_t len, uint8_t *resp,
				       uint16_t *resp_len, void *ext_params)
{
	if (!mctp_inst || !buf || !resp || !resp_len) {
		return PLDM_ERROR;
	}

	struct _get_fw_parm_resp *resp_p = (struct _get_fw_parm_resp *)resp;

	resp_p->completion_code = PLDM_BASE_CODES_SUCCESS;
	*resp_len = 1;

	uint32_t cap_during_update = 0x7; //TBD: currently same as NIC config
	swaping(&cap_during_update, 2);
	resp_p->cap_during_update = cap_during_update;
	uint16_t comp_cnt = ARRAY_SIZE(bic_parameters_config);
	swaping(&comp_cnt, 2);
	resp_p->comp_cnt = comp_cnt; // means 1 component count
	resp_p->active_comp_img_set_ver_str_type = STR_TYPE_ASCII;
	resp_p->active_comp_img_set_ver_str_len = MAX_VER_STR_LEN; // TBD: temparary set 16 bytes
	resp_p->pend_comp_img_set_ver_str_type =
		STR_TYPE_UNKNOWN; // currently not include pending component
	resp_p->pend_comp_img_set_ver_str_len = 0X0; // currently not include pending component

	*resp_len = (sizeof(struct _get_fw_parm_resp) - 1);

	if (!resp_p->comp_cnt) {
		LOG_WRN("%s: Component count shouldn't be 0", __func__);
		resp_p->completion_code = PLDM_BASE_CODES_ERROR_INVALID_DATA;
		*resp_len = 1;
		goto end;
	}

	uint8_t *cur_resp = &resp_p->variables;
	if (resp_p->active_comp_img_set_ver_str_len) {
		memcpy(cur_resp, &version_str_set_config.act_version,
		       resp_p->active_comp_img_set_ver_str_len);
		cur_resp += resp_p->active_comp_img_set_ver_str_len;
		*resp_len += resp_p->active_comp_img_set_ver_str_len;
	}
	if (resp_p->pend_comp_img_set_ver_str_len) {
		memcpy(cur_resp, &version_str_set_config.pend_version,
		       resp_p->pend_comp_img_set_ver_str_len);
		cur_resp += resp_p->pend_comp_img_set_ver_str_len;
		*resp_len += resp_p->pend_comp_img_set_ver_str_len;
	}

	/* ComponentParameterTable */
	for (int i = 0; i < comp_cnt; i++) {
		memcpy(cur_resp, &bic_parameters_config[i], sizeof(bic_parameters_config[i]));
		cur_resp += sizeof(bic_parameters_config[i]);

		*resp_len += sizeof(bic_parameters_config[i]);

		if (bic_parameters_config[i].active_comp_ver_str_len) {
			memcpy(cur_resp, &version_str_config[i].act_version,
			       bic_parameters_config[i].active_comp_ver_str_len);
			cur_resp += bic_parameters_config[i].active_comp_ver_str_len;
			*resp_len += bic_parameters_config[i].active_comp_ver_str_len;
		}
		if (bic_parameters_config[i].pend_comp_ver_str_len) {
			memcpy(cur_resp, &version_str_config[i].pend_version,
			       bic_parameters_config[i].pend_comp_ver_str_len);
			cur_resp += bic_parameters_config[i].pend_comp_ver_str_len;
			*resp_len += bic_parameters_config[i].pend_comp_ver_str_len;
		}
	}

end:
	return PLDM_SUCCESS;
}

static uint8_t request_update(void *mctp_inst, uint8_t *buf, uint16_t len, uint8_t *resp,
			      uint16_t *resp_len, void *ext_params)
{
	if (!mctp_inst || !buf || !resp || !resp_len) {
		return PLDM_ERROR;
	}

	struct _req_update_req *req_p = (struct _req_update_req *)buf;
	struct _req_update_resp *resp_p = (struct _req_update_resp *)resp;

	resp_p->completion_code = PLDM_BASE_CODES_SUCCESS;

	/* TBD: currently not support meta data */
	resp_p->fw_dev_meta_data_len = 0x0000;

	if (req_p->pkg_data_len) {
		resp_p->fd_will_send_get_pkg_data_cmd = 0x01;
	} else {
		resp_p->fd_will_send_get_pkg_data_cmd = 0x00;
	}

	*resp_len = sizeof(struct _req_update_resp);
	pre_state = cur_state;
	cur_state = STATE_LEARN_COMP;
	return PLDM_SUCCESS;
}

static uint8_t pass_component_table(void *mctp_inst, uint8_t *buf, uint16_t len, uint8_t *resp,
				    uint16_t *resp_len, void *ext_params)
{
	if (!mctp_inst || !buf || !resp || !resp_len) {
		return PLDM_ERROR;
	}

	struct _pass_comp_table_req *req_p = (struct _pass_comp_table_req *)buf;
	struct _pass_comp_table_resp *resp_p = (struct _pass_comp_table_resp *)resp;

	resp_p->completion_code = PLDM_BASE_CODES_SUCCESS;

	/* TBD: Transfer flag may need to be varified here */

	/* TBD: Component classification may need to be varified here */

	if (req_p->comp_class == 0xFFFF) {
		if (req_p->comp_id > 0xFFF) {
			LOG_WRN("%s: Invalid component id for downstream device.", __func__);
			resp_p->completion_code = PLDM_BASE_CODES_ERROR_INVALID_DATA;
			goto end;
		}
	}

	/* TBD: Component id may need to be varified here */

	if (req_p->comp_class == 0xFFFF) {
		if (req_p->comp_class_idx == 0xFF) {
			/* TBD: Update 1 downstream device */
			;
		} else if (req_p->comp_class_idx == 0x00) {
			/* TBD: Update all downstream device */
			;
		} else {
			LOG_WRN("%s: Invalid component classification index for downstream device.",
				__func__);
			resp_p->completion_code = PLDM_BASE_CODES_ERROR_INVALID_DATA;
			goto end;
		}
	}

	/* TBD: Should be determined in future */
	resp_p->comp_resp_code = 0x00;

	if (!resp_p->comp_resp_code) {
		resp_p->comp_resp = 0x00;
	} else {
		resp_p->comp_resp = 0x01;
	}

	*resp_len = sizeof(struct _pass_comp_table_resp);
	pre_state = cur_state;
	cur_state = STATE_RDY_XFER;
end:
	return PLDM_SUCCESS;
}

static uint8_t update_component(void *mctp_inst, uint8_t *buf, uint16_t len, uint8_t *resp,
				uint16_t *resp_len, void *ext_params)
{
	if (!mctp_inst || !buf || !resp || !resp_len) {
		return PLDM_ERROR;
	}

	if (fw_update_tid != NULL && strcmp(k_thread_state_str(fw_update_tid), "dead") != 0) {
		LOG_WRN("%s: PLDM fw update thread is running!", __func__);
		return PLDM_ERROR;
	}

	update_comp_req *req_p = (update_comp_req *)buf;
	update_comp_resp *resp_p = (update_comp_resp *)resp;
	if (len < sizeof(update_comp_req)) {
		resp_p->completion_code = PLDM_BASE_CODES_ERROR_INVALID_LENGTH;
		return PLDM_SUCCESS;
	}

	/* TODO: check request data */
	comp_image_size = req_p->comp_image_size;

	resp_p->completion_code = PLDM_BASE_CODES_SUCCESS;
	resp_p->comp_compatibility_resp = 0x00;
	resp_p->comp_compatibility_resp_code = 0x00;
	resp_p->update_option_flag_enabled = req_p->update_option_flag;
	resp_p->estimate_time_before_update = UPDATE_THREAD_DELAY_SECOND;
	*resp_len = sizeof(update_comp_resp);

	fw_update_tid =
		k_thread_create(&pldm_fw_update_thread, pldm_fw_update_stack,
				K_THREAD_STACK_SIZEOF(pldm_fw_update_stack), req_fw_update_handler,
				mctp_inst, NULL, NULL, CONFIG_MAIN_THREAD_PRIORITY, 0,
				K_SECONDS(UPDATE_THREAD_DELAY_SECOND));
	k_thread_name_set(&pldm_fw_update_thread, "pldm_fw_update_thread");

	pre_state = cur_state;
	cur_state = STATE_DOWNLOAD;
	return PLDM_SUCCESS;
}

static uint8_t activate_firmware(void *mctp_inst, uint8_t *buf, uint16_t len, uint8_t *resp,
				 uint16_t *resp_len, void *ext_params)
{
	if (!mctp_inst || !buf || !resp || !resp_len) {
		return PLDM_ERROR;
	}

	activate_firmware_resp *resp_p = (activate_firmware_resp *)resp;
	resp_p->completion_code = PLDM_BASE_CODES_SUCCESS;
	resp_p->estimate_time = 0;
	*resp_len = sizeof(activate_firmware_resp);

	pre_state = cur_state;
	cur_state = STATE_ACTIVATE;
	return PLDM_SUCCESS;
}

static uint8_t get_status(void *mctp_inst, uint8_t *buf, uint16_t len, uint8_t *resp,
			  uint16_t *resp_len, void *ext_params)
{
	if (!mctp_inst || !buf || !resp || !resp_len) {
		return PLDM_ERROR;
	}

	struct _get_status_resp *resp_p = (struct _get_status_resp *)resp;
	*resp_len = 1;

	resp_p->pre_state = pre_state;
	resp_p->cur_state = cur_state;
	resp_p->aux_state = 1; //TBD: not support now
	resp_p->aux_state_status = 0x00; //TBD: not support now
	resp_p->prog_percent = 0x00; //TBD: not support now
	resp_p->reason_code = 0x00; //TBD: not support now
	resp_p->update_op_flags_en = 0x01; //force update enable

	*resp_len = sizeof(struct _get_status_resp);
	return PLDM_SUCCESS;
}

static uint8_t cancel_update_component(void *mctp_inst, uint8_t *buf, uint16_t len, uint8_t *resp,
				       uint16_t *resp_len, void *ext_params)
{
	if (!mctp_inst || !buf || !resp || !resp_len) {
		return PLDM_ERROR;
	}

	if (cur_state != STATE_DOWNLOAD) {
		return PLDM_ERROR;
	}

	LOG_WRN("%s: Cancel update component!", __func__);
	*resp = PLDM_BASE_CODES_SUCCESS;
	*resp_len = 1;

	pre_state = cur_state;
	cur_state = STATE_RDY_XFER;

	return PLDM_SUCCESS;
}

static uint8_t canel_update(void *mctp_inst, uint8_t *buf, uint16_t len, uint8_t *resp,
			    uint16_t *resp_len, void *ext_params)
{
	if (!mctp_inst || !buf || !resp || !resp_len) {
		return PLDM_ERROR;
	}

	if (fw_update_tid != NULL && strcmp(k_thread_state_str(fw_update_tid), "dead") != 0) {
		k_thread_abort(fw_update_tid);
	}

	LOG_WRN("Cancel update, current state: %d", cur_state);
	cancel_update_resp *resp_p = (cancel_update_resp *)resp;

	resp_p->completion_code = PLDM_BASE_CODES_SUCCESS;
	resp_p->non_function_comp = 0;
	resp_p->non_function_comp_bitmap = 0;

	*resp_len = sizeof(cancel_update_resp);

	pre_state = cur_state;
	cur_state = STATE_IDLE;

	return PLDM_SUCCESS;
}

uint16_t pldm_fw_update_read(void *mctp_p, PLDM_FW_UPDATE_CMD cmd, uint8_t *req, uint16_t req_len,
			     uint8_t *rbuf, uint16_t rbuf_len)
{
	pldm_msg msg;
	memset(&msg, 0, sizeof(msg));

	msg.hdr.pldm_type = PLDM_TYPE_FW_UPDATE;
	msg.hdr.cmd = cmd;
	msg.hdr.rq = 1;

	msg.buf = req;
	msg.len = req_len;

	return mctp_pldm_read(mctp_p, &msg, rbuf, rbuf_len);
}

static pldm_cmd_handler pldm_fw_update_cmd_tbl[] = {
	{ PLDM_FW_UPDATE_CMD_CODE_QUERY_DEVICE_IDENTIFIERS, query_device_identifiers },
	{ PLDM_FW_UPDATE_CMD_CODE_GET_FIRMWARE_PARAMETERS, get_firmware_parameters },
	{ PLDM_FW_UPDATE_CMD_CODE_REQUEST_UPDATE, request_update },
	{ PLDM_FW_UPDATE_CMD_CODE_PASS_COMPONENT_TABLE, pass_component_table },
	{ PLDM_FW_UPDATE_CMD_CODE_UPDATE_COMPONENT, update_component },
	{ PLDM_FW_UPDATE_CMD_CODE_ACTIVE_FIRMWARE, activate_firmware },
	{ PLDM_FW_UPDATE_CMD_CODE_GET_STATUS, get_status },
	{ PLDM_FW_UPDATE_CMD_CODE_CANCEL_UPDATE_COMPONENT, cancel_update_component },
	{ PLDM_FW_UPDATE_CMD_CODE_CANCEL_UPDATE, canel_update },
};

uint8_t pldm_fw_update_handler_query(uint8_t code, void **ret_fn)
{
	if (!ret_fn) {
		return PLDM_ERROR;
	}

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
