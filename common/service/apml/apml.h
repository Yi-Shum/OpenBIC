#ifndef APML_H
#define APML_H

#endif
#ifndef IPMB_H
#define IPMB_H

enum {
	TSI_CPU_TEMP_INT = 0x01,
	TSI_CONFIG = 0x03,
	TSI_UPDATE_RATE = 0x04,
	TSI_HIGH_TEMP_INT = 0x07,
	TSI_CPU_TEMP_DEC = 0x10,
	TSI_ALERT_CONFIG = 0xBF,
};

bool TSI_read(uint8_t bus, uint8_t addr, uint8_t command, uint8_t *read_data);
bool TSI_write(uint8_t bus, uint8_t addr, uint8_t command, uint8_t write_data);
bool TSI_set_temperature_throttle(uint8_t bus, uint8_t addr, uint8_t temp_threshold, uint8_t rate,
				  bool alert_comparator_mode);

#endif
