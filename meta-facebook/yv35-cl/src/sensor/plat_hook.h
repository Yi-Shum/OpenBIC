#ifndef PLAT_HOOK_H
#define PLAT_HOOK_H

typedef struct _isl69254_pre_proc_arg {
  struct tca9548 *mux_conf; 
  /* vr page to set */
  uint8_t vr_page;
} isl69254_pre_proc_arg;

/**************************************************************************************************
 * INIT ARGS 
**************************************************************************************************/
extern adc_asd_init_arg adc_asd_init_args[];

/**************************************************************************************************
 *  PRE-HOOK/POST-HOOK ARGS 
 **************************************************************************************************/
extern uint8_t tmp75_test[];
extern struct tca9548 mux_conf_addr_0xe0[];
extern struct tca9548 mux_conf_addr_0xe2[];
extern isl69254_pre_proc_arg isl69254_pre_read_args[];

/**************************************************************************************************
 *  PRE-HOOK/POST-HOOK FUNC
 **************************************************************************************************/
bool pre_tmp75_read(uint8_t snr_num, void *args);
bool post_tmp75_read(uint8_t snr_num, void *args);
bool pre_isl69254_read(uint8_t snr_num, void *args);
bool pre_nvme_read(uint8_t snr_num, void *args);
bool pre_ast_adc_read(uint8_t snr_num, void *args);
bool post_ast_adc_read(uint8_t snr_num, void *args);

/**************************************************************************************************
 *  ACCESS CHECK FUNC
 **************************************************************************************************/
bool stby_access(uint8_t snr_num);
bool DC_access(uint8_t snr_num);
bool post_access(uint8_t snr_num);

#endif