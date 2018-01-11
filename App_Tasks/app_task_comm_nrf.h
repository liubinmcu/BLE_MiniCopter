#ifndef __APP_TASK_COMM_NRF_H
#define __APP_TASK_CMMM_NRF_H

#include "app_global_include.h"

void app_task_comm_nrf (void * pvParameter);
void nrf_esb_sendpacket( const atkp_t* txPkt );


#endif /* __APP_TASK_COMM_NRF_H */
