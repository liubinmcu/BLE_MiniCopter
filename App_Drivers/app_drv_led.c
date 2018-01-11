#include "app_drv_led.h"
#include "nrf_gpio.h"


void app_drv_led_init( void )
{
    app_drv_led_gpio_output( APP_BSP_LED_0 );
    app_drv_led_gpio_output( APP_BSP_LED_1 );
    app_drv_led_gpio_output( APP_BSP_LED_2 );
    app_drv_led_gpio_output( APP_BSP_LED_3 );
    app_drv_led_off( APP_BSP_LED_0 );
    app_drv_led_off( APP_BSP_LED_1 );
    app_drv_led_off( APP_BSP_LED_2 );
    app_drv_led_off( APP_BSP_LED_3 );
//    app_drv_led_on( APP_BSP_LED_0 );
//    app_drv_led_on( APP_BSP_LED_1 );
}

bool app_drv_led_state_get( uint32_t led_num )
{
    bool pin_set = nrf_gpio_pin_out_read( led_num ) ? true : false;
    return ( pin_set == ( APP_LEDS_ACTIVE_STATE ? true : false ) );
}

void app_drv_led_gpio_output( uint32_t led_num )
{
    nrf_gpio_cfg_output( led_num );
}

void app_drv_led_on( uint32_t led_num )
{
    nrf_gpio_pin_write( led_num, APP_LEDS_ACTIVE_STATE ? 1 : 0 );
}

void app_drv_led_off( uint32_t led_num )
{
    nrf_gpio_pin_write( led_num, APP_LEDS_ACTIVE_STATE ? 0 : 1 );
}

void app_drv_led_invert( uint32_t led_num )
{
    nrf_gpio_pin_toggle( led_num );
}


