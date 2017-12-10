#include "app_task_main.h"
#include "app_global_include.h"

#include "app_task_led.h"
#include "app_task_pwm.h"
#include "app_task_comm.h"
#include "app_task_comm_nrf.h"
#include "app_task_button.h"

void app_task_main (void * pvParameter)
{
    taskENTER_CRITICAL();	/*进入临界区*/
//    xTaskCreate(app_task_led_blink, "Task_LED", configMINIMAL_STACK_SIZE + 200, NULL, configMAX_PRIORITIES - 2, NULL);
//    xTaskCreate(app_task_pwm, "Task_PWM", configMINIMAL_STACK_SIZE + 200, NULL, configMAX_PRIORITIES - 2, NULL);
//    xTaskCreate(app_task_comm, "Task_Comm", configMINIMAL_STACK_SIZE + 200, NULL, configMAX_PRIORITIES - 2, NULL);
    xTaskCreate(app_task_comm_nrf, "Task_Comm_NRF", configMINIMAL_STACK_SIZE + 200, NULL, configMAX_PRIORITIES - 2, NULL);
//    xTaskCreate(app_task_button, "Task_Button", configMINIMAL_STACK_SIZE + 200, NULL, configMAX_PRIORITIES - 2, NULL);

    vTaskDelete(mainTaskHandle);
    taskEXIT_CRITICAL();	/*退出临界区*/
}

