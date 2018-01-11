/**
 * Copyright (c) 2009 - 2017, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdbool.h>
#include <stdint.h>

#include "twi_master.h"
#include "app_drv_mpu6050_sw.h"
#include "app_drv_mpu60x0_register_map.h"
#include "app_type_define.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
/*lint ++flb "Enter library region" */

#define ADDRESS_WHO_AM_I          (0x75U) // !< WHO_AM_I register identifies the device. Expected value is 0x68.
#define ADDRESS_SIGNAL_PATH_RESET (0x68U) // !<
#define MPU6050_GYRO_OUT        0x43
#define MPU6050_ACC_OUT         0x3B

#define SENSORS_MPU6500_BUFF_LEN    14

static const uint8_t expected_who_am_i = 0x68U; // !< Expected value to get from WHO_AM_I register.
static uint8_t       m_device_address;          // !< Device address in bits [7:1]
static uint8_t mpu_sensor_buff[SENSORS_MPU6500_BUFF_LEN] = {0};
static Axis3i16 gyroRaw;
static Axis3i16 accRaw;


bool mpu6050_init( uint8_t device_address )
{
    bool transfer_succeeded = true;
    bool ret = twi_master_init();
    if( !ret )
    {
        NRF_LOG_DEBUG( "TWI Init Fail\r\n" );
    }
    else
    {
        NRF_LOG_DEBUG( "TWI Init Success!\r\n" );
    }
    m_device_address = ( uint8_t )( device_address << 1 );
    transfer_succeeded &= mpu6050_register_write( MPU_REG_PWR_MGMT_1, 0x00 );
    transfer_succeeded &= mpu6050_register_write( MPU_REG_SMPLRT_DIV, 0x00 );
    transfer_succeeded &= mpu6050_register_write( MPU_REG_CONFIG, 0x02 );
    transfer_succeeded &= mpu6050_register_write( MPU_REG_GYRO_CONFIG, 0x18 );
    transfer_succeeded &= mpu6050_register_write( MPU_REG_ACCEL_CONFIG, 0x18 );
    // Read and verify product ID
    transfer_succeeded &= mpu6050_verify_product_id();
    return transfer_succeeded;
}

bool mpu6050_verify_product_id( void )
{
    uint8_t who_am_i = 0;
    if( mpu6050_register_read( ADDRESS_WHO_AM_I, &who_am_i, 1 ) )
    {
        NRF_LOG_DEBUG( "wo_am_i = %x.\r\n", who_am_i );
        if( who_am_i != expected_who_am_i )
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    else
    {
        return false;
    }
}

bool mpu6050_register_write( uint8_t register_address, uint8_t value )
{
    uint8_t w2_data[2];
    w2_data[0] = register_address;
    w2_data[1] = value;
    return twi_master_transfer( m_device_address, w2_data, 2, TWI_ISSUE_STOP );
}

bool mpu6050_register_read( uint8_t register_address, uint8_t* destination, uint8_t number_of_bytes )
{
    bool transfer_succeeded;
    transfer_succeeded  = twi_master_transfer( m_device_address, &register_address, 1, TWI_DONT_ISSUE_STOP );
    transfer_succeeded &= twi_master_transfer( m_device_address | TWI_READ_BIT, destination, number_of_bytes, TWI_ISSUE_STOP );
    return transfer_succeeded;
}

void mpu6050_read_sensor_data(void)
{
    mpu6050_register_read(MPU_REG_ACCEL_XOUT_H, mpu_sensor_buff, SENSORS_MPU6500_BUFF_LEN);
    
    /*注意传感器读取方向(旋转270°x和y交换)*/
    int16_t ay = (((int16_t) mpu_sensor_buff[0]) << 8) | mpu_sensor_buff[1];
    int16_t ax = (((int16_t) mpu_sensor_buff[2]) << 8) | mpu_sensor_buff[3];
    int16_t az = (((int16_t) mpu_sensor_buff[4]) << 8) | mpu_sensor_buff[5];
    int16_t gy = (((int16_t) mpu_sensor_buff[8]) << 8) | mpu_sensor_buff[9];
    int16_t gx = (((int16_t) mpu_sensor_buff[10]) << 8) | mpu_sensor_buff[11];
    int16_t gz = (((int16_t) mpu_sensor_buff[12]) << 8) | mpu_sensor_buff[13];

    accRaw.x = ax;/*用于上传到上位机*/
    accRaw.y = ay;
    accRaw.z = az;
    gyroRaw.x = gx;
    gyroRaw.y = gy;
    gyroRaw.z = gz;
}

/*上位机获取读取原始数据*/
void getSensorRawData(Axis3i16* acc, Axis3i16* gyro)
{
	*acc = accRaw;
	*gyro = gyroRaw;
}


/*lint --flb "Leave library region" */
