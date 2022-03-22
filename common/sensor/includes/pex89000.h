#ifndef PEX89000_H
#define PEX89000_H

#define BRCM_I2C5_CMD_READ 0b100
#define BRCM_I2C5_CMD_WRITE 0b011

#define BRCM_I2C5_ACCESS_MODE_FULL 2
#define BRCM_I2C5_ACCESS_MODE_LOWER18 0

#define BRCM_CHIME_AXI_CSR_ADDR 0x001F0100
#define BRCM_CHIME_AXI_CSR_DATA 0x001F0104
#define BRCM_CHIME_AXI_CSR_CTL 0x001F0108

#define BRCM_TEMP_SENSOR0_CTL_REG1 0xFFE78504
#define BRCM_TEMP_SENSOR0_STAT_REG0 0xFFE78538

#define BRCM_TEMP_SENSOR0_CTL_REG1_RESET 0x000653E8

#define BRCM_FULL_ADDR_BIT 0x007C0000 // bits[22:18]

#define PEX89000_TEMP_OFFSET 0xFE // TBD: sensor offset

typedef struct _pex89000_init_arg {
	uint8_t idx;
	struct k_mutex brcm_pciesw;

	/* Initailize function will set following arguments, no need to give value */
	bool is_init;
} pex89000_init_arg;

#endif
