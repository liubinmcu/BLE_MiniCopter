#include "app_task_esb_rx.h"
#include "app_global_include.h"

void app_data_analysis(atkp_t* rxPkt)
{
    if(rxPkt->msgID == DOWN_COMMAND)
    {
        
    }
    else if(rxPkt->msgID == DOWN_ACK)
    {

    }
    else if(rxPkt->msgID == DOWN_RCDATA)
    {
        rcdata = *((joystickFlyui16_t*)rxPkt->data);
    }
    else if(rxPkt->msgID == DOWN_POWER)
    {

    }
    else if(rxPkt->msgID == DOWN_REMOTER)
    {

    }
    else if(rxPkt->msgID == DOWN_PID1)
    {

    }
    else if(rxPkt->msgID == DOWN_PID2)
    {

    }
    else if(rxPkt->msgID == DOWN_PID3)
    {

    }
    else if(rxPkt->msgID == DOWN_PID4)
    {

    }
    else if(rxPkt->msgID == DOWN_PID5)
    {

    }
    else if(rxPkt->msgID == DOWN_PID6)
    {

    }
}

void app_task_esb_rx( void* pvParameter )
{
    while(true)
    {
        vTaskDelay(10);
    }
}