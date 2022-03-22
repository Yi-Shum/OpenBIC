#ifndef ADM1278_H
#define ADM1278_H

#define REG_PWR_MT_CFG 0xD4

#define ADM1278_EIN_ROLLOVER_CNT_MAX 0x10000
#define ADM1278_EIN_SAMPLE_CNT_MAX 0x1000000
#define ADM1278_EIN_ENERGY_CNT_MAX 0x800000

typedef struct _adm1278_init_arg {
	uint8_t ID;
	/* value to set configuration register */
	union {
		uint16_t value;
		struct {
			uint16_t RSV1 : 1;
			uint16_t VOUT_EN : 1;
			uint16_t VIN_EN : 1;
			uint16_t TEMP1_EN : 1;
			uint16_t PMON_MODE : 1;
			uint16_t RSV2 : 3;
			uint16_t VI_AVG : 3;
			uint16_t PWR_AVG : 3;
			uint16_t SIMULTANEOUS : 1;
			uint16_t TSFILT : 1;
		} fields;
	} config;
	/* Rsense valus, unit: milliohm */
	float r_sense;

	/* Initailize function will set following arguments, no need to give value */
	bool is_init;
} adm1278_init_arg;

enum adm1278_offset {
	ADM1278_PEAK_IOUT_OFFSET = 0xD0,
	ADM1278_PEAK_PIN_OFFSET = 0xDA,
	ADM1278_EIN_EXT_OFFSET = 0xDC,
};

#endif
