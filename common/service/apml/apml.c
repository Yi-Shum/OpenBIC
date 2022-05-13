
#include <stdio.h>
#include "hal_i2c.h"
#include "apml.h"

/****************** TSI *********************/

bool TSI_read(uint8_t bus, uint8_t addr, uint8_t command, uint8_t *read_data)
{
	uint8_t retry = 5;
	I2C_MSG msg;
	msg.bus = bus;
	msg.target_addr = addr;
	msg.tx_len = 1;
	msg.rx_len = 1;
	msg.data[0] = command;

	if (i2c_master_read(&msg, retry)) {
		return false;
	}
	*read_data = msg.data[0];
	return true;
}

bool TSI_write(uint8_t bus, uint8_t addr, uint8_t command, uint8_t write_data)
{
	uint8_t retry = 5;
	I2C_MSG msg;
	msg.bus = bus;
	msg.target_addr = addr;
	msg.tx_len = 1;
	msg.rx_len = 0;
	msg.data[0] = command;

	if (i2c_master_write(&msg, retry)) {
		return false;
	}
	return true;
}

bool TSI_set_temperature_throttle(uint8_t bus, uint8_t addr, uint8_t temp_threshold, uint8_t rate,
				  bool alert_comparator_mode)
{
	/* 
	 * 1. Set high temperature threshold.
	 * 2. Set 'alert comparator mode' enable/disable.
	 * 3. Set update rate.
	 */
	if (!TSI_write(bus, addr, TSI_HIGH_TEMP_INT, temp_threshold)) {
		return false;
	}

	if (alert_comparator_mode) {
		if (TSI_write(bus, addr, TSI_ALERT_CONFIG, 0x01)) {
			return false;
		}
	} else {
		if (TSI_write(bus, addr, TSI_ALERT_CONFIG, 0x00)) {
			return false;
		}
	}

	if (TSI_write(bus, addr, TSI_UPDATE_RATE, rate)) {
		return false;
	}
	return true;
}
