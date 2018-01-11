#include "app_common.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"


TaskHandle_t mainTaskHandle;
xQueueHandle  txQueue;
joystickFlyui16_t rcdata;



uint32_t logging_init( void )
{
    uint32_t err_code;
    err_code = NRF_LOG_INIT( NULL );
    return err_code;
}




