#include "app_task_button.h"
#include "app_global_include.h"


void bsp_evt_handler(bsp_event_t evt);

void app_task_button (void * pvParameter)
{
    app_drv_button_init(bsp_evt_handler);
    while(true)
    {
        vTaskDelay(10);
    }
}

void bsp_evt_handler(bsp_event_t evt)
{
    switch (evt)
    {
        case BSP_EVENT_KEY_0:
            NRF_LOG_DEBUG("test button0\n");
            break;

        case BSP_EVENT_KEY_1:
            NRF_LOG_DEBUG("test button1\n");
            break;
            
        case BSP_EVENT_KEY_2:
            NRF_LOG_DEBUG("test button2\n");
            break;

        case BSP_EVENT_KEY_3:
            NRF_LOG_DEBUG("test button3\n");
            break;

        default:
            return; // no implementation needed
    }

}


