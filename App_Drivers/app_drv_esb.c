#include "app_drv_esb.h"

uint32_t esb_init( nrf_esb_event_handler_t nrf_esb_event_handler );

void app_drv_esp_init( nrf_esb_event_handler_t nrf_esb_event_handler )
{
    uint32_t err_code;
    err_code = esb_init( nrf_esb_event_handler );
    APP_ERROR_CHECK( err_code );
    err_code = nrf_esb_start_rx();
    APP_ERROR_CHECK( err_code );
}

uint32_t esb_init( nrf_esb_event_handler_t nrf_esb_event_handler )
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
    err_code = nrf_esb_init( &nrf_esb_config );
    VERIFY_SUCCESS( err_code );
    err_code = nrf_esb_set_base_address_0( base_addr_0 );
    VERIFY_SUCCESS( err_code );
    err_code = nrf_esb_set_base_address_1( base_addr_1 );
    VERIFY_SUCCESS( err_code );
    err_code = nrf_esb_set_prefixes( addr_prefix, 8 );
    VERIFY_SUCCESS( err_code );
    //    tx_payload.length = 1;
    return NRF_SUCCESS;
}



