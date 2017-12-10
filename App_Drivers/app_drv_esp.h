#ifndef __APP_DRV_ESP_H
#define __APP_DRV_ESP_H

#include "nrf_esb.h"
#include "nrf_esb_error_codes.h"
#include "app_global_include.h"

//typedef void (* nrf_esb_event_handler_t)(nrf_esb_evt_t const * p_event);


void app_drv_esp_init( nrf_esb_event_handler_t nrf_esb_event_handler );




#endif /* __APP_DRV_ESP_H */
