#include <stdio.h>
#include <string.h>
#include "sdr.h"
#include "sensor.h"
#include "sensor_def.h"
#include "hal_i2c.h"
#include "plat_i2c.h"
#include "plat_func.h"
#include "pal.h"

typedef struct _isl69254_pre_proc_arg {
  struct tca9548 *mux_conf; 
  /* vr page to set */
  uint8_t vr_page;
} isl69254_pre_proc_arg;

bool stby_access(uint8_t snr_num);
bool DC_access(uint8_t snr_num);
bool post_access(uint8_t snr_num);
bool pre_tmp75_read(uint8_t snr_num, void *args);
bool post_tmp75_read(uint8_t snr_num, void *args);
bool pre_isl69254_read(uint8_t snr_num, void *args);
bool pre_nvme_read(uint8_t snr_num, void *args);

/* Init flag */
adc_asd_init_arg adc_asd_init_args[] = {
  [0] = {.is_init = false}
};

uint8_t tmp75_test[] = {62, 93};

struct tca9548 mux_conf_addr_0xe0[8] =
{
  [0] = {.addr = 0xe0, .chan = 0},
  [1] = {.addr = 0xe0, .chan = 1},
  [2] = {.addr = 0xe0, .chan = 2},
  [3] = {.addr = 0xe0, .chan = 3},
  [4] = {.addr = 0xe0, .chan = 4},
  [5] = {.addr = 0xe0, .chan = 5},
  [6] = {.addr = 0xe0, .chan = 6},
  [7] = {.addr = 0xe0, .chan = 7},
};
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

isl69254_pre_proc_arg isl69254_pre_read_args[] =
{
        /* mux_conf,                vr_page */
  [0] = { &mux_conf_addr_0xe2[6],      0x0},
  [1] = { &mux_conf_addr_0xe2[6],      0x1},
};

static uint8_t SnrCfg_num;

snr_cfg plat_sensor_config[] = {
  /* number,                           type,            port,           address,                  offset,             access check       arg0,   arg1,   cache,   cache_status,              pre_hook_fn,                pre_hook_args,         post_hook_fn,      post_hook_args,              init_arg */

  // temperature
  {SENSOR_NUM_TEMP_TMP75_IN          , sen_dev_tmp75  , i2c_bus2      , tmp75_in_addr           , tmp75_tmp_offset   , stby_access      , 0     , 0     , 0      , SNR_INIT_STATUS,       pre_tmp75_read,                   tmp75_test,      post_tmp75_read,       tmp75_test + 1,                    NULL},
  {SENSOR_NUM_TEMP_TMP75_OUT         , sen_dev_tmp75  , i2c_bus2      , tmp75_out_addr          , tmp75_tmp_offset   , stby_access      , 0     , 0     , 0      , SNR_INIT_STATUS,                 NULL,                         NULL,                 NULL,                 NULL,                    NULL},
  {SENSOR_NUM_TEMP_TMP75_FIO         , sen_dev_tmp75  , i2c_bus2      , tmp75_fio_addr          , tmp75_tmp_offset   , stby_access      , 0     , 0     , 0      , SNR_INIT_STATUS,                 NULL,                         NULL,                 NULL,                 NULL,                    NULL},

  // NVME
  {SENSOR_NUM_TEMP_SSD0              , sen_dev_nvme   , i2c_bus2      , SSD0_addr               , SSD0_offset        , DC_access        , 0     , 0     , 0      , SNR_INIT_STATUS,        pre_nvme_read,       &mux_conf_addr_0xe2[2],                 NULL,                 NULL,                    NULL},
                                                                                                                                                                 
  // PECI                                                                                                                                                        
  {SENSOR_NUM_TEMP_CPU               , type_peci      , 0             , CPU_PECI_addr           , 0                  , post_access      , 0     , 0     , 0      , SNR_INIT_STATUS,                 NULL,                         NULL,                 NULL,                 NULL,                    NULL},
  {SENSOR_NUM_TEMP_CPU_MARGIN        , type_peci      , 0             , CPU_PECI_addr           , 0                  , post_access      , 0     , 0     , 0      , SNR_INIT_STATUS,                 NULL,                         NULL,                 NULL,                 NULL,                    NULL},
  {SENSOR_NUM_TEMP_CPU_TJMAX         , type_peci      , 0             , CPU_PECI_addr           , 0                  , post_access      , 0     , 0     , 0      , SNR_INIT_STATUS,                 NULL,                         NULL,                 NULL,                 NULL,                    NULL},
  {SENSOR_NUM_TEMP_DIMM_A            , type_peci      , 0             , CPU_PECI_addr           , 0                  , post_access      , 0     , 0     , 0      , SNR_INIT_STATUS,                 NULL,                         NULL,                 NULL,                 NULL,                    NULL},
  {SENSOR_NUM_TEMP_DIMM_C            , type_peci      , 0             , CPU_PECI_addr           , 0                  , post_access      , 0     , 0     , 0      , SNR_INIT_STATUS,                 NULL,                         NULL,                 NULL,                 NULL,                    NULL},
  {SENSOR_NUM_TEMP_DIMM_D            , type_peci      , 0             , CPU_PECI_addr           , 0                  , post_access      , 0     , 0     , 0      , SNR_INIT_STATUS,                 NULL,                         NULL,                 NULL,                 NULL,                    NULL},
  {SENSOR_NUM_TEMP_DIMM_E            , type_peci      , 0             , CPU_PECI_addr           , 0                  , post_access      , 0     , 0     , 0      , SNR_INIT_STATUS,                 NULL,                         NULL,                 NULL,                 NULL,                    NULL},
  {SENSOR_NUM_TEMP_DIMM_G            , type_peci      , 0             , CPU_PECI_addr           , 0                  , post_access      , 0     , 0     , 0      , SNR_INIT_STATUS,                 NULL,                         NULL,                 NULL,                 NULL,                    NULL},
  {SENSOR_NUM_TEMP_DIMM_H            , type_peci      , 0             , CPU_PECI_addr           , 0                  , post_access      , 0     , 0     , 0      , SNR_INIT_STATUS,                 NULL,                         NULL,                 NULL,                 NULL,                    NULL},
  {SENSOR_NUM_PWR_CPU                , type_peci      , 0             , CPU_PECI_addr           , 0                  , post_access      , 0     , 0     , 0      , SNR_INIT_STATUS,                 NULL,                         NULL,                 NULL,                 NULL,                    NULL},
                                                                                                                                                                 
  // adc voltage                                                                                                                                                 
  {SENSOR_NUM_VOL_STBY12V            , sen_dev_adc_asd, adc_port0     , 0                       , 0                  , stby_access      , 667   , 100   , 0      , SNR_INIT_STATUS,                 NULL,                         NULL,                 NULL,                 NULL,   &adc_asd_init_args[0]},
  {SENSOR_NUM_VOL_STBY3V             , sen_dev_adc_asd, adc_port2     , 0                       , 0                  , stby_access      , 2     , 1     , 0      , SNR_INIT_STATUS,                 NULL,                         NULL,                 NULL,                 NULL,   &adc_asd_init_args[0]},
  {SENSOR_NUM_VOL_STBY1V05           , sen_dev_adc_asd, adc_port3     , 0                       , 0                  , stby_access      , 1     , 1     , 0      , SNR_INIT_STATUS,                 NULL,                         NULL,                 NULL,                 NULL,   &adc_asd_init_args[0]},
  {SENSOR_NUM_VOL_BAT3V              , sen_dev_adc_asd, adc_port4     , 0                       , 0                  , stby_access      , 3     , 1     , 0      , SNR_INIT_STATUS,                 NULL,                         NULL,                 NULL,                 NULL,   &adc_asd_init_args[0]},
  {SENSOR_NUM_VOL_STBY5V             , sen_dev_adc_asd, adc_port9     , 0                       , 0                  , stby_access      , 711   , 200   , 0      , SNR_INIT_STATUS,                 NULL,                         NULL,                 NULL,                 NULL,   &adc_asd_init_args[0]},
  {SENSOR_NUM_VOL_DIMM12V            , sen_dev_adc_asd, adc_port11    , 0                       , 0                  , DC_access        , 667   , 100   , 0      , SNR_INIT_STATUS,                 NULL,                         NULL,                 NULL,                 NULL,   &adc_asd_init_args[0]},
  {SENSOR_NUM_VOL_STBY1V2            , sen_dev_adc_asd, adc_port13    , 0                       , 0                  , stby_access      , 1     , 1     , 0      , SNR_INIT_STATUS,                 NULL,                         NULL,                 NULL,                 NULL,   &adc_asd_init_args[0]},
  {SENSOR_NUM_VOL_M2_3V3             , sen_dev_adc_asd, adc_port14    , 0                       , 0                  , DC_access        , 2     , 1     , 0      , SNR_INIT_STATUS,                 NULL,                         NULL,                 NULL,                 NULL,   &adc_asd_init_args[0]},
  {SENSOR_NUM_VOL_STBY1V8            , sen_dev_adc_asd, adc_port15    , 0                       , 0                  , stby_access      , 1     , 1     , 0      , SNR_INIT_STATUS,                 NULL,                         NULL,                 NULL,                 NULL,   &adc_asd_init_args[0]},

  // VR voltage
  {SENSOR_NUM_VOL_PVCCD_HV           , sen_dev_isl69254, i2c_bus5      , PVCCD_HV_addr           , VR_VOL_CMD        , DC_access        , 0     , 0     , 0      , SNR_INIT_STATUS,    pre_isl69254_read,   &isl69254_pre_read_args[0],                 NULL,                 NULL,                    NULL},
  {SENSOR_NUM_VOL_PVCCINFAON         , sen_dev_isl69254, i2c_bus5      , PVCCINFAON_addr         , VR_VOL_CMD        , DC_access        , 0     , 0     , 0      , SNR_INIT_STATUS,    pre_isl69254_read,   &isl69254_pre_read_args[0],                 NULL,                 NULL,                    NULL},
  {SENSOR_NUM_VOL_PVCCFA_EHV         , sen_dev_isl69254, i2c_bus5      , PVCCFA_EHV_addr         , VR_VOL_CMD        , DC_access        , 0     , 0     , 0      , SNR_INIT_STATUS,    pre_isl69254_read,   &isl69254_pre_read_args[1],                 NULL,                 NULL,                    NULL},
  {SENSOR_NUM_VOL_PVCCIN             , sen_dev_isl69254, i2c_bus5      , PVCCIN_addr             , VR_VOL_CMD        , DC_access        , 0     , 0     , 0      , SNR_INIT_STATUS,    pre_isl69254_read,   &isl69254_pre_read_args[0],                 NULL,                 NULL,                    NULL},
  {SENSOR_NUM_VOL_PVCCFA_EHV_FIVRA   , sen_dev_isl69254, i2c_bus5      , PVCCFA_EHV_FIVRA_addr   , VR_VOL_CMD        , DC_access        , 0     , 0     , 0      , SNR_INIT_STATUS,    pre_isl69254_read,   &isl69254_pre_read_args[1],                 NULL,                 NULL,                    NULL},
  
  // VR current
  {SENSOR_NUM_CUR_PVCCD_HV           , sen_dev_isl69254, i2c_bus5      , PVCCD_HV_addr           , VR_CUR_CMD        , DC_access        , 0     , 0     , 0      , SNR_INIT_STATUS,    pre_isl69254_read,   &isl69254_pre_read_args[0],                 NULL,                 NULL,                    NULL},
  {SENSOR_NUM_CUR_PVCCINFAON         , sen_dev_isl69254, i2c_bus5      , PVCCINFAON_addr         , VR_CUR_CMD        , DC_access        , 0     , 0     , 0      , SNR_INIT_STATUS,    pre_isl69254_read,   &isl69254_pre_read_args[0],                 NULL,                 NULL,                    NULL},
  {SENSOR_NUM_CUR_PVCCFA_EHV         , sen_dev_isl69254, i2c_bus5      , PVCCFA_EHV_addr         , VR_CUR_CMD        , DC_access        , 0     , 0     , 0      , SNR_INIT_STATUS,    pre_isl69254_read,   &isl69254_pre_read_args[1],                 NULL,                 NULL,                    NULL},
  {SENSOR_NUM_CUR_PVCCIN             , sen_dev_isl69254, i2c_bus5      , PVCCIN_addr             , VR_CUR_CMD        , DC_access        , 0     , 0     , 0      , SNR_INIT_STATUS,    pre_isl69254_read,   &isl69254_pre_read_args[0],                 NULL,                 NULL,                    NULL},
  {SENSOR_NUM_CUR_PVCCFA_EHV_FIVRA   , sen_dev_isl69254, i2c_bus5      , PVCCFA_EHV_FIVRA_addr   , VR_CUR_CMD        , DC_access        , 0     , 0     , 0      , SNR_INIT_STATUS,    pre_isl69254_read,   &isl69254_pre_read_args[1],                 NULL,                 NULL,                    NULL},
  
  // VR temperature
  {SENSOR_NUM_TEMP_PVCCD_HV          , sen_dev_isl69254, i2c_bus5      , PVCCD_HV_addr           , VR_TEMP_CMD       , DC_access        , 0     , 0     , 0      , SNR_INIT_STATUS,    pre_isl69254_read,   &isl69254_pre_read_args[0],                 NULL,                 NULL,                    NULL},
  {SENSOR_NUM_TEMP_PVCCINFAON        , sen_dev_isl69254, i2c_bus5      , PVCCINFAON_addr         , VR_TEMP_CMD       , DC_access        , 0     , 0     , 0      , SNR_INIT_STATUS,    pre_isl69254_read,   &isl69254_pre_read_args[0],                 NULL,                 NULL,                    NULL},
  {SENSOR_NUM_TEMP_PVCCFA_EHV        , sen_dev_isl69254, i2c_bus5      , PVCCFA_EHV_addr         , VR_TEMP_CMD       , DC_access        , 0     , 0     , 0      , SNR_INIT_STATUS,    pre_isl69254_read,   &isl69254_pre_read_args[1],                 NULL,                 NULL,                    NULL},
  {SENSOR_NUM_TEMP_PVCCIN            , sen_dev_isl69254, i2c_bus5      , PVCCIN_addr             , VR_TEMP_CMD       , DC_access        , 0     , 0     , 0      , SNR_INIT_STATUS,    pre_isl69254_read,   &isl69254_pre_read_args[0],                 NULL,                 NULL,                    NULL},
  {SENSOR_NUM_TEMP_PVCCFA_EHV_FIVRA  , sen_dev_isl69254, i2c_bus5      , PVCCFA_EHV_FIVRA_addr   , VR_TEMP_CMD       , DC_access        , 0     , 0     , 0      , SNR_INIT_STATUS,    pre_isl69254_read,   &isl69254_pre_read_args[1],                 NULL,                 NULL,                    NULL},
  
  // VR power 
  {SENSOR_NUM_PWR_PVCCD_HV           , sen_dev_isl69254, i2c_bus5      , PVCCD_HV_addr           , VR_PWR_CMD        , DC_access        , 0     , 0     , 0      , SNR_INIT_STATUS,    pre_isl69254_read,   &isl69254_pre_read_args[0],                 NULL,                 NULL,                    NULL},
  {SENSOR_NUM_PWR_PVCCINFAON         , sen_dev_isl69254, i2c_bus5      , PVCCINFAON_addr         , VR_PWR_CMD        , DC_access        , 0     , 0     , 0      , SNR_INIT_STATUS,    pre_isl69254_read,   &isl69254_pre_read_args[0],                 NULL,                 NULL,                    NULL},
  {SENSOR_NUM_PWR_PVCCFA_EHV         , sen_dev_isl69254, i2c_bus5      , PVCCFA_EHV_addr         , VR_PWR_CMD        , DC_access        , 0     , 0     , 0      , SNR_INIT_STATUS,    pre_isl69254_read,   &isl69254_pre_read_args[1],                 NULL,                 NULL,                    NULL},
  {SENSOR_NUM_PWR_PVCCIN             , sen_dev_isl69254, i2c_bus5      , PVCCIN_addr             , VR_PWR_CMD        , DC_access        , 0     , 0     , 0      , SNR_INIT_STATUS,    pre_isl69254_read,   &isl69254_pre_read_args[0],                 NULL,                 NULL,                    NULL},
  {SENSOR_NUM_PWR_PVCCFA_EHV_FIVRA   , sen_dev_isl69254, i2c_bus5      , PVCCFA_EHV_FIVRA_addr   , VR_PWR_CMD        , DC_access        , 0     , 0     , 0      , SNR_INIT_STATUS,    pre_isl69254_read,   &isl69254_pre_read_args[1],                 NULL,                 NULL,                    NULL},
  
  // ME
  {SENSOR_NUM_TEMP_PCH               , type_pch        , i2c_bus3      , PCH_addr                , 0                 , post_access      , 0     , 0     , 0      , SNR_INIT_STATUS,                 NULL,                         NULL,                 NULL,                 NULL,                    NULL},
  
  // HSC
  {SENSOR_NUM_TEMP_HSC               , type_hsc        , i2c_bus2      , HSC_addr                , HSC_TEMP_CMD      , stby_access      , 0     , 0     , 0      , SNR_INIT_STATUS,                 NULL,                         NULL,                 NULL,                 NULL,                    NULL},
  {SENSOR_NUM_VOL_HSCIN              , type_hsc        , i2c_bus2      , HSC_addr                , HSC_VOL_CMD       , stby_access      , 0     , 0     , 0      , SNR_INIT_STATUS,                 NULL,                         NULL,                 NULL,                 NULL,                    NULL},
  {SENSOR_NUM_CUR_HSCOUT             , type_hsc        , i2c_bus2      , HSC_addr                , HSC_CUR_CMD       , stby_access      , 0     , 0     , 0      , SNR_INIT_STATUS,                 NULL,                         NULL,                 NULL,                 NULL,                    NULL},
  {SENSOR_NUM_PWR_HSCIN              , type_hsc        , i2c_bus2      , HSC_addr                , HSC_PWR_CMD       , stby_access      , 0     , 0     , 0      , SNR_INIT_STATUS,                 NULL,                         NULL,                 NULL,                 NULL,                    NULL},
};

snr_cfg fix_C2Snrconfig_table[] = {
// number , type , port , address , offset , access check , arg0 , arg1 , cache , cache_status
};
snr_cfg fix_1ouSnrconfig_table[] = {
// number , type , port , address , offset , access check , arg0 , arg1 , cache , cache_status
};
snr_cfg fix_DVPSnrconfig_table[] = {
// number , type , port , address , offset , access check , arg0 , arg1 , cache , cache_status
};

bool pre_tmp75_read(uint8_t snr_num, void *args)
{
  uint8_t *val = (uint8_t *)args;
  printk("snr_num = %d, args = %d\n", snr_num, *val);
  return true;
}

bool post_tmp75_read(uint8_t snr_num, void *args)
{
  uint8_t *val = (uint8_t *)args;
  printk("snr_num = %d, args = %d\n", snr_num, *val);
  return true;
}

/* ISL6925 pre read function
 *
 * set mux and VR page
 *
 * @param snr_num sensor number
 * @param args pointer to isl69254_pre_proc_arg
 * @retval true if setting mux and page is successful.
 * @retval false if setting mux or page fails.
 */
bool pre_isl69254_read(uint8_t snr_num, void *args) {
  if (args == NULL) {
    return false;
  }

  isl69254_pre_proc_arg *pre_proc_args = (isl69254_pre_proc_arg*)args;
  uint8_t retry = 5;
  I2C_MSG msg;

  if (tca9548_select_chan(snr_num, pre_proc_args->mux_conf)) {
    printk("pre_isl69254_read, set mux fail\n");
    return false;
  }

  /* set page */
  msg.bus = sensor_config[SnrNum_SnrCfg_map[snr_num]].port;
  msg.slave_addr = sensor_config[SnrNum_SnrCfg_map[snr_num]].slave_addr;
  msg.tx_len = 2;
  msg.data[0] = 0x00;
  msg.data[1] = pre_proc_args->vr_page;
  if (i2c_master_write(&msg, retry)) {
    printk("pre_isl69254_read, set page fail\n");
    return false;
  }
  return true;
}

/* nvme pre read function
 *
 * set mux
 *
 * @param snr_num sensor number
 * @param args pointer to struct tca9548
 * @retval true if setting mux is successful.
 * @retval false if setting mux fails.
 */
bool pre_nvme_read(uint8_t snr_num, void *args)
{
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

bool stby_access(uint8_t snr_num) {
  return 1;
}

bool DC_access(uint8_t snr_num) {
  return get_DC_status();
}

bool post_access(uint8_t snr_num) {
  return get_post_status();
}

bool pal_load_snr_config(void) {
  memcpy(&sensor_config[0], &plat_sensor_config[0], sizeof(plat_sensor_config));
  return 1;
};

uint8_t map_SnrNum_Snrconfig( uint8_t sensor_num ) {
  uint8_t i , j;
  for ( i = 0 ; i < SENSOR_NUM_MAX ; i++ ) {
    for ( j = 0 ; j < SnrCfg_num ; ++j ) {
      if ( sensor_num == sensor_config[j].num ) {
        return j;
      } else if ( i == SnrCfg_num ) {
        return 0xFF;
      }
    }
  }
  return 0xFF;
};

void add_Snrconfig( snr_cfg add_Snrconfig ) {
  if ( map_SnrNum_Snrconfig( add_Snrconfig.num ) != 0xFF ) {
    printk( "add sensor num is already exists\n" );
    return;
  }
  sensor_config[ SnrCfg_num++ ] = add_Snrconfig;
};

void pal_fix_Snrconfig() {
/*
  SnrCfg_num = sizeof(plat_sensor_config) / sizeof(plat_sensor_config[0]);
  uint8_t fix_SnrCfg_num;
  if ( get_bic_class() ) {
    // fix usage when fix_C2Snrconfig_table is defined
    fix_SnrCfg_num = sizeof( fix_C2Snrconfig_table ) / sizeof( fix_C2Snrconfig_table[0] );
    while ( fix_SnrCfg_num ) {
      add_Snrconfig ( fix_C2Snrconfig_table[ fix_SnrCfg_num - 1 ] );
      fix_SnrCfg_num--;
    }
  }
  if ( get_1ou_status() ) {
    // fix usage when fix_1ouSnrconfig_table is defined
    fix_SnrCfg_num = sizeof( fix_1ouSnrconfig_table ) / sizeof( fix_1ouSnrconfig_table[0] );
    while ( fix_SnrCfg_num ) {
      add_Snrconfig ( fix_1ouSnrconfig_table[ fix_SnrCfg_num - 1 ] );
      fix_SnrCfg_num--;
    }
  }
  if ( get_2ou_status() ) {
    // fix usage when fix_DVPSnrconfig_table is defined
    fix_SnrCfg_num = sizeof( fix_DVPSnrconfig_table ) / sizeof( fix_DVPSnrconfig_table[0] );
    while ( fix_SnrCfg_num ) {
      add_Snrconfig ( fix_DVPSnrconfig_table[ fix_SnrCfg_num - 1 ] );
      fix_SnrCfg_num--;
    }
  }
  if ( SnrCfg_num != SDR_NUM ) {
    printk("fix sensor SDR and config table not match\n");
  }
*/
};
