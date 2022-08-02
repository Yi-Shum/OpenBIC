#ifndef HAL_I3C_H
#define HAL_I3C_H

#include <drivers/i3c/i3c.h>

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i3c0), okay) && !DT_PROP_OR(DT_NODELABEL(i3c0), secondary, 0)
#define DEV_I3C_0
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i3c1), okay) && !DT_PROP_OR(DT_NODELABEL(i3c1), secondary, 0)
#define DEV_I3C_1
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i3c2), okay) && !DT_PROP_OR(DT_NODELABEL(i3c2), secondary, 0)
#define DEV_I3C_2
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i3c3), okay) && !DT_PROP_OR(DT_NODELABEL(i3c3), secondary, 0)
#define DEV_I3C_3
#endif

#define I3C_BUS_MAX_NUM 4
#define I3C_TARGET_DEV_MAX_NUM 32
#define I3C_BUFF_SIZE 256

typedef struct _I3C_TARGET_CFG {
	uint8_t bus;
	uint8_t address;
	bool enable_ibi;
} I3C_TARGET_CFG;

typedef struct _I3C_MSG_ {
	uint8_t bus;
	uint8_t target_addr;
	uint8_t rx_len;
	uint8_t tx_len;
	uint8_t data[I3C_BUFF_SIZE];
} I3C_MSG;

struct i3c_msg_package {
	uint16_t msg_length;
	uint8_t msg[CONFIG_I3C_ASPEED_MAX_IBI_PAYLOAD];
};

typedef struct _i3c_bus_data {
	uint8_t bus_num;
	const struct device *dev_i3c;
	struct i3c_dev_desc i3c_slave;
	bool is_secondary_master;
	struct k_msgq slave_msgq;
	struct i3c_msg_package current_msg;
} i3c_bus_data;

/* external reference */
int i3c_slave_mqueue_read(const struct device *dev, uint8_t *dest, int budget);
int i3c_slave_mqueue_write(const struct device *dev, uint8_t *src, int size);

int i3c_master_read(I3C_MSG *msg, uint8_t retry);
int i3c_master_write(I3C_MSG *msg, uint8_t retry);
bool util_i3c_bus_init(uint8_t bus);
void util_init_i3c();

#endif
