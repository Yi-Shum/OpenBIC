#include <stdio.h>
#include <string.h>
#include "sensor.h"
#include "sensor_def.h"
#include "plat_i2c.h"
#include "plat_func.h"
#include "plat_gpio.h"
#include "plat_hook.h"

/**************************************************************************************************
 * INIT ARGS 
**************************************************************************************************/
adc_asd_init_arg adc_asd_init_args[] = {
  [0] = {.is_init = false}
};

adm1278_init_arg adm1278_init_args[] = {
  [0] = {.is_init = false, .config = {0x3F1C}, .r_sense = 0.25}
};

/**************************************************************************************************
 *  PRE-HOOK/POST-HOOK ARGS 
 **************************************************************************************************/
struct tca9548 mux_conf_addr_0xe2[8] =
{
  [0] = {.addr = 0xe2, .chan = 0},
  [1] = {.addr = 0xe2, .chan = 1},
  [2] = {.addr = 0xe2, .chan = 2},
  [3] = {.addr = 0xe2, .chan = 3},
  [4] = {.addr = 0xe2, .chan = 4},
  [5] = {.addr = 0xe2, .chan = 5},
  [6] = {.addr = 0xe2, .chan = 6},
  [7] = {.addr = 0xe2, .chan = 7},
};

isl69259_pre_proc_arg isl69259_pre_read_args[] =
{
  [0] = { 0x0 },
  [1] = { 0x1 },
};

/**************************************************************************************************
 *  PRE-HOOK/POST-HOOK FUNC
 **************************************************************************************************/
/* ISL6925 pre read function
 *
 * set mux and VR page
 *
 * @param snr_num sensor number
 * @param args pointer to isl69259_pre_proc_arg
 * @param reading pointer to reading from previous step
 * @retval true if setting mux and page is successful.
 * @retval false if setting mux or page fails.
 */
bool pre_isl69259_read(uint8_t snr_num, void *args) {
  if (args == NULL) {
    return false;
  }

  isl69259_pre_proc_arg *pre_proc_args = (isl69259_pre_proc_arg*)args;
  uint8_t retry = 5;
  I2C_MSG msg;

  /* set page */
  msg.bus = sensor_config[SnrNum_SnrCfg_map[snr_num]].port;
  msg.slave_addr = sensor_config[SnrNum_SnrCfg_map[snr_num]].slave_addr;
  msg.tx_len = 2;
  msg.data[0] = 0x00;
  msg.data[1] = pre_proc_args->vr_page;
  if (i2c_master_write(&msg, retry)) {
    printk("pre_isl69259_read, set page fail\n");
    return false;
  }
  return true;
}

/* NVME pre read function
 *
 * set mux
 *
 * @param snr_num sensor number
 * @param args pointer to struct tca9548
 * @param reading pointer to reading from previous step
 * @retval true if setting mux is successful.
 * @retval false if setting mux fails.
 */
bool pre_nvme_read(uint8_t snr_num, void *args)
{
  if (!args)
    return false;

  struct tca9548 *pre_proc_args = (struct tca9548 *)args;

  uint8_t retry = 5;

  I2C_MSG msg;
  msg.bus = sensor_config[SnrNum_SnrCfg_map[snr_num]].port;
  msg.slave_addr = pre_proc_args->addr >> 1;
  msg.data[0] = pre_proc_args->chan;
  msg.tx_len = 1;
  msg.rx_len = 0;

  if ( i2c_master_write(&msg, retry) )
    return false;

  return true;
}

/* AST ADC pre read function
 *
 * set gpio high if sensor is "SENSOR_NUM_VOL_BAT3V"
 *
 * @param snr_num sensor number
 * @param args pointer to NULL
 * @param reading pointer to reading from previous step
 * @retval true always.
 * @retval false NULL
 */
bool pre_vol_bat3v_read(uint8_t snr_num, void *args)
{
  ARG_UNUSED(args);

  if( snr_num == SENSOR_NUM_VOL_BAT3V) {
    gpio_set(A_P3V_BAT_SCALED_EN_R, GPIO_HIGH);
    k_msleep(1);
  }

  return true;
}

/* AST ADC post read function
 *
 * set gpio low if sensor is "SENSOR_NUM_VOL_BAT3V"
 *
 * @param snr_num sensor number
 * @param args pointer to NULL
 * @param reading pointer to reading from previous step
 * @retval true always.
 * @retval false NULL
 */
bool post_vol_bat3v_read(uint8_t snr_num, void *args, int *reading)
{
  ARG_UNUSED(args);
  ARG_UNUSED(reading);

  if( snr_num == SENSOR_NUM_VOL_BAT3V)
    gpio_set(A_P3V_BAT_SCALED_EN_R, GPIO_LOW);

  return true;
}

/* INTEL PECI post read function
 *
 * modify certain sensor value after reading
 *
 * @param snr_num sensor number
 * @param args pointer to NULL
 * @param reading pointer to reading from previous step
 * @retval true if no error
 * @retval false if reading get NULL
 */

bool post_cpu_margin_read(uint8_t snr_num, void *args, int *reading)
{
  if (!reading)
    return false;
  ARG_UNUSED(args);

  sen_val *sval = (sen_val *)reading;
  sval->integer = -sval->integer; /* for BMC minus */
  return true;
}

/**************************************************************************************************
 *  ACCESS CHECK FUNC
 **************************************************************************************************/
bool stby_access(uint8_t snr_num) {
  return 1;
}

bool DC_access(uint8_t snr_num) {
  return get_DC_status();
}

bool post_access(uint8_t snr_num) {
  return get_post_status();
}