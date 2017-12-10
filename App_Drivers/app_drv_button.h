#ifndef __APP_DRV_BUTTON_H
#define __APP_DRV_BUTTON_H

#include "app_global_include.h"

typedef void (* bsp_evt_handler_t)( bsp_event_t evt );

void bsp_evt_handler2(bsp_event_t evt);
void app_drv_button_init(bsp_evt_handler_t bsp_evt_handler);


#endif
