#ifndef __APP_COMMON_H
#define __APP_COMMON_H

#define NRF_LOG_MODULE_NAME "APP"
#include "app_type_define.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"




//数据拆分宏定义
#define  BYTE0(dwTemp)       ( *( (u8 *)(&dwTemp)   )  )
#define  BYTE1(dwTemp)       ( *( (u8 *)(&dwTemp) + 1) )
#define  BYTE2(dwTemp)       ( *( (u8 *)(&dwTemp) + 2) )
#define  BYTE3(dwTemp)       ( *( (u8 *)(&dwTemp) + 3) )



#define APP_TIMER_PRESCALER             0           /**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_OP_QUEUE_SIZE         4           /**< Size of timer operation queues. */



/*上行帧头*/
#define UP_BYTE1 0xAA
#define UP_BYTE2 0xAA

/*下行帧头*/
#define DOWN_BYTE1 0xAA
#define DOWN_BYTE2 0xAF



/*上行指令ID*/
typedef enum
{
    UP_VERSION      = 0x00,
    UP_STATUS       = 0x01,
    UP_SENSER       = 0x02,
    UP_RCDATA       = 0x03,
    UP_GPSDATA      = 0x04,
    UP_POWER        = 0x05,
    UP_MOTOR        = 0x06,
    UP_SENSER2      = 0x07,
    UP_FLYMODE      = 0x0A,
    UP_SPEED        = 0x0B,
    UP_PID1         = 0x10,
    UP_PID2         = 0x11,
    UP_PID3         = 0x12,
    UP_PID4         = 0x13,
    UP_PID5         = 0x14,
    UP_PID6         = 0x15,
    UP_RADIO        = 0x40,
    UP_MSG          = 0xEE,
    UP_CHECK        = 0xEF,

    UP_REMOTER      = 0x50,
    UP_PRINTF       = 0x51,
} upmsgID_e;


/*下行指令*/
#define  D_COMMAND_ACC_CALIB        0x01
#define  D_COMMAND_GYRO_CALIB       0x02
#define  D_COMMAND_MAG_CALIB        0x04
#define  D_COMMAND_BARO_CALIB       0x05
#define  D_COMMAND_FLIGHT_LOCK      0xA0
#define  D_COMMAND_FLIGHT_ULOCK     0xA1

#define  D_ACK_READ_PID             0x01
#define  D_ACK_READ_VERSION         0xA0
#define  D_ACK_RESET_PARAM          0xA1
/*下行指令ID*/
typedef enum
{
    DOWN_COMMAND    = 0x01,
    DOWN_ACK        = 0x02,
    DOWN_RCDATA     = 0x03,
    DOWN_POWER      = 0x05,
    DOWN_FLYMODE    = 0x0A,
    DOWN_PID1       = 0x10,
    DOWN_PID2       = 0x11,
    DOWN_PID3       = 0x12,
    DOWN_PID4       = 0x13,
    DOWN_PID5       = 0x14,
    DOWN_PID6       = 0x15,
    DOWN_RADIO      = 0x40,

    DOWN_REMOTER    = 0x50,
} downmsgID_e;






extern TaskHandle_t mainTaskHandle;
extern xQueueHandle  txQueue;
extern joystickFlyui16_t rcdata;
uint32_t logging_init( void );


#endif
