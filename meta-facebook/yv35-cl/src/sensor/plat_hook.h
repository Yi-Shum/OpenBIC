#ifndef PLAT_HOOK_H
#define PLAT_HOOK_H

#include "adm1278.h"
#include "ast_adc.h"
#include "i2c-mux-tca9548.h"

typedef struct _isl69259_pre_proc_arg {
	/* vr page to set */
	uint8_t vr_page;
} isl69259_pre_proc_arg;

typedef struct _ast_adc_post_proc_arg {
	float amplification;
} ast_adc_post_proc_arg;

/**************************************************************************************************
 * INIT ARGS
**************************************************************************************************/
extern ast_adc_init_arg ast_adc_init_args[];
extern adm1278_init_arg adm1278_init_args[];

/**************************************************************************************************
 *  PRE-HOOK/POST-HOOK ARGS
 **************************************************************************************************/
extern tca9548_channel_info mux_conf_addr_0xe2[];
extern isl69259_pre_proc_arg isl69259_pre_read_args[];
extern ast_adc_post_proc_arg ast_adc_post_args[];

/**************************************************************************************************
 *  PRE-HOOK/POST-HOOK FUNC
 **************************************************************************************************/
bool pre_isl69259_read(uint8_t sensor_num, void *args);
bool pre_nvme_read(uint8_t sensor_num, void *args);
bool pre_vol_bat3v_read(uint8_t sensor_num, void *args);
bool post_vol_bat3v_read(uint8_t sensor_num, void *args, int *reading);
bool post_cpu_margin_read(uint8_t sensor_num, void *args, int *reading);
bool post_ast_adc_read(uint8_t sensor_num, void *args, int *reading);

/**************************************************************************************************
 *  ACCESS CHECK FUNC
 **************************************************************************************************/
bool stby_access(uint8_t sensor_num);
bool DC_access(uint8_t sensor_num);
bool post_access(uint8_t sensor_num);
bool VR_access(uint8_t sensor_num);

#endif
