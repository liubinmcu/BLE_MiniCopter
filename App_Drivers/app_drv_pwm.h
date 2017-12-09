#ifndef __APP_DRV_PWM_H
#define __APP_DRV_PWM_H

#include "app_global_include.h"

#define MOTORS_NUMBER    4


#define APP_BSP_MOTOR_0          10
#define APP_BSP_MOTOR_1          11
#define APP_BSP_MOTOR_2          12
#define APP_BSP_MOTOR_3          13



void app_drv_pwm_init(void);
void app_drv_pwm_set_value(uint8_t value1, uint8_t value2, uint8_t value3, uint8_t value4);




#endif
