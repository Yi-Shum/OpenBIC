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

#ifndef PLAT_SENSOR_TABLE_H
#define PLAT_SENSOR_TABLE_H

#include <stdint.h>
#include "sensor.h"

/* SENSOR POLLING TIME(second) */
#define POLL_TIME_BAT3V 3600

/* SENSOR ADDRESS(7-bit)/OFFSET */
#define TMP75_IN_ADDR (0x94 >> 1)
#define TMP75_OUT_ADDR (0x92 >> 1)
#define TMP75_IOM_ADDR (0x94 >> 1)
#define TMP75_TEMP_OFFSET 0x00
#define SSD0_ADDR (0xD4 >> 1)
#define SSD0_OFFSET 0x00
#define MPS_MP5990_ADDR (0x40 >> 1)
#define PCH_ADDR (0x2C >> 1)
#define ME_SENSOR_NUM_TEMP_PCH 0x08
#define VR_PU14_SRC0_ADDR (0xC0 >> 1)
#define VR_PU14_SRC1_ADDR (0xC0 >> 1)
#define VR_PU5_SRC0_ADDR (0xC2 >> 1)
#define VR_PU5_SRC1_ADDR (0xC8 >> 1)
#define VR_PU35_SRC0_ADDR (0xC6 >> 1)
#define VR_PU35_SRC1_ADDR (0xC4 >> 1)
#define PVCCIN_ADDR VR_PU14_SRC0_ADDR
#define PVCCFA_EHV_FIVRA_ADDR VR_PU14_SRC0_ADDR
#define PVCCINFAON_ADDR VR_PU5_SRC0_ADDR
#define PVCCFA_EHV_ADDR VR_PU5_SRC0_ADDR
#define PVCCD_HV_ADDR VR_PU35_SRC0_ADDR
#define VR_VOL_CMD 0x8B
#define VR_CUR_CMD 0x8C
#define VR_CUR_IN_CMD 0x89
#define VR_TEMP_CMD 0x8D
#define VR_PWR_CMD 0x96
#define CPU_PECI_ADDR 0x30
#define INA230_ADDR (0x82 >> 1)

/* SENSOR NUMBER(1 based) - temperature */
#define SENSOR_NUM_TEMP_TMP75_IN 0x01
#define SENSOR_NUM_TEMP_TMP75_OUT 0x02
#define SENSOR_NUM_TEMP_TMP75_IOM 0x03
#define SENSOR_NUM_TEMP_PCH 0x04
#define SENSOR_NUM_TEMP_CPU 0x05
#define SENSOR_NUM_TEMP_CPU_MARGIN 0x06
#define SENSOR_NUM_TEMP_CPU_TJMAX 0x07
#define SENSOR_NUM_TEMP_SSD0 0x08
#define SENSOR_NUM_TEMP_HSC 0x09
#define SENSOR_NUM_TEMP_DIMM_A 0x0A
#define SENSOR_NUM_TEMP_DIMM_C 0x0B
#define SENSOR_NUM_TEMP_DIMM_E 0x0C
#define SENSOR_NUM_TEMP_DIMM_G 0x0D
#define SENSOR_NUM_TEMP_PVCCIN 0x0E
#define SENSOR_NUM_TEMP_PVCCFA_EHV_FIVRA 0x0F
#define SENSOR_NUM_TEMP_PVCCFA_EHV 0x10
#define SENSOR_NUM_TEMP_PVCCD_HV 0x11
#define SENSOR_NUM_TEMP_PVCCINFAON 0x12

/* SENSOR NUMBER(1 based) - voltage */
#define SENSOR_NUM_VOL_STBY12V 0x20
#define SENSOR_NUM_VOL_BAT3V 0x21
#define SENSOR_NUM_VOL_STBY3V 0x22
#define SENSOR_NUM_VOL_STBY1V8 0x23
#define SENSOR_NUM_VOL_STBY1V05 0x24
#define SENSOR_NUM_VOL_STBY5V 0x25
#define SENSOR_NUM_VOL_DIMM12V 0x26
#define SENSOR_NUM_VOL_STBY1V2 0x27
#define SENSOR_NUM_VOL_M2_3V3 0x28
#define SENSOR_NUM_VOL_HSCIN 0x29
#define SENSOR_NUM_VOL_PVCCIN 0x2A
#define SENSOR_NUM_VOL_PVCCFA_EHV_FIVRA 0x2B
#define SENSOR_NUM_VOL_PVCCFA_EHV 0x2C
#define SENSOR_NUM_VOL_PVCCD_HV 0x2D
#define SENSOR_NUM_VOL_PVCCINFAON 0x2E
#define SENSOR_NUM_VOL_IOM_INA 0x2F

/* SENSOR NUMBER(1 based) - current */
#define SENSOR_NUM_CUR_HSCOUT 0x40
#define SENSOR_NUM_CUR_PVCCIN 0x41
#define SENSOR_NUM_CUR_PVCCFA_EHV_FIVRA 0x42
#define SENSOR_NUM_CUR_PVCCFA_EHV 0x43
#define SENSOR_NUM_CUR_PVCCD_HV 0x44
#define SENSOR_NUM_CUR_IN_PVCCD_HV 0x45
#define SENSOR_NUM_CUR_PVCCINFAON 0x46
#define SENSOR_NUM_CUR_IOM_INA 0x47

/* SENSOR NUMBER(1 based) - power */
#define SENSOR_NUM_PWR_CPU 0x60
#define SENSOR_NUM_PWR_HSCIN 0x61
#define SENSOR_NUM_PWR_DIMMA_PMIC 0x62
#define SENSOR_NUM_PWR_DIMMC_PMIC 0x63
#define SENSOR_NUM_PWR_DIMME_PMIC 0x64
#define SENSOR_NUM_PWR_DIMMG_PMIC 0x65
#define SENSOR_NUM_PWR_PVCCIN 0x66
#define SENSOR_NUM_PWR_PVCCFA_EHV_FIVRA 0x67
#define SENSOR_NUM_PWR_PVCCFA_EHV 0x68
#define SENSOR_NUM_PWR_PVCCD_HV 0x69
#define SENSOR_NUM_PWR_PVCCINFAON 0x6A
#define SENSOR_NUM_PWR_IOM_INA 0x6B

/* SENSOR NUMBER - sel */
#define SENSOR_NUM_SYSTEM_STATUS 0x10
#define SENSOR_NUM_POWER_ERROR 0x56
#define SENSOR_NUM_PROC_FAIL 0x65
#define SENSOR_NUM_VR_HOT 0xB2
#define SENSOR_NUM_CPUDIMM_HOT 0xB3
#define SENSOR_NUM_PMIC_ERROR 0xB4
#define SENSOR_NUM_CATERR 0xEB
#define SENSOR_NUM_RMCA 0xEC //TBD: BMC should know this

typedef struct _dimm_pmic_mapping_cfg {
	uint8_t dimm_sensor_num;
	uint8_t mapping_pmic_sensor_num;
} dimm_pmic_mapping_cfg;

uint8_t plat_get_config_size();
void load_sensor_config(void);
bool disable_dimm_pmic_sensor(uint8_t sensor_num);

#endif
