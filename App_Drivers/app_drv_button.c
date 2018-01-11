#include "app_drv_button.h"
#include "app_global_include.h"

///**@brief Function for handling bsp events.
// */
//void bsp_evt_handler(bsp_event_t evt)
//{
//    switch (evt)
//    {
//        case BSP_EVENT_KEY_0:
//            printf("test button0\n");
//            break;
//
//        case BSP_EVENT_KEY_1:
//            printf("test button1\n");
//            break;
//
//        case BSP_EVENT_KEY_2:
//            printf("test button2\n");
//            break;
//
//        case BSP_EVENT_KEY_3:
//            printf("test button3\n");
//            break;
//
//        default:
//            return; // no implementation needed
//    }
//
//}


void app_drv_button_init( bsp_evt_handler_t bsp_evt_handler )
{
    uint32_t err_code = bsp_init( BSP_INIT_BUTTONS,
                                  APP_TIMER_TICKS( 100, APP_TIMER_PRESCALER ),
                                  bsp_evt_handler );
    APP_ERROR_CHECK( err_code );
}
