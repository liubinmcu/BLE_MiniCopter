#include "app_task_led.h"
#include "app_global_include.h"



void app_task_led_blink (void * pvParameter)
{
    UNUSED_PARAMETER(pvParameter);
    app_drv_led_init();
    while (true)
    {
        app_drv_led_invert(APP_BSP_LED_0);

        /* Delay a task for a given number of ticks */
        vTaskDelay(400);

        /* Tasks must be implemented to never return... */
    }
}

