#include "app_task_esb_tx.h"
#include "app_task_comm_nrf.h"


//数据返回周期时间（单位ms）
#define  PERIOD_STATUS      30
#define  PERIOD_SENSOR      10
#define  PERIOD_RCDATA      40
#define  PERIOD_POWER       100
#define  PERIOD_MOTOR       40
#define  PERIOD_SENSOR2     40
#define  PERIOD_SPEED       50

#define RADIOLINK_TX_QUEUE_SIZE  30 /*接收队列个数*/

void app_task_esb_tx_init( void )
{
    /*创建发送队列，CRTP_TX_QUEUE_SIZE个消息*/
    txQueue = xQueueCreate( RADIOLINK_TX_QUEUE_SIZE, sizeof( atkp_t ) );
    ASSERT( txQueue );
}

void radiolinkSendPacket( const atkp_t* p )
{
    ASSERT( p );
    ASSERT( p->dataLen <= ATKP_MAX_DATA_SIZE );
    nrf_esb_sendpacket(p);
}

/***************************发送至匿名上位机指令******************************/
void sendStatus(float roll, float pitch, float yaw, s32 alt, u8 fly_model, u8 armed)
{
	u8 _cnt=0;
	atkp_t p;
	vs16 _temp;
	vs32 _temp2 = alt;
	
	p.msgID = UP_STATUS;
	
	_temp = (int)(roll*100);
	p.data[_cnt++]=BYTE1(_temp);
	p.data[_cnt++]=BYTE0(_temp);
	_temp = (int)(pitch*100);
	p.data[_cnt++]=BYTE1(_temp);
	p.data[_cnt++]=BYTE0(_temp);
	_temp = (int)(yaw*100);
	p.data[_cnt++]=BYTE1(_temp);
	p.data[_cnt++]=BYTE0(_temp);
	
	p.data[_cnt++]=BYTE3(_temp2);
	p.data[_cnt++]=BYTE2(_temp2);
	p.data[_cnt++]=BYTE1(_temp2);
	p.data[_cnt++]=BYTE0(_temp2);
	
	p.data[_cnt++] = fly_model;
	p.data[_cnt++] = armed;
	
	p.dataLen = _cnt;
    radiolinkSendPacket( &p );
//	atkpSendPacket(&p);
}

void sendSenser( s16 a_x, s16 a_y, s16 a_z, s16 g_x, s16 g_y, s16 g_z, s16 m_x, s16 m_y, s16 m_z )
{
    u8 _cnt = 0;
    atkp_t p;
    vs16 _temp;
    p.msgID = UP_SENSER;
    _temp = a_x;
    p.data[_cnt++] = BYTE1( _temp );
    p.data[_cnt++] = BYTE0( _temp );
    _temp = a_y;
    p.data[_cnt++] = BYTE1( _temp );
    p.data[_cnt++] = BYTE0( _temp );
    _temp = a_z;
    p.data[_cnt++] = BYTE1( _temp );
    p.data[_cnt++] = BYTE0( _temp );
    _temp = g_x;
    p.data[_cnt++] = BYTE1( _temp );
    p.data[_cnt++] = BYTE0( _temp );
    _temp = g_y;
    p.data[_cnt++] = BYTE1( _temp );
    p.data[_cnt++] = BYTE0( _temp );
    _temp = g_z;
    p.data[_cnt++] = BYTE1( _temp );
    p.data[_cnt++] = BYTE0( _temp );
    _temp = m_x;
    p.data[_cnt++] = BYTE1( _temp );
    p.data[_cnt++] = BYTE0( _temp );
    _temp = m_y;
    p.data[_cnt++] = BYTE1( _temp );
    p.data[_cnt++] = BYTE0( _temp );
    _temp = m_z;
    p.data[_cnt++] = BYTE1( _temp );
    p.data[_cnt++] = BYTE0( _temp );
    _temp = 0;
    p.data[_cnt++] = BYTE1( _temp );
    p.data[_cnt++] = BYTE0( _temp );
    p.dataLen = _cnt;
    radiolinkSendPacket( &p );
    //  atkpSendPacket(&p);
}

void sendRCData(u16 thrust,u16 yaw,u16 roll,u16 pitch,u16 aux1,u16 aux2,u16 aux3,u16 aux4,u16 aux5,u16 aux6)
{
	u8 _cnt=0;
	atkp_t p;
	
	p.msgID = UP_RCDATA;
	p.data[_cnt++]=BYTE1(thrust);
	p.data[_cnt++]=BYTE0(thrust);
	p.data[_cnt++]=BYTE1(yaw);
	p.data[_cnt++]=BYTE0(yaw);
	p.data[_cnt++]=BYTE1(roll);
	p.data[_cnt++]=BYTE0(roll);
	p.data[_cnt++]=BYTE1(pitch);
	p.data[_cnt++]=BYTE0(pitch);
	p.data[_cnt++]=BYTE1(aux1);
	p.data[_cnt++]=BYTE0(aux1);
	p.data[_cnt++]=BYTE1(aux2);
	p.data[_cnt++]=BYTE0(aux2);
	p.data[_cnt++]=BYTE1(aux3);
	p.data[_cnt++]=BYTE0(aux3);
	p.data[_cnt++]=BYTE1(aux4);
	p.data[_cnt++]=BYTE0(aux4);
	p.data[_cnt++]=BYTE1(aux5);
	p.data[_cnt++]=BYTE0(aux5);
	p.data[_cnt++]=BYTE1(aux6);
	p.data[_cnt++]=BYTE0(aux6);

	p.dataLen = _cnt;
    radiolinkSendPacket( &p );
//	atkpSendPacket(&p);
}



/*数据周期性发送给上位机，每1ms调用一次*/
static void atkpSendPeriod( void )
{
    static u16 count_ms = 1;
    if( !( count_ms % PERIOD_STATUS ) )
    {
        //      attitude_t attitude;
        //      getAttitudeData(&attitude);
        //      int positionZ = getPositionZ() * 100.f;
        //      sendStatus(attitude.roll, attitude.pitch, attitude.yaw, positionZ, 0, flyable);
    }
    if( !( count_ms % PERIOD_SENSOR ) )
    {
        //      Axis3i16 acc;
        //      Axis3i16 gyro;
        //      Axis3i16 mag;
        //      getSensorRawData(&acc, &gyro, &mag);
    }
    if( !( count_ms % PERIOD_RCDATA ) )
    {
        //      sendRCData(rcdata.thrust, rcdata.yaw, rcdata.roll,
        //                  rcdata.pitch, 0, 0, 0, 0, 0, 0);
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


void app_task_esb_tx( void* pvParameter )
{
    app_task_esb_tx_init();
    while( true )
    {
        atkpSendPeriod();
        vTaskDelay( 1 );
    }
}



