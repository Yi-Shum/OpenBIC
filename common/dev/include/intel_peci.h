/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef PECI_H
#define PECI_H

#include <stdbool.h>
#include <stdint.h>

enum {
	PECI_UNKNOWN = 0x00,
	PECI_TEMP_CPU_MARGIN,
	PECI_TEMP_CPU,
	PECI_TEMP_CPU_TJMAX,
	PECI_TEMP_CHANNEL0_DIMM0,
	PECI_TEMP_CHANNEL0_DIMM1,
	PECI_TEMP_CHANNEL1_DIMM0,
	PECI_TEMP_CHANNEL1_DIMM1,
	PECI_TEMP_CHANNEL2_DIMM0,
	PECI_TEMP_CHANNEL2_DIMM1,
	PECI_TEMP_CHANNEL3_DIMM0,
	PECI_TEMP_CHANNEL3_DIMM1,
	PECI_TEMP_CHANNEL4_DIMM0,
	PECI_TEMP_CHANNEL4_DIMM1,
	PECI_TEMP_CHANNEL5_DIMM0,
	PECI_TEMP_CHANNEL5_DIMM1,
	PECI_TEMP_CHANNEL6_DIMM0,
	PECI_TEMP_CHANNEL6_DIMM1,
	PECI_TEMP_CHANNEL7_DIMM0,
	PECI_TEMP_CHANNEL7_DIMM1,
	PECI_PWR_CPU,
	PECI_MAX,
};

typedef struct {
	uint8_t time_unit;
	uint8_t energy_unit;
	uint8_t power_unit;
} intel_peci_unit;

bool peci_sensor_read(uint8_t sensor_num, int *reading);
bool check_dimm_present(uint8_t dimm_channel, uint8_t dimm_num, uint8_t *present_result);

#endif
