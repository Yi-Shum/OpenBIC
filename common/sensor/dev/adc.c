#include <stdio.h>
#include "sensor.h"
#include "sensor_def.h"
#include "pal.h"
#include "plat_gpio.h"
#include <zephyr.h>
#include <sys/printk.h>
#include <drivers/adc.h>

#define ADC_NUM 2
#define ADC_CHAN_NUM 8
#define BUFFER_SIZE 1

#if DT_NODE_EXISTS(DT_NODELABEL(adc0))
#define DEV_ADC0
#endif

#if DT_NODE_EXISTS(DT_NODELABEL(adc1))
#define DEV_ADC1
#endif

#define ADC_RESOLUTION          10
#define ADC_CALIBRATE           0
#define ADC_GAIN                ADC_GAIN_1
#define ADC_REFERENCE           ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME    ADC_ACQ_TIME_DEFAULT

static const struct device *dev_adc[ADC_NUM];
static int16_t sample_buffer[BUFFER_SIZE];

static struct adc_channel_cfg channel_cfg = {
  .gain = ADC_GAIN,
  .reference = ADC_REFERENCE,
  .acquisition_time = ADC_ACQUISITION_TIME,
  .channel_id = 0,
  .differential = 0,
};

static struct adc_sequence sequence = {
  .channels    = 0,
  .buffer      = sample_buffer,
  .buffer_size = sizeof(sample_buffer),
  .resolution  = ADC_RESOLUTION,
  .calibrate   = ADC_CALIBRATE,
};

enum {
  adc0,
  adc1,
};

static void init_adc_dev() 
{
#ifdef DEV_ADC0
  dev_adc[adc0] = device_get_binding("ADC0");
#endif

#ifdef DEV_ADC1
  dev_adc[adc1] = device_get_binding("ADC1");
#endif
}

static bool adc_read_mv(uint8_t sensor_num, uint32_t index, uint32_t channel, int *adc_val)
{
  int err, retval;
  sequence.channels = BIT(channel);
  channel_cfg.channel_id = channel;
  retval = adc_channel_setup(dev_adc[index], &channel_cfg);

  if (retval) {
    printk("ADC sensor %x channel set fail\n", sensor_num);
    return false;
  }

  err = adc_read(dev_adc[index], &sequence);
  if (err != 0) {
    printk("ADC sensor %x reading fail with error %d\n", sensor_num, err);
    return false;
  }

  int32_t raw_value = sample_buffer[0];
  if (adc_get_ref(dev_adc[index]) <= 0)
    return false;
  
  *adc_val = raw_value;
  adc_raw_to_millivolts( adc_get_ref(dev_adc[index]), channel_cfg.gain, sequence.resolution, adc_val);

  return true;
}

uint8_t adc_asd_read(uint8_t sensor_num, int *reading) {
    uint8_t snrcfg_sensor_num = SnrNum_SnrCfg_map[sensor_num];
    uint8_t chip = sensor_config[snrcfg_sensor_num].port / ADC_CHAN_NUM;
    uint8_t number = sensor_config[snrcfg_sensor_num].port % ADC_CHAN_NUM;
    int val = 1;

    if ( !adc_read_mv(sensor_num, chip, number, &val) )
        return SNR_FAIL_TO_ACCESS;

    if( sensor_num == SENSOR_NUM_VOL_BAT3V) {
        gpio_set(A_P3V_BAT_SCALED_EN_R, GPIO_HIGH);
        osDelay(1);

        if ( !adc_read_mv(sensor_num, chip, number, &val) )
            return SNR_FAIL_TO_ACCESS;

        gpio_set(A_P3V_BAT_SCALED_EN_R, GPIO_LOW);
    }

    val = val * sensor_config[snrcfg_sensor_num].arg0 / sensor_config[snrcfg_sensor_num].arg1;

    sen_val *sval = (sen_val *)reading;
    sval->integer = (val/1000) & 0xFFFF;
    sval->fraction = (int)( ( (float)val/1000 - val/1000 )*1000 ) & 0xFFFF;

    //printk("ADC_READ[0x%x] = %d (%d.%d)\n", sensor_num, val, sval->integer, sval->fraction);

    return SNR_READ_SUCCESS;
}

uint8_t adc_asd_init(uint8_t sensor_num)
{
    if ( !sensor_config[SnrNum_SnrCfg_map[sensor_num]].init_args )
        return false;

    adc_asd_init_param *init_args = (adc_asd_init_param *) sensor_config[SnrNum_SnrCfg_map[sensor_num]].init_args;
    if ( init_args->is_init )
        goto skip_init;

    init_adc_dev();
  
    if (! ( device_is_ready(dev_adc[0]) && device_is_ready(dev_adc[1]) ) ) {
        printk("ADC device not found\n");
        return false;
    }

    for (uint8_t i = 0; i < ADC_NUM; i++) {
        channel_cfg.channel_id = i;
        adc_channel_setup(dev_adc[i], &channel_cfg);
        sequence.channels |= BIT(i);
    }

    init_args->is_init = true;

skip_init:
    sensor_config[SnrNum_SnrCfg_map[sensor_num]].read = adc_asd_read;

    return true;
}