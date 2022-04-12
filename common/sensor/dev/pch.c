#include <stdio.h>
#include <stdlib.h>
#include "sensor.h"
#include "ipmi.h"

uint8_t pch_read(uint8_t sensor_num, int *reading)
{
	if (!reading)
		return SENSOR_UNSPECIFIED_ERROR;

	ipmb_error status;
	ipmi_msg *bridge_msg;
	bridge_msg = (ipmi_msg *)malloc(sizeof(ipmi_msg));
	if (bridge_msg == NULL) {
		printk("pch_read bridge message alloc fail\n");
		return SENSOR_UNSPECIFIED_ERROR;
	}

	/* read sensor from ME */
	bridge_msg->seq_source = 0xff;
	bridge_msg->netfn = NETFN_SENSOR_REQ;
	bridge_msg->cmd = CMD_SENSOR_GET_SENSOR_READING;
	bridge_msg->InF_source = Self_IFs;
	bridge_msg->InF_target = ME_IPMB_IFs;
	bridge_msg->data_len = 1;
	/* parameter offset is the sensor number to read from pch */
	bridge_msg->data[0] = sensor_config[SensorNum_SensorCfg_map[sensor_num]].offset;

	uint8_t pch_retry_num = 0;
	for (pch_retry_num = 0; pch_retry_num < 4; pch_retry_num++) {
		status = ipmb_read(bridge_msg, IPMB_inf_index_map[bridge_msg->InF_target]);
		if (status != ipmb_error_success) {
			printk("pch_read ipmb read fail, ret %d\n", status);
			free(bridge_msg);
			return SENSOR_FAIL_TO_ACCESS;
		}

		if (bridge_msg->completion_code == CC_SUCCESS) {
			sensor_val *sval = (sensor_val *)reading;
			memset(sval, 0, sizeof(sensor_val));
			sval->integer = bridge_msg->data[0];
			free(bridge_msg);
			return SENSOR_READ_SUCCESS;
		} else if (bridge_msg->completion_code == CC_NODE_BUSY) {
			continue;
		} else {
			free(bridge_msg);
			return SENSOR_UNSPECIFIED_ERROR;
		}
	}

	printf("pch_read retry read fail\n");
	free(bridge_msg);
	return SENSOR_UNSPECIFIED_ERROR;
}

uint8_t pch_init(uint8_t sensor_num)
{
	sensor_config[SensorNum_SensorCfg_map[sensor_num]].read = pch_read;
	return SENSOR_INIT_SUCCESS;
}
