#include <zephyr.h>
#include <stdio.h>
#include <stdint.h>
#include <device.h>
#include "hal_i3c_master.h"

#define I3C_MSGQ_LEN 10

static const struct device *dev_i3c[I3C_BUS_MAX_NUM];
static struct k_mutex i3c_mutex[I3C_BUS_MAX_NUM];
// I3C_TARGET_CFG i3c_target_cfg[I3C_BUS_MAX_NUM * I3C_TARGET_DEV_MAX_NUM];
// static uint8_t i3c_target_count[I3C_BUS_MAX_NUM];
// static struct i3c_dev_desc i3c_target_desc[I3C_BUS_MAX_NUM][I3C_TARGET_DEV_MAX_NUM];

// static struct i3c_bus_data i3c_bus_info[I3C_BUS_NUM];
// static struct i3c_ibi_payload i3c_payload[I3C_BUS_NUM];
// char __aligned(4) i3c_msgq_buffer[I3C_BUS_NUM][I3C_MSGQ_LEN * sizeof(struct i3c_msg_package)];

// static struct i3c_ibi_payload *ibi_write_requested(struct i3c_dev_desc *desc)
// {
// 	struct i3c_bus_data *current_bus_data;

// 	current_bus_data = CONTAINER_OF(desc, struct i3c_bus_data, i3c_slave);

// 	i3c_payload[current_bus_data->bus_num].buf = current_bus_data->current_msg.msg;
// 	i3c_payload[current_bus_data->bus_num].size = 0;
// 	return &i3c_payload[current_bus_data->bus_num];
// }

// static void ibi_write_done(struct i3c_dev_desc *desc)
// {
// 	struct i3c_bus_data *current_bus_data;

// 	current_bus_data = CONTAINER_OF(desc, struct i3c_bus_data, i3c_slave);

// 	if (IS_MDB_PENDING_READ_NOTIFY(current_bus_data->current_msg.msg[0])) {
// 		/* todo: read the pending data of ibi */
// 	} else {
// 		current_bus_data->current_msg.msg_length =
// 			i3c_payload[current_bus_data->bus_num].size;
// 		/* todo: put assigned address into 1st byte & remove original 1st byte(MDB)?? */
// 		k_msgq_put(&current_bus_data->slave_msgq, &current_bus_data->current_msg,
// 			   K_NO_WAIT);
// 	}
// }

// static struct i3c_ibi_callbacks i3c_ibi_def_callbacks = {
// 	.write_requested = ibi_write_requested,
// 	.write_done = ibi_write_done,
// };

// static struct i3c_dev_desc *find_i3c_target_dev(uint8_t bus, uint8_t addr)
// {
// 	for (uint8_t i = 0; i < i3c_target_count[bus]; i++) {
// 		if (i3c_target_desc[bus][i].info.dynamic_addr == addr) {
// 			return &i3c_target_desc[bus][i];
// 		}
// 	}
// 	return NULL;
// }

int i3c_master_read(I3C_MSG *msg, uint8_t retry)
{
	if (!msg) {
		return -1;
	}
	if (!dev_i3c[msg->bus]) {
		printf("[%s] I3C bus %d not found\n", __func__, msg->bus);
		return -1;
	}

	static struct i3c_dev_desc slave;
	struct i3c_priv_xfer xfer[2];
	int ret;

	slave.info.static_addr = msg->target_addr;
	slave.info.assigned_dynamic_addr = msg->target_addr;
	slave.info.i2c_mode = 0;

	ret = i3c_master_attach_device(dev_i3c[msg->bus], &slave);
	if (ret) {
		printf("[%s] Failed to attach slave\n", __func__);
		return -1;
	}

	for (int i = 0; i < retry; i++) {
		xfer[0].rnw = 0;
		xfer[0].len = msg->tx_len;
		xfer[0].data.in = msg->data;

		xfer[1].rnw = 1;
		xfer[1].len = msg->rx_len;
		xfer[1].data.out = msg->data;

		ret = i3c_master_priv_xfer(&slave, xfer, 2);
		if (ret) {
			continue;
		}
		ret = i3c_master_detach_device(dev_i3c[msg->bus], &slave);
		if (ret) {
			printf("[%s] failed to detach slave\n", __func__);
			return -1;
		}
		return 0;
	}
	printf("[%s] retry reach max\n", __func__);
	ret = i3c_master_detach_device(dev_i3c[msg->bus], &slave);
	if (ret) {
		printf("[%s] failed to detach slave\n", __func__);
	}
	return -1;
}

int i3c_master_write(I3C_MSG *msg, uint8_t retry)
{
	if (!msg) {
		return -1;
	}
	if (!dev_i3c[msg->bus]) {
		printf("[%s] I3C bus %d not found\n", __func__, msg->bus);
		return -1;
	}

	static struct i3c_dev_desc slave;
	struct i3c_priv_xfer xfer;
	int ret;

	slave.info.static_addr = msg->target_addr;
	slave.info.assigned_dynamic_addr = msg->target_addr;
	slave.info.i2c_mode = 0;

	ret = i3c_master_attach_device(dev_i3c[msg->bus], &slave);
	if (ret) {
		printf("[%s] Failed to attach slave\n", __func__);
		return -1;
	}

	for (int i = 0; i < retry; i++) {
		xfer.rnw = 0;
		xfer.len = msg->tx_len;
		xfer.data.in = msg->data;

		ret = i3c_master_priv_xfer(&slave, &xfer, 1);
		if (ret) {
			continue;
		}
		ret = i3c_master_detach_device(dev_i3c[msg->bus], &slave);
		if (ret) {
			printf("[%s] failed to detach slave\n", __func__);
			return -1;
		}
		return 0;
	}
	printf("[%s] retry reach max\n", __func__);
	ret = i3c_master_detach_device(dev_i3c[msg->bus], &slave);
	if (ret) {
		printf("[%s] failed to detach slave\n", __func__);
	}
	return -1;
}

bool util_i3c_bus_init(uint8_t bus)
{
	if (!dev_i3c[bus]) {
		printf("[%s] I3C bus %d not found\n", __func__, bus);
		return false;
	}

	if (k_mutex_lock(&i3c_mutex[bus], K_MSEC(1000))) {
		printf("[%s] I3C %d get mutex timeout\n", __func__, bus);
		return false;
	}

	int ret;
	ret = i3c_master_send_rstdaa(dev_i3c[bus]);
	ret = i3c_master_send_rstdaa(dev_i3c[bus]);
	if (ret) {
		printf("[%s] RSTDAA failed, ret %d\n", __func__, ret);
		goto error;
	}

	ret = i3c_master_send_aasa(dev_i3c[bus]);
	if (ret) {
		printf("[%s] SETAASA failed, ret %d\n", __func__, ret);
		goto error;
	}

	// int ret;
	// for (int i = 0; i < I3C_BUS_MAX_NUM * I3C_TARGET_DEV_MAX_NUM; i++) {
	// 	I3C_TARGET_CFG *cfg = &i3c_target_cfg[i];
	// 	if (cfg->bus == bus & cfg->address != 0) {
	// 		uint8_t count = i3c_target_count[i];
	// 		i3c_target_desc[cfg->bus][count].info.static_addr = cfg->address;
	// 		i3c_target_desc[cfg->bus][count].info.assigned_dynamic_addr = cfg->address;
	// 		i3c_target_desc[cfg->bus][count].info.i2c_mode = 0;
	// 		ret = i3c_master_attach_device(dev_i3c[cfg->bus],
	// 					       &i3c_target_desc[cfg->bus][count]);
	// 		if (ret) {
	// 			printf("Failed to attach device, bus %d, addr 0x%x\n", cfg->bus,
	// 			       cfg->address);
	// 			continue;
	// 		}
	// 		if (cfg->enable_ibi) {
	// 			ret = i3c_master_send_getpid(
	// 				dev_i3c[cfg->bus], cfg->address,
	// 				&i3c_target_desc[cfg->bus][count].info.pid);
	// 			if (ret) {
	// 				printf("Failed to get PID, bus %d, addr 0x%x\n", cfg->bus,
	// 				       cfg->address);
	// 				continue;
	// 			}

	// 			ret = i3c_master_send_getbcr(
	// 				dev_i3c[cfg->bus], cfg->address,
	// 				&i3c_target_desc[cfg->bus][count].info.bcr);
	// 			if (ret) {
	// 				printf("Failed to get BCR, bus %d, addr 0x%x\n", cfg->bus,
	// 				       cfg->address);
	// 				continue;
	// 			}
	// 			i3c_master_request_ibi(&i3c_target_desc[cfg->bus][count],
	// 					       &i3c_ibi_def_callbacks);
	// 			ret = i3c_master_enable_ibi(&i3c_target_desc[cfg->bus][count]);
	// 			if (ret) {
	// 				printf("Failed to enable IBI, bus %d, addr 0x%x\n",
	// 				       cfg->bus, cfg->address);
	// 				continue;
	// 			}
	// 		}
	// 		i3c_target_count[i]++;
	// 	}
	// }
	if (k_mutex_unlock(&i3c_mutex[bus])) {
		printf("[%s] I2C %d master write release mutex fail\n", __func__, bus);
		return false;
	}
	return true;
error:
	k_mutex_unlock(&i3c_mutex[bus]);
	return false;
}

// __weak bool pal_load_i3c_target_config(void)
// {
// 	return true;
// }

void util_init_i3c()
{
#ifdef DEV_I3C_0
	i3c_bus_info[0].dev_i3c = device_get_binding("I3C_0");
	if (k_mutex_init(&i3c_mutex[0]))
		printf("i3c0 mutex init fail\n");
#endif

#ifdef DEV_I3C_1
	dev_i3c[1] = device_get_binding("I3C_1");
	if (k_mutex_init(&i3c_mutex[1]))
		printf("i3c1 mutex init fail\n");
#endif

#ifdef DEV_I3C_2
	dev_i3c[2] = device_get_binding("I3C_2");
	if (k_mutex_init(&i3c_mutex[2]))
		printf("i3c2 mutex init fail\n");
#endif

#ifdef DEV_I3C_3
	dev_i3c[3] = device_get_binding("I3C_3");
	if (k_mutex_init(&i3c_mutex[3]))
		printf("i3c3 mutex init fail\n");
#endif

	// int ret;
	// pal_load_i3c_target_config();
	// for (int i = 0; i < I3C_BUS_MAX_NUM * I3C_TARGET_DEV_MAX_NUM; i++) {
	// 	if (i3c_target_cfg[i].address != 0) {
	// 		I3C_TARGET_CFG *cfg = &i3c_target_cfg[i];
	// 		uint8_t count = i3c_target_count[i];
	// 		i3c_target_desc[cfg->bus][count].info.static_addr = cfg->address;
	// 		i3c_target_desc[cfg->bus][count].info.assigned_dynamic_addr = cfg->address;
	// 		i3c_target_desc[cfg->bus][count].info.i2c_mode = 0;
	// 		ret = i3c_master_attach_device(dev_i3c[cfg->bus],
	// 					       &i3c_target_desc[cfg->bus][count]);
	// 		if (ret) {
	// 			printf("Failed to attach device, bus %d, addr 0x%x\n", cfg->bus,
	// 			       cfg->address);
	// 			continue;
	// 		}
	// 		i3c_target_count[i]++;
	// 	}
	// }
}