#ifndef AST_ADC_H
#define AST_ADC_H

#define ADC_CHANNEL_NUM 8
#define BUFFER_SIZE 1

#if DT_NODE_EXISTS(DT_NODELABEL(adc0))
#define DEV_ADC0
#endif

#if DT_NODE_EXISTS(DT_NODELABEL(adc1))
#define DEV_ADC1
#endif

#define ADC_RESOLUTION 10
#define ADC_CALIBRATE 0
#define ADC_GAIN ADC_GAIN_1
#define ADC_REFERENCE ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME ADC_ACQ_TIME_DEFAULT

enum adc_device_idx { adc0, adc1, ADC_NUM };

typedef struct _ast_adc_init_arg {
	bool is_init;
} ast_adc_init_arg;

#endif
