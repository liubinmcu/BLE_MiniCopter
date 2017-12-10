#include "bsp_hardware.h"
#include "app_global_include.h"

void clocks_start( void )
{
    // Start HFCLK and wait for it to start.
    NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_HFCLKSTART = 1;
    while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);
}

void bsp_hardware_init(void)
{
    uint32_t err_code;
    
    clocks_start();
    // Initialize.
    /* Initialize clock driver for better time accuracy in FREERTOS */
    err_code = nrf_drv_clock_init();
    APP_ERROR_CHECK(err_code);
    
    err_code = logging_init();
    APP_ERROR_CHECK(err_code);
    
    //��ʼ�������ʱ��ģ�飬�ö�ʱ��ģ��ʹ��RTC1 �����ģ�ⶨʱ��
    APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, false);
//    app_drv_led_init();
//    app_drv_pwm_init();
//    app_drv_button_init();
}