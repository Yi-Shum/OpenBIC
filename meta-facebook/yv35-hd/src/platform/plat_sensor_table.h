#ifndef PLAT_SENSOR_TABLE_H
#define PLAT_SENSOR_TABLE_H

#include <stdint.h>

/*  define config for sensors  */

/*  threshold sensor number, 1 based  */
#define SENSOR_NUM_PWR_HSCIN 0x39

uint8_t load_sensor_config(void);

#endif
