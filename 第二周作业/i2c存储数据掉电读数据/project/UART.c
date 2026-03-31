#include <REGX52.H>

void UART_Init(void)	//9600bps@11.0592MHz
{
    PCON &= 0x7F;
    SCON = 0x50;
    TMOD &= 0x0F;
    TMOD |= 0x20;
    TL1 = 0xFD;
    TH1 = 0xFD;
    ET1 = 0;
    TR1 = 1;
    EA = 1;
    ES = 1;
}

void UART_SendByte(unsigned char Byte)
{
    SBUF = Byte;
    while(TI == 0);
    TI = 0;
}


void UART_SendString(char *String)
{
    while(*String)
    {
        UART_SendByte(*String++);
    }
}