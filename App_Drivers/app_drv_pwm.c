#include "app_drv_pwm.h"
#include "app_error.h"
#include "app_pwm.h"
#include "boards.h"

APP_PWM_INSTANCE(PWM1,1);                   // Create the instance "PWM1" using TIMER1.
APP_PWM_INSTANCE(PWM2,2);                   // Create the instance "PWM1" using TIMER1.

static volatile bool ready_flag;            // A flag indicating PWM status.

__STATIC_INLINE void pwm_ready_callback(uint32_t pwm_id)    // PWM callback function
{
    ready_flag = true;
}

static volatile bool ready_flag2;            // A flag indicating PWM status.

__STATIC_INLINE void pwm_ready_callback2(uint32_t pwm_id)    // PWM callback function
{
    ready_flag2 = true;
}

void app_drv_pwm_init(void)
{
    ret_code_t err_code;

    /* 2-channel PWM, 200Hz, output on DK LED pins. */
    app_pwm_config_t pwm1_cfg = APP_PWM_DEFAULT_CONFIG_2CH(5000L, BSP_LED_0, BSP_LED_1);

    /* Initialize and enable PWM. */
    err_code = app_pwm_init(&PWM1,&pwm1_cfg,pwm_ready_callback);
    APP_ERROR_CHECK(err_code);
    app_pwm_enable(&PWM1);
    
    /* 2-channel PWM, 200Hz, output on DK LED pins. */
    app_pwm_config_t pwm2_cfg = APP_PWM_DEFAULT_CONFIG_2CH(5000L, BSP_LED_2, BSP_LED_3);

    /* Initialize and enable PWM. */
    err_code = app_pwm_init(&PWM2,&pwm2_cfg,pwm_ready_callback2);
    APP_ERROR_CHECK(err_code);
    app_pwm_enable(&PWM2);
}

void app_drv_pwm_set_value(uint8_t value1, uint8_t value2, uint8_t value3, uint8_t value4)
{
    while (app_pwm_channel_duty_set(&PWM1, 0, value1) == NRF_ERROR_BUSY);
    while (app_pwm_channel_duty_set(&PWM1, 1, value2) == NRF_ERROR_BUSY);

    while (app_pwm_channel_duty_set(&PWM2, 0, value3) == NRF_ERROR_BUSY);
    while (app_pwm_channel_duty_set(&PWM2, 1, value4) == NRF_ERROR_BUSY);
}



