#include "board.h"
#include "stdio.h"
#include "string.h"
#include "analysis_data.h"
#define RE_0_BUFF_LEN_MAX	128

extern uint8_t g_uart_rx_buf[127];  /*rx DMA buffer of uart2*/
extern uint16_t g_uart_rx_cnt; /*reciede data length of uart2*/

extern payload_data_t *payload;

extern uint8_t g_decode_data[127];  /*buffer for decoding*/
extern uint16_t g_decode_data_pos;  /*bytes left in decode buffer*/

volatile uint8_t  recv0_buff[RE_0_BUFF_LEN_MAX] = {0};
volatile uint16_t recv0_length = 0;
volatile uint8_t  recv0_flag = 0;

IMUData_Packet_t IMUData_Packet;
AHRSData_Packet_t AHRSData_Packet;
uint8_t PID_Send;
uint8_t Fd_data[64];
uint8_t Fd_rsimu[64];
uint8_t Fd_rsahrs[56];
int rs_ahrstype =0;
int rs_imutype =0;

//搭配滴答定时器实现的精确us延时
void delay_us(unsigned long __us) 
{
    uint32_t ticks;
    uint32_t told, tnow, tcnt = 38;

    // 计算需要的时钟数 = 延迟微秒数 * 每微秒的时钟数
    ticks = __us * (32000000 / 1000000);

    // 获取当前的SysTick值
    told = SysTick->VAL;

    while (1)
    {
        // 重复刷新获取当前的SysTick值
        tnow = SysTick->VAL;

        if (tnow != told)
        {
            if (tnow < told)
                tcnt += told - tnow;
            else
                tcnt += SysTick->LOAD - tnow + told;

            told = tnow;

            // 如果达到了需要的时钟数，就退出循环
            if (tcnt >= ticks)
                break;
        }
    }
}
//搭配滴答定时器实现的精确ms延时
void delay_ms(unsigned long ms) 
{
	delay_us( ms * 1000 );
}

void delay_1us(unsigned long __us){ delay_us(__us); }
void delay_1ms(unsigned long ms){ delay_ms(ms); }

//串口发送单个字符
void uart0_send_char(char ch)
{
	//当串口0忙的时候等待，不忙的时候再发送传进来的字符
	while( DL_UART_isBusy(UART_0_INST) == true );
	//发送单个字符
	DL_UART_Main_transmitData(UART_0_INST, ch);

}
//串口发送字符串
void uart0_send_string(char* str)
{
	//当前字符串地址不在结尾 并且 字符串首地址不为空
	while(*str!=0&&str!=0)
	{
		//发送字符串首地址中的字符，并且在发送完成之后首地址自增
		uart0_send_char(*str++);
	}
}


#if !defined(__MICROLIB)
//不使用微库的话就需要添加下面的函数
#if (__ARMCLIB_VERSION <= 6000000)
//如果编译器是AC5  就定义下面这个结构体
struct __FILE
{
	int handle;
};
#endif

FILE __stdout;

//定义_sys_exit()以避免使用半主机模式
void _sys_exit(int x)
{
	x = x;
}
#endif


//printf函数重定义
int fputc(int ch, FILE *stream)
{
	//当串口0忙的时候等待，不忙的时候再发送传进来的字符
	while( DL_UART_isBusy(UART_0_INST) == true );

	DL_UART_Main_transmitDataBlocking(UART_0_INST, ch);

	return ch;
}

int fputs(const char* restrict s,FILE* restrict stream)
{
   uint16_t i,len;
   len = strlen(s);
   for(i=0;i<len;i++)
   {
       DL_UART_Main_transmitDataBlocking(UART_0_INST,s[i]);
   }
   return len;
}

int puts(const char *_ptr)
{
    int count = fputs(_ptr,stdout);
    count += fputs("\n",stdout);
    return count;
}

//串口的中断服务函数
void UART_0_INST_IRQHandler(void)
{
	uint8_t receivedData = 0;

	//如果产生了串口中断
	switch( DL_UART_getPendingInterrupt(UART_0_INST) )
	{
		case DL_UART_IIDX_RX://如果是接收中断

			// 接收发送过来的数据保存
			receivedData = DL_UART_Main_receiveData(UART_0_INST);

			// 检查缓冲区是否已满
			if (recv0_length < RE_0_BUFF_LEN_MAX - 1)
			{
				recv0_buff[recv0_length++] = receivedData;

				// 将保存的数据再发送出去，不想回传可以注释掉
				uart0_send_char(receivedData);
			}
			else
			{
				recv0_length = 0;
			}

			// 标记接收标志
			recv0_flag = 1;

			break;

		default://其他的串口中断
			break;
	}
}


//串口的中断服务函数
void UART_1_INST_IRQHandler(void)
{
    uint8_t  UART2_Data = 0;
    static uint8_t Count=0;
    static uint8_t rs_count=0;
  static uint8_t last_rsnum=0;
    unsigned char ret = 0xff;
    unsigned short payload_len = 0;
    unsigned short check_sum = 0,CRC_num;
    unsigned short pos = 0;

    //如果产生了串口中断
    switch( DL_UART_getPendingInterrupt(UART_1_INST) )
    {

        case DL_UART_IIDX_RX://如果是接收中断
            NVIC_ClearPendingIRQ(UART_1_INST_INT_IRQN);
            UART2_Data = DL_UART_Main_receiveData(UART_1_INST);
            if(((last_rsnum==0x59)&&(UART2_Data == 0x53))||Count>0)
            {
                    rs_count=1;
                    if(Count==0)
                    {
                        g_uart_rx_buf[0] = 0x59;
                        g_uart_rx_buf[1] = 0x53;
                        Count = 1;
                    }
                    else
                    {
                        g_uart_rx_buf[Count] = UART2_Data;
                    }
                    Count++;

            }
            if(Count==67)
            {
                Count = 0;
                calc_checksum(g_uart_rx_buf+2,63, &check_sum);
                CRC_num = g_uart_rx_buf[65];
                if(check_sum == CRC_num)
                {


                }
                payload_len = g_uart_rx_buf[4];
                pos = 5;
                while(payload_len > 0)
                {
                    payload = (payload_data_t *)(g_uart_rx_buf+pos);
                    ret = check_data_len_by_id(payload->data_id, payload->data_len, (unsigned char *)payload + 2);
                    if((unsigned char)0x01 == ret)
                    {
                        pos += payload->data_len + sizeof(payload_data_t);
                        payload_len -= payload->data_len + sizeof(payload_data_t);
                    }
                    else
                    {
                        pos++;
                        payload_len--;
                    }
               }
            }
            last_rsnum=UART2_Data;
            break;
        default://其他的串口中断
            break;

    }
}


