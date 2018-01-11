#include "app_task_main.h"
#include "app_task_led.h"
#include "app_task_pwm.h"
#include "app_task_comm.h"
#include "app_task_comm_nrf.h"
#include "app_task_button.h"
#include "app_task_esb_rx.h"
#include "app_task_esb_tx.h"

#include "app_drv_mpu6050_sw.h"

//数据返回周期时间（单位ms）
#define  PERIOD_STATUS      30
#define  PERIOD_SENSOR      10
#define  PERIOD_RCDATA      40
#define  PERIOD_POWER       100
#define  PERIOD_MOTOR       40
#define  PERIOD_SENSOR2     40
#define  PERIOD_SPEED       50


static void app_time_slicing( void )
{
    static u16 count_ms = 1;
    if( !( count_ms % PERIOD_STATUS ) )
    {
        //      attitude_t attitude;
        //      getAttitudeData(&attitude);
        //      int positionZ = getPositionZ() * 100.f;
        //      sendStatus(attitude.roll, attitude.pitch, attitude.yaw, positionZ, 0, flyable);
        sendStatus(0, 0, 10, 0, 0, 0);
    }
    if( !( count_ms % PERIOD_SENSOR ) )
    {
          Axis3i16 acc;
          Axis3i16 gyro;
        //      Axis3i16 mag;
        mpu6050_read_sensor_data();
        getSensorRawData(&acc, &gyro);
        sendSenser(acc.x, acc.y, acc.z, gyro.x, gyro.y, gyro.z, 0, 0, 0);
    }
    if( !( count_ms % PERIOD_RCDATA ) )
    {
        //      sendRCData(rcdata.thrust, rcdata.yaw, rcdata.roll,
        //                  rcdata.pitch, 0, 0, 0, 0, 0, 0);
//        sendRCData(1800, 1600, 1800, 1600, 0, 0, 0, 0, 0, 0);
        sendRCData(rcdata.thrust, rcdata.yaw, rcdata.roll, rcdata.pitch, 0, 0, 0, 0, 0, 0);
    }
    if( !( count_ms % PERIOD_POWER ) )
    {
        //      float bat = pmGetBatteryVoltage();
        //      sendPower(bat*100,500);
    }
    if( !( count_ms % PERIOD_MOTOR ) )
    {
        //      u16 m1,m2,m3,m4;
        //      motorPWM_t motorPWM;
        //      getMotorPWM(&motorPWM);
        //      m1 = (float)motorPWM.m1/65535*1000;
        //      m2 = (float)motorPWM.m2/65535*1000;
        //      m3 = (float)motorPWM.m3/65535*1000;
        //      m4 = (float)motorPWM.m4/65535*1000;
        //      sendMotorPWM(m1,m2,m3,m4,0,0,0,0);
    }
    if( !( count_ms % PERIOD_SENSOR2 ) )
    {
        //      int baro = getBaroData() * 100.f;
        //      sendSenser2(baro,0);
    }
    if( ++count_ms >= 65535 )
    {
        count_ms = 1;
    }
}

void app_task_main( void* pvParameter )
{

    while( true )
    {
        app_time_slicing();
        /* Delay a task for a given number of ticks */
        vTaskDelay( 1 );
        /* Tasks must be implemented to never return... */
    }
}

