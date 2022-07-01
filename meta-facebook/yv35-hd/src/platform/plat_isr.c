#include "plat_isr.h"
#include "plat_gpio.h"
#include "plat_sensor_table.h"
#include "libipmi.h"
#include "power_status.h"
#include "ipmi.h"

K_WORK_DELAYABLE_DEFINE(set_DC_on_5s_work, set_DC_on_delayed_status);
K_WORK_DELAYABLE_DEFINE(set_DC_off_10s_work, set_DC_off_delayed_status);
#define DC_ON_5_SECOND 5
#define DC_OFF_10_SECOND 10
void ISR_DC_ON()
{
	set_DC_status(RST_PLTRST_BIC_N);
	if (get_DC_status() == true) {
		k_work_schedule(&set_DC_on_5s_work, K_SECONDS(DC_ON_5_SECOND));

		if (k_work_cancel_delayable(&set_DC_off_10s_work) != 0) {
			printf("Cancel set dc off delay work fail\n");
		}
		set_DC_off_delayed_status();
	} else {
		set_DC_on_delayed_status();
		k_work_schedule(&set_DC_off_10s_work, K_SECONDS(DC_OFF_10_SECOND));

		if ((gpio_get(FM_CPU_BIC_SLP_S3_N) == GPIO_HIGH) &&
		    (gpio_get(RST_RSMRST_BMC_N) == GPIO_HIGH)) {
			common_addsel_msg_t sel_msg;
			sel_msg.InF_target = BMC_IPMB;
			sel_msg.sensor_type = IPMI_OEM_SENSOR_TYPE_OEM_C3;
			sel_msg.event_type = IPMI_EVENT_TYPE_SENSOR_SPEC;
			sel_msg.sensor_number = SENSOR_NUM_POWER_ERROR;
			sel_msg.event_data1 = IPMI_OEM_EVENT_OFFSET_SYS_PWROK_FAIL;
			sel_msg.event_data2 = 0xFF;
			sel_msg.event_data3 = 0xFF;
			if (!common_add_sel_evt_record(&sel_msg)) {
				printf("Failed to addsel of DC off error\n");
			}
			printf("success to addsel\n");
		}
	}
}
