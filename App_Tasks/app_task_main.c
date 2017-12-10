#include "app_task_main.h"
#include "app_global_include.h"

#include "app_task_led.h"
#include "app_task_pwm.h"
#include "app_task_comm.h"


void app_task_main (void * pvParameter)
{
//    xTaskCreate(app_task_led_blink, "Task_LED", configMINIMAL_STACK_SIZE + 200, NULL, configMAX_PRIORITIES - 2, NULL);
//    xTaskCreate(app_task_pwm, "Task_PWM", configMINIMAL_STACK_SIZE + 200, NULL, configMAX_PRIORITIES - 2, NULL);
//    xTaskCreate(app_task_comm, "Task_Comm", configMINIMAL_STACK_SIZE + 200, NULL, configMAX_PRIORITIES - 2, NULL);

    while(true)
    {
        vTaskDelay(100);
    }
}

