#include "app_task_comm_nrf.h"
#include "app_task_esb_rx.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"

static nrf_esb_payload_t tx_payload;
static nrf_esb_payload_t rx_payload;
static atkp_t txPacket;
static atkp_t rxPacket;
//static uint8_t m_state[4];

void nrf_comm_event_handler( nrf_esb_evt_t const* p_event );

void nrf_esb_sendpacket( const atkp_t* txPkt )
{
    tx_payload.length = txPkt->dataLen + 2;
    memcpy( tx_payload.data, txPkt, tx_payload.length );
    nrf_esb_write_payload( &tx_payload );
}

/*radiolink接收到ATKPPacket预处理*/
static void atkpPacketDispatch( atkp_t* rxPkt )
{
    //  atkpReceivePacketBlocking(rxPacket);
    if( rxPkt->msgID == DOWN_POWER )
    {;}/*do noting*/
    else
    {
        /*接收到一个遥控无线数据包则发送一个包*/
        if( xQueueReceive( txQueue, &txPacket, 0 ) == pdTRUE )
        {
            ASSERT( txPacket.dataLen <= ATKP_MAX_DATA_SIZE );
            nrf_esb_sendpacket( &txPacket );
            //          uartSendPacket(&txPacket);
        }
    }
}

void app_task_comm_nrf( void* pvParameter )
{
    app_drv_esp_init( nrf_comm_event_handler );
    //    while( true )
    //    {
    //        vTaskDelay( 50 );
    //    }
}

void nrf_comm_event_handler( nrf_esb_evt_t const* p_event )
{
    switch( p_event->evt_id )
    {
        case NRF_ESB_EVENT_TX_SUCCESS:
            NRF_LOG_DEBUG( "SUCCESS\r\n" );
            break;
        case NRF_ESB_EVENT_TX_FAILED:
            NRF_LOG_DEBUG( "FAILED\r\n" );
            ( void ) nrf_esb_flush_tx();
            break;
        case NRF_ESB_EVENT_RX_RECEIVED:
            while( nrf_esb_read_rx_payload( &rx_payload ) == NRF_SUCCESS ) ;
            NRF_LOG_DEBUG( "Receiving packet: %x\r\n", rx_payload.data[0] );
            rxPacket.msgID = rx_payload.data[0];
            rxPacket.dataLen = rx_payload.data[1];
            memcpy( rxPacket.data, &rx_payload.data[2], rxPacket.dataLen );
            app_data_analysis(&rxPacket);
            //            nrf_esb_sendpacket( &txPacket );
            //            atkpPacketDispatch( &rxPacket );
//            tx_payload.length = 1;
//            tx_payload.data[0] = 0x68;
//            ( void ) nrf_esb_write_payload( &tx_payload );
//            NRF_LOG_DEBUG( "Queue transmitt packet: %02x\r\n", tx_payload.data[0] );
            break;
    }
}