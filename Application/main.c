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
#define NRF_LOG_MODULE_NAME "APP"
#include "app_task_main.h"
#include "app_global_include.h"
  
#include "nrf_esb.h"
#include "nrf_esb_error_codes.h"

   
static nrf_esb_payload_t tx_payload;
static nrf_esb_payload_t rx_payload;
static uint8_t m_state[4];
uint8_t led_nr;

void nrf_esb_event_handler(nrf_esb_evt_t const * p_event)
{
    switch (p_event->evt_id)
    {
        case NRF_ESB_EVENT_TX_SUCCESS:
            NRF_LOG_DEBUG("SUCCESS\r\n");
            break;
        case NRF_ESB_EVENT_TX_FAILED:
            NRF_LOG_DEBUG("FAILED\r\n");
            (void) nrf_esb_flush_tx();
            break;
        case NRF_ESB_EVENT_RX_RECEIVED:
            while (nrf_esb_read_rx_payload(&rx_payload) == NRF_SUCCESS) ;
            NRF_LOG_DEBUG("Receiving packet: %x\r\n", rx_payload.data[0]);

            switch (rx_payload.data[0] & 0xFUL)
            {
                case 0x1:
                    m_state[0] = !m_state[0];
                    nrf_gpio_pin_write(LED_1, m_state[0]);
                    break;

                case 0x2:
                    m_state[1] = !m_state[1];
                    nrf_gpio_pin_write(LED_2, m_state[1]);
                    break;

                case 0x4:
                    m_state[2] = !m_state[2];
                    nrf_gpio_pin_write(LED_3, m_state[2]);
                    break;

                case 0x8:
                    m_state[3] = !m_state[3];
                    nrf_gpio_pin_write(LED_4, m_state[3]);
                    break;
            }

            nrf_gpio_pin_write(LED_1, m_state[0]);
            nrf_gpio_pin_write(LED_2, m_state[1]);
            nrf_gpio_pin_write(LED_3, m_state[2]);
            nrf_gpio_pin_write(LED_4, m_state[3]);

            tx_payload.length = 1;
            tx_payload.data[0] = m_state[0] << 0
                               | m_state[1] << 1
                               | m_state[2] << 2
                               | m_state[3] << 3;
            (void) nrf_esb_write_payload(&tx_payload);

            NRF_LOG_DEBUG("Queue transmitt packet: %02x\r\n", tx_payload.data[0]);
            break;
    }
}

uint32_t logging_init( void )
{
    uint32_t err_code;
    err_code = NRF_LOG_INIT(NULL);
    return err_code;
}

void clocks_start( void )
{
    // Start HFCLK and wait for it to start.
    NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_HFCLKSTART = 1;
    while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);
}

uint32_t esb_init( void )
{
    uint32_t err_code;
    uint8_t base_addr_0[4] = {0x78, 0x56, 0x34, 0x12};
    uint8_t base_addr_1[4] = {0xC2, 0xC2, 0xC2, 0xC2};
    uint8_t addr_prefix[8] = {0x9A, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0x9A };
//    uint8_t base_addr_0[4] = {0x9A, 0x78, 0x56, 0x34};
//    uint8_t base_addr_1[4] = {0xC2, 0xC2, 0xC2, 0xC2};
//    uint8_t addr_prefix[8] = {0x12, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0x12 };

#ifndef NRF_ESB_LEGACY
    nrf_esb_config_t nrf_esb_config         = NRF_ESB_DEFAULT_CONFIG;
#else // NRF_ESB_LEGACY
    nrf_esb_config_t nrf_esb_config         = NRF_ESB_LEGACY_CONFIG;
#endif // NRF_ESB_LEGACY
    nrf_esb_config.selective_auto_ack       = 0;
    nrf_esb_config.payload_length           = 3;
    nrf_esb_config.bitrate                  = NRF_ESB_BITRATE_250KBPS;
    nrf_esb_config.mode                     = NRF_ESB_MODE_PRX;
    nrf_esb_config.event_handler            = nrf_esb_event_handler;

    err_code = nrf_esb_init(&nrf_esb_config);
    VERIFY_SUCCESS(err_code);

    err_code = nrf_esb_set_base_address_0(base_addr_0);
    VERIFY_SUCCESS(err_code);

    err_code = nrf_esb_set_base_address_1(base_addr_1);
    VERIFY_SUCCESS(err_code);

    err_code = nrf_esb_set_prefixes(addr_prefix, 8);
    VERIFY_SUCCESS(err_code);

    tx_payload.length = 1;

    return NRF_SUCCESS;
}

/**
 * @brief Function for application main entry.
 */
int main(void)
{
    uint32_t err_code;
    err_code = logging_init();
    APP_ERROR_CHECK(err_code);
    err_code = esb_init();
    APP_ERROR_CHECK(err_code);
    clocks_start();
    
    NRF_LOG_DEBUG("Enhanced ShockBurst Receiver Example running.\r\n");
    
    err_code = nrf_esb_start_rx();
    APP_ERROR_CHECK(err_code);
    bsp_hardware_init();

    xTaskCreate(app_task_main, "MainThread", configMINIMAL_STACK_SIZE + 200, NULL, configMAX_PRIORITIES - 1, NULL);

    /* Activate deep sleep mode */
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

    /* Start FreeRTOS scheduler. */
    vTaskStartScheduler();
    
    // Enter main loop.
    for (;;)
    {
      
    }

}

/**
 *@}
 **/
