#ifndef I2C_MUX_TCA9548_H
#define I2C_MUX_TCA9548_H

typedef struct _tca9548_channel_info {
	uint8_t addr;
	uint8_t chan;
} tca9548_channel_info;

bool tca9548_select_chan(uint8_t sensor_num, void *args);

#endif