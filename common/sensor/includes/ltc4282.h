#ifndef LTC4282_H
#define LTC4282_H

enum ltc4282_offset {
	LTC4282_ADJUST_OFFSET = 0x11,
	LTC4282_VSOURCE_OFFSET = 0x3A,
	LTC4282_VSENSE_OFFSET = 0x40,
	LTC4282_POWER_OFFSET = 0x46,
};

typedef struct _ltc4282_init_arg {
	uint8_t ID;
	float r_sense;

	/* Initailize function will set following arguments, no need to give value */
	bool is_init;
} ltc4282_init_arg;

#endif
