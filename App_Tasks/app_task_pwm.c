#include "app_task_pwm.h"
#include "app_global_include.h"


void app_task_pwm( void* pvParameter )
{
    static uint32_t value = 0;
    app_drv_pwm_init();
    while( true )
    {
        /* Set the duty cycle - keep trying until PWM is ready... */
        for( uint8_t i = 0; i < 40; ++i )
        {
            value = ( i < 20 ) ? ( i * 5 ) : ( 100 - ( i - 20 ) * 5 );
            app_drv_pwm_set_value( value, value, value, value );
            vTaskDelay( 30 );
        }
    }
}

