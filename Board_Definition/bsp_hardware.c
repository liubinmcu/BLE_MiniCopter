#include "bsp_hardware.h"
#include "app_global_include.h"

void bsp_hardware_init(void)
{
    uint32_t err_code;

    // Initialize.
    /* Initialize clock driver for better time accuracy in FREERTOS */
    err_code = nrf_drv_clock_init();
    APP_ERROR_CHECK(err_code);
    
    //初始化软件定时器模块，该定时器模块使用RTC1 来软件模拟定时器
    APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, false);
    app_drv_led_init();
    app_drv_pwm_init();
    app_drv_button_init();
}