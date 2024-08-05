/*
 * Copyright (c) 2021, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ti_msp_dl_config.h"
#include "led.h"
#include "board.h"
#include "analysis_data.h"
uint8_t g_uart_rx_buf[67];  /*rx DMA buffer of uart2*/
uint16_t g_uart_rx_cnt = 0; /*reciede data length of uart2*/

uint8_t g_decode_data[67];  /*buffer for decoding*/
uint16_t g_decode_data_pos = 0; /*bytes left in decode buffer*/

int32_t Get_Encoder_countA,encoderA_cnt,PWMA,Get_Encoder_countB,encoderB_cnt,PWMB;
uint8_t INS_Count = 0,INS_flag=0;
extern uint8_t PID_Send;
int Motor_A,Motor_B,Target_A,Target_B;           //电机舵机控制相关
float Velocity=15,Turn;
float Velocity_KP=0.037,Velocity_KI=0.007;         //速度控制PID参数
payload_data_t *payload = NULL;
int main(void)
{
    SYSCFG_DL_init();
    NVIC_ClearPendingIRQ(UART_0_INST_INT_IRQN);//清楚中断标志位
    NVIC_ClearPendingIRQ(TIMER_0_INST_INT_IRQN);
    //使能串口中断
    NVIC_EnableIRQ(UART_1_INST_INT_IRQN);//开启中断
    NVIC_EnableIRQ(TIMER_0_INST_INT_IRQN);
    while (1) {

        printf("Roll:%d Pitch:%d yaw:%d\n\r",(int)(g_output_info.roll),(int)(g_output_info.pitch),(int)(g_output_info.yaw));
        delay_ms(5);
    }
}


void TIMER_0_INST_IRQHandler(void)
{
    if(DL_TimerA_getPendingInterrupt(TIMER_0_INST))
    {
        if(DL_TIMER_IIDX_ZERO)
        {

                LED_Flash(100);//LED1闪烁
        }

    }
}




