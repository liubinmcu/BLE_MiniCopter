#ifndef __APP_DRV_LED_H
#define __APP_DRV_LED_H

#include "app_global_include.h"

// LEDs definitions for PCA10028
#define LEDS_NUMBER    4


#define APP_BSP_LED_0          21
#define APP_BSP_LED_1          22
#define APP_BSP_LED_2          23
#define APP_BSP_LED_3          24

#define LEDS_ACTIVE_STATE 0

void app_drv_led_init(void);
void app_drv_led_gpio_output(uint32_t led_num);
bool app_drv_led_state_get(uint32_t led_num);
void app_drv_led_on(uint32_t led_num);
void app_drv_led_off(uint32_t led_num);
void app_drv_led_invert(uint32_t led_num);



#endif
