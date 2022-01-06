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
#include "util_worker.h"

void device_init() {
  adc_init();
  peci_init();
  hsc_init();
}

void set_sys_status() {
  gpio_set(FM_SPI_PCH_MASTER_SEL_R, GPIO_LOW);
  gpio_set(BIC_READY, GPIO_HIGH);
  set_DC_status();
  set_post_status();
  set_SCU_setting();
}

void stress_fn(void* ptr, uint32_t ui32arg){
  if (ui32arg == 0) {
    printk("[%d] test_fn, delay work, ptr = %p, count = %d, ui32arg = %d\n",k_uptime_get_32(), ptr, *((int*)ptr), ui32arg);
  }
  else {
    printk("[%d] test_fn, no delay work, ptr = %p, count = %d, ui32arg = %d\n",k_uptime_get_32(), ptr, *((int*)ptr), ui32arg);
  }
  free(ptr);
}

void delay_fn(void* ptr, uint32_t ui32arg){
  printk("delay_fn, sleep 2000ms\n");
  k_msleep(2000);
}

void main(void)
{
  uint8_t proj_stage = (FIRMWARE_REVISION_1 & 0xf0) >> 4;
  printk("Hello, wellcome to yv35 craterlake %x%x.%x.%x\n", BIC_FW_YEAR_MSB, BIC_FW_YEAR_LSB, BIC_FW_WEEK, BIC_FW_VER);

  util_init_timer();
  util_init_I2C();

  set_sys_config();
  disable_PRDY_interrupt();
  sensor_init();
  FRU_init();
  ipmi_init();
  kcs_init();
  usb_dev_init();
  device_init();
  set_sys_status();


  k_msleep(1000);

  init_worker();
  int ret = 0;
	int *a;
	int count = 0;
	worker_job job;

 	/* stress */
  while(1) {
    count++;
      
    /* delay work */
    a = malloc(sizeof(int));
    // printk("delay, count %d, a = %p\n", count, a);
    if( a == NULL ) {
      printk("Error: unable to allocate required memory\n");
      continue;
    }
    *a = count;
    job.fn = stress_fn;
    job.ptr_arg = a;
    job.ui32_arg = 0;
    job.delay_ms = 100;
    job.name = malloc(32 * sizeof(char));
    if (job.name == NULL) {
      printk("allcate job.name fail\n");
      free(a);
      continue;
    }
    sprintf(job.name, "delay_work_%d", count);

    ret = add_work(job);
    if (ret != 1) {
      free(a);
      printk("add_work fail, ret: %d\n", ret);
    }
    free(job.name);
    k_msleep(15);

    /* no delay work */
    a = malloc(sizeof(int));
    // printk("no delay, count %d, a = %p\n", count, a);
    if( a == NULL ) {
      printk("Error: unable to allocate required memory\n");
      continue;
    }
    *a = count;
    job.fn = stress_fn;
    job.ptr_arg = a;
    job.ui32_arg = 1;
    job.delay_ms = 0;
    job.name = malloc(32 * sizeof(char));
    if (job.name == NULL) {
      printk("allcate job.name fail\n");
      free(a);
      continue;
    }
    sprintf(job.name, "work_%d", count);
    ret = add_work(job);
    if (ret != 1) {
      free(a);
      printk("add_work fail, ret: %d\n", ret);
    }
    free(job.name);
    k_msleep(15);
  }

  // /* work full test */
  // while(1) {
  //   count++;
      
  //   /* long delay work */
  //   a = malloc(sizeof(int));
  //   // printk("delay, count %d, a = %p\n", count, a);
  //   if( a == NULL ) {
  //     printk("Error: unable to allocate required memory\n");
  //     continue;
  //   }
  //   *a = count;
  //   job.fn = stress_fn;
  //   job.ptr_arg = a;
  //   job.ui32_arg = 0;
  //   job.delay_ms = 2000;
  //   job.name = malloc(32 * sizeof(char));
  //   if (job.name == NULL) {
  //     printk("allcate job.name fail\n");
  //     free(a);
  //     continue;
  //   }
  //   sprintf(job.name, "delay_work_%d", count);

  //   ret = add_work(job);
  //   if (ret != 1) {
  //     free(a);
  //     printk("add_work fail, count %d, ret: %d\n", count, ret);
  //     printk("get_work_count %d\n", get_work_count());
  //     k_msleep(4000);
  //   }
  //   free(job.name);
  //   k_msleep(20);
  // }

  // /* processing time too long test */
  // while(1) {
  //   /* add work with sleep 2 seconds */
  //   job.fn = delay_fn;
  //   job.ptr_arg = NULL;
  //   job.ui32_arg = 0;
  //   job.delay_ms = 0;
  //   job.name = "DELAY_2_SEC_WORK";

  //   ret = add_work(job);
  //   if (ret != 1) {
  //     printk("add_work fail, ret: %d\n", ret);
  //   }
  //   k_msleep(2000);
  // }

}

#define DEF_PROJ_GPIO_PRIORITY 61

DEVICE_DEFINE(PRE_DEF_PROJ_GPIO, "PRE_DEF_PROJ_GPIO_NAME",
        &gpio_init, NULL,
        NULL, NULL,
        POST_KERNEL, DEF_PROJ_GPIO_PRIORITY,
        NULL);
