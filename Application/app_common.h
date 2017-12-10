#ifndef __APP_COMMON_H
#define __APP_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"


#define NRF_LOG_MODULE_NAME "APP"

#define APP_TIMER_PRESCALER             0           /**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_OP_QUEUE_SIZE         4           /**< Size of timer operation queues. */

extern TaskHandle_t mainTaskHandle;
uint32_t logging_init( void );


#endif
