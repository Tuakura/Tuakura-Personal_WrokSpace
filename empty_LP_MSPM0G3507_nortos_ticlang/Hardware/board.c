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

//����δ�ʱ��ʵ�ֵľ�ȷus��ʱ
void delay_us(unsigned long __us) 
{
    uint32_t ticks;
    uint32_t told, tnow, tcnt = 38;

    // ������Ҫ��ʱ���� = �ӳ�΢���� * ÿ΢���ʱ����
    ticks = __us * (32000000 / 1000000);

    // ��ȡ��ǰ��SysTickֵ
    told = SysTick->VAL;

    while (1)
    {
        // �ظ�ˢ�»�ȡ��ǰ��SysTickֵ
        tnow = SysTick->VAL;

        if (tnow != told)
        {
            if (tnow < told)
                tcnt += told - tnow;
            else
                tcnt += SysTick->LOAD - tnow + told;

            told = tnow;

            // ����ﵽ����Ҫ��ʱ���������˳�ѭ��
            if (tcnt >= ticks)
                break;
        }
    }
}
//����δ�ʱ��ʵ�ֵľ�ȷms��ʱ
void delay_ms(unsigned long ms) 
{
	delay_us( ms * 1000 );
}

void delay_1us(unsigned long __us){ delay_us(__us); }
void delay_1ms(unsigned long ms){ delay_ms(ms); }

//���ڷ��͵����ַ�
void uart0_send_char(char ch)
{
	//������0æ��ʱ��ȴ�����æ��ʱ���ٷ��ʹ��������ַ�
	while( DL_UART_isBusy(UART_0_INST) == true );
	//���͵����ַ�
	DL_UART_Main_transmitData(UART_0_INST, ch);

}
//���ڷ����ַ���
void uart0_send_string(char* str)
{
	//��ǰ�ַ�����ַ���ڽ�β ���� �ַ����׵�ַ��Ϊ��
	while(*str!=0&&str!=0)
	{
		//�����ַ����׵�ַ�е��ַ��������ڷ������֮���׵�ַ����
		uart0_send_char(*str++);
	}
}


#if !defined(__MICROLIB)
//��ʹ��΢��Ļ�����Ҫ�������ĺ���
#if (__ARMCLIB_VERSION <= 6000000)
//�����������AC5  �Ͷ�����������ṹ��
struct __FILE
{
	int handle;
};
#endif

FILE __stdout;

//����_sys_exit()�Ա���ʹ�ð�����ģʽ
void _sys_exit(int x)
{
	x = x;
}
#endif


//printf�����ض���
int fputc(int ch, FILE *stream)
{
	//������0æ��ʱ��ȴ�����æ��ʱ���ٷ��ʹ��������ַ�
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

//���ڵ��жϷ�����
void UART_0_INST_IRQHandler(void)
{
	uint8_t receivedData = 0;

	//��������˴����ж�
	switch( DL_UART_getPendingInterrupt(UART_0_INST) )
	{
		case DL_UART_IIDX_RX://����ǽ����ж�

			// ���շ��͹��������ݱ���
			receivedData = DL_UART_Main_receiveData(UART_0_INST);

			// ��黺�����Ƿ�����
			if (recv0_length < RE_0_BUFF_LEN_MAX - 1)
			{
				recv0_buff[recv0_length++] = receivedData;

				// ������������ٷ��ͳ�ȥ������ش�����ע�͵�
				uart0_send_char(receivedData);
			}
			else
			{
				recv0_length = 0;
			}

			// ��ǽ��ձ�־
			recv0_flag = 1;

			break;

		default://�����Ĵ����ж�
			break;
	}
}


//���ڵ��жϷ�����
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

    //��������˴����ж�
    switch( DL_UART_getPendingInterrupt(UART_1_INST) )
    {

        case DL_UART_IIDX_RX://����ǽ����ж�
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
        default://�����Ĵ����ж�
            break;

    }
}


