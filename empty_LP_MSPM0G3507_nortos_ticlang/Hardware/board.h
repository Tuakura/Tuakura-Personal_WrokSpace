#ifndef __BOARD_H__
#define __BOARD_H__

#include "ti_msp_dl_config.h"
#include "math.h"

#define ABS(a)      (a>0 ? a:(-a))

//FDlink candata
#define FRAME_HEAD 0xfc
#define FRAME_END 0xfd
#define TYPE_IMU 0x40
#define TYPE_AHRS 0x41
#define TYPE_INSGPS 0x42
#define TYPE_GROUND 0xf0
#define IMU_LEN  0x38   //56+8  8组数据
#define AHRS_LEN 0x30   //48+8  7组数据
#define INSGPS_LEN 0x42 //72+8  10组数据
#define IMU_CAN 9
#define AHRS_CAN 8
#define INSGPS_CAN 11

#define FRAME_HEADER      0X7B //Frame_header //??
#define FRAME_TAIL        0X7D //Frame_tail   //?β
#define SEND_DATA_SIZE    24
#define RECEIVE_DATA_SIZE 11
#define IMU_RS 64
#define AHRS_RS 56
#define INSGPS_RS 80

typedef struct IMUData_Packet_t{
        float gyroscope_x;          //unit: rad/s
        float gyroscope_y;          //unit: rad/s
        float gyroscope_z;          //unit: rad/s
        float accelerometer_x;      //m/s^2
        float accelerometer_y;      //m/s^2
        float accelerometer_z;      //m/s^2
        float magnetometer_x;       //mG
        float magnetometer_y;       //mG
        float magnetometer_z;       //mG
        float imu_temperature;      //C
        float Pressure;             //Pa
        float pressure_temperature; //C
        uint32_t Timestamp;          //us
} IMUData_Packet_t;

typedef struct AHRSData_Packet_t
{
    float RollSpeed;   //unit: rad/s
    float PitchSpeed;  //unit: rad/s
    float HeadingSpeed;//unit: rad/s
    float Roll;        //unit: rad
    float Pitch;       //unit: rad
    float Heading;     //unit: rad
    float Qw;//w          //Quaternion
    float Qx;//x
    float Qy;//y
    float Qz;//z
    uint32_t Timestamp; //unit: us
}AHRSData_Packet_t;


extern int32_t Get_Encoder_countA,Get_Encoder_countB;


void delay_us(unsigned long __us);
void delay_ms(unsigned long ms);
void delay_1us(unsigned long __us);
void delay_1ms(unsigned long ms);

void uart0_send_char(char ch);
void uart0_send_string(char* str);
uint8_t TTL_Hex2Dec(void);


#endif
