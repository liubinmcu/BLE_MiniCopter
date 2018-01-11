#ifndef __APP_GLOBAL_INCLUDE_H
#define __APP_GLOBAL_INCLUDE_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "app_type_define.h"

#include "nordic_common.h"
#include "ble_nus.h"
#include "sdk_errors.h"
#include "app_error.h"
#include "app_timer.h"
#include "app_uart.h"

/*FreeRTOS相关头文件*/
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

#include "nrf51.h"
#include "bsp.h"
#include "nrf_drv_clock.h"
#include "bsp_hardware.h"
#include "app_common.h"
#include "app_drv_led.h"
#include "app_drv_button.h"
#include "app_drv_pwm.h"
#include "app_drv_esb.h"
#include "app_drv_mpu6050.h"







#endif
