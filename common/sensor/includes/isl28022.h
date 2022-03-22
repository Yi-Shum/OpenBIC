#ifndef ISL28022_H
#define ISL28022_H

typedef struct _isl28022_init_arg {
	uint8_t ID;
	/* value to set configuration register */
	union {
		uint16_t value;
		struct {
			uint16_t MODE : 3;
			uint16_t SADC : 4;
			uint16_t BADC : 4;
			uint16_t PG : 2;
			uint16_t BRNG : 2;
			uint16_t RST : 1;
		} fields;
	} config;
	/* R_shunt valus, unit: milliohm */
	uint32_t r_shunt;

	/* Initailize function will set following arguments, no need to give value */
	bool is_init;
} isl28022_init_arg;

enum {
	ISL28022_CONFIG_REG = 0x00,
	ISL28022_BUS_VOLTAGE_REG = 0x02,
	ISL28022_POWER_REG = 0x03,
	ISL28022_CURRENT_REG = 0x04,
	ISL28022_CALIBRATION_REG = 0x05,
};

#endif
