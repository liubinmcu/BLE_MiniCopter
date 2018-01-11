/**
 * Copyright (c) 2014 - 2017, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


#include "app_global_include.h"
#include "app_task_main.h"
#include "app_drv_mpu6050_sw.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"


static void app_task_start(void)
{
//    xTaskCreate(app_task_led_blink, "Task_LED", configMINIMAL_STACK_SIZE + 200, NULL, configMAX_PRIORITIES - 2, NULL);
//    xTaskCreate(app_task_pwm, "Task_PWM", configMINIMAL_STACK_SIZE + 200, NULL, configMAX_PRIORITIES - 3, NULL);
//    xTaskCreate(app_task_comm, "Task_Comm", configMINIMAL_STACK_SIZE + 200, NULL, configMAX_PRIORITIES - 2, NULL);
//    xTaskCreate(app_task_button, "Task_Button", configMINIMAL_STACK_SIZE + 200, NULL, configMAX_PRIORITIES - 2, NULL);
//    xTaskCreate( app_task_comm_nrf, "Task_Comm_NRF", configMINIMAL_STACK_SIZE + 200, NULL, configMAX_PRIORITIES - 2, NULL );
//    xTaskCreate( app_task_esb_tx, "Task_ESB_TX", configMINIMAL_STACK_SIZE + 200, NULL, configMAX_PRIORITIES - 3, NULL );
//    xTaskCreate( app_task_esb_rx, "Task_ESB_RX", configMINIMAL_STACK_SIZE + 200, NULL, configMAX_PRIORITIES - 4, NULL );

}

/**
 * @brief Function for application main entry.
 */
int main( void )
{
    bsp_hardware_init();
    NRF_LOG_DEBUG( "Enhanced ShockBurst Receiver Example running.\r\n" );
//    app_task_start();
    xTaskCreate( app_task_main, "MainThread", configMINIMAL_STACK_SIZE + 200, NULL, 8, &mainTaskHandle );
    /* Activate deep sleep mode */
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
    /* Start FreeRTOS scheduler. */
    vTaskStartScheduler();
    // Enter main loop.
    for( ;; )
    {
        //      NRF_LOG_PROCESS();
//        MPU6050_ReadAcc( &tem1[0], &tem1[1] , &tem1[2] );
//        MPU6050_ReadGyro(&tem2[0] , &tem2[1] , &tem2[2] );
//		
//	NRF_LOG_DEBUG("ACC:  %d	%d	%d	",tem1[0],tem1[1],tem1[2]);
//	NRF_LOG_DEBUG("GYRO: %d	%d	%d	\r\n",tem2[0],tem2[1],tem2[2]);
    }
}

/**
 *@}
 **/
