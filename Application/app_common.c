#include "app_common.h"
#include "app_global_include.h"

TaskHandle_t mainTaskHandle;


uint32_t logging_init( void )
{
    uint32_t err_code;
    err_code = NRF_LOG_INIT(NULL);
    return err_code;
}




