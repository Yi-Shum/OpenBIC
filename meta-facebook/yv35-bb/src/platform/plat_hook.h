#ifndef PLAT_HOOK_H
#define PLAT_HOOK_H

typedef struct _ltc4282_pre_proc_arg {
	char *vsource_status;
} ltc4282_pre_proc_arg;

typedef struct _ast_adc_post_proc_arg {
	float amplification;
} ast_adc_post_proc_arg;

/**************************************************************************************************
 * INIT ARGS
**************************************************************************************************/
extern ast_adc_init_arg ast_adc_init_args[];

extern adm1278_init_arg adm1278_init_args[];

extern ltc4282_init_arg ltc4282_init_args[];

/**************************************************************************************************
 *  PRE-HOOK/POST-HOOK ARGS
 **************************************************************************************************/
extern ltc4282_pre_proc_arg ltc4282_pre_read_args[];

extern ast_adc_post_proc_arg ast_adc_post_args[];

/**************************************************************************************************
 *  PRE-HOOK/POST-HOOK FUNC
 **************************************************************************************************/
bool pre_ltc4282_read(uint8_t sensor_num, void *args);
bool post_ast_adc_read(uint8_t sensor_num, void *args, int *reading);

#endif
