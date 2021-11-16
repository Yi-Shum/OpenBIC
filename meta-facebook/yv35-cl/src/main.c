/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel.h>
#include <cmsis_os2.h>
#include <sys/printk.h>
#include "sensor.h"
#include "hal_i2c.h"
#include "plat_gpio.h"
#include "ipmi_def.h"
#include "ipmi.h"
#include "kcs.h"
#include "plat_func.h"
#include <drivers/i3c/i3c.h>
#include <device.h>

struct k_thread I3C_thread;
K_KERNEL_STACK_MEMBER(I3C_thread_stack, 4000);

void device_init() {
  adc_init();
  peci_init();
}

void set_sys_status() {
  gpio_set(FM_SPI_PCH_MASTER_SEL_R, GPIO_LOW);
  gpio_set(BIC_READY, GPIO_HIGH);
  set_DC_status();
  set_post_status();
}

void I3C_handler(void *arug0, void *arug1, void *arug2){
  const struct device *slave_mq;
  slave_mq = device_get_binding(DT_LABEL(DT_NODELABEL(i3c0_smq)));
  if (!slave_mq) {
    printk("slave-mq device not found\n");
    return;
  }

  uint8_t result[256];
  int ret, index;
  while(1){
    ret = i3c_slave_mqueue_read(slave_mq, result, 256);
    if (ret)
    {
      printk("receive data from i3c bus: ");
      for (index = 0; index < ret; index++){
        printk("0x%x ",result[index]);
      }
      printk("\n");
    }
    k_msleep(1);
  }
}

void main(void)
{
  uint8_t proj_stage = (FIRMWARE_REVISION_1 & 0xf0) >> 4;
  printk("Hello, wellcome to yv35 craterlake POC %d\n", FIRMWARE_REVISION_2);

  util_init_timer();
  util_init_I2C();

  // sensor_init();
  FRU_init();
  ipmi_init();
  kcs_init();
  usb_dev_init();
  device_init();
  set_sys_status();
  k_thread_create(&I3C_thread, I3C_thread_stack,
                K_THREAD_STACK_SIZEOF(I3C_thread_stack),
                I3C_handler,
                NULL, NULL, NULL,
                osPriorityBelowNormal, 0, K_NO_WAIT);
}

#define DEF_PROJ_GPIO_PRIORITY 61

DEVICE_DEFINE(PRE_DEF_PROJ_GPIO, "PRE_DEF_PROJ_GPIO_NAME",
        &gpio_init, NULL,
        NULL, NULL,
        POST_KERNEL, DEF_PROJ_GPIO_PRIORITY,
        NULL);
