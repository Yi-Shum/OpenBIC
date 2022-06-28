#include "plat_i2c.h"
#include "hal_i2c.h"
#include "util_spi.h"

#define CPLD_ADDR 0x21
#define CPLD_SPI_OOB_CONTROL_REG 0x0B

int pal_get_bios_flash_position()
{
	return DEVSPI_SPI1_CS0;
}

bool pal_switch_bios_spi_mux(uint8_t data)
{
	uint8_t retry = 5;
	I2C_MSG msg;

	msg.bus = I2C_BUS1;
	msg.target_addr = CPLD_ADDR;
	msg.tx_len = 2;
	msg.data[0] = CPLD_SPI_OOB_CONTROL_REG;
	msg.data[1] = data;

	if (i2c_master_write(&msg, retry)) {
		return false;
	}
	return true;
}
