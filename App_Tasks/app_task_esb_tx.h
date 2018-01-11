#ifndef __APP_TASK_ESB_TX_H
#define __APP_TASK_ESB_TX_H

#include "app_global_include.h"




void app_task_esb_tx( void * pvParameter);
void sendStatus(float roll, float pitch, float yaw, s32 alt, u8 fly_model, u8 armed);
void sendSenser( s16 a_x, s16 a_y, s16 a_z, s16 g_x, s16 g_y, s16 g_z, s16 m_x, s16 m_y, s16 m_z );
void sendRCData(u16 thrust,u16 yaw,u16 roll,u16 pitch,u16 aux1,u16 aux2,u16 aux3,u16 aux4,u16 aux5,u16 aux6);



#endif /* __APP_TASK_ESB_TX_H */
