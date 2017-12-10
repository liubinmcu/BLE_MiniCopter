#include "app_task_comm_nrf.h"
#include "app_drv_esp.h"

static nrf_esb_payload_t tx_payload;
static nrf_esb_payload_t rx_payload;
static uint8_t m_state[4];

void nrf_comm_event_handler(nrf_esb_evt_t const * p_event);

void app_task_comm_nrf (void * pvParameter)
{
    app_drv_esp_init(nrf_comm_event_handler);
    
    while(true)
    {
        vTaskDelay(10);
    }
}

void nrf_comm_event_handler(nrf_esb_evt_t const * p_event)
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