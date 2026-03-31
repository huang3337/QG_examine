#include <REGX52.H>
#include "UART.h"
#include "AT24C02.h"
#include "Delay.h"
#include "LCD1602.h"

// 缓冲区设为16，刚好对应LCD1602一行的长度
unsigned char RxBuf[16]; 
unsigned char RxIndex = 0; // 记录当前存到了第几个字符
bit RxFlag = 0;  // 接收完成标志：0表示没收完，1表示收完(遇到回车)

void main()
{
	unsigned char i;
	unsigned char readData;
	
	UART_Init(); // 初始化串口
	LCD_Init();  // 初始化LCD
	
	//
	// 单片机上电，先去 EEPROM 里读出上次保存的数据
	//
	LCD_ShowString(1, 1, "EEPROM Data:");
	UART_SendString("EEPROM Data:\r\n");
	
	// 挨个读取，最多读16个字符
	for (i = 0; i < 16; i++)
	{
		readData = AT24C02_ReadByte(i);
		
		// 如果读到结束符 \0 或者芯片空白处 0xFF，说明数据读完了
		if (readData == '\0' || readData == 0xFF) 
		{
			RxBuf[i] = '\0'; // 加上结束符
			break;           // 退出读取循环
		}
		RxBuf[i] = readData; // 存入数组
	}
	
	// 把读出来的老数据，同时显示在LCD第二行和电脑串口上
	LCD_ShowString(2, 1, RxBuf);
	UART_SendString(RxBuf);
	UART_SendString("\r\nWait New Data...\r\n");

	//
	// 主循环，一直等待串口发来新数据，然后保存
	//
	while (1)
	{
		// RxFlag == 1 说明中断函数里收到回车键，数据接收完毕
		if (RxFlag == 1)
		{
			LCD_Init(); // 重新初始化LCD，相当于一键清屏
			LCD_ShowString(1, 1, "Saving...");
			UART_SendString("Saving...\r\n");
			
			// 把刚才收到的字符串，挨个写进 EEPROM
			for (i = 0; i <= RxIndex; i++)
			{
				AT24C02_WriteByte(i, RxBuf[i]);
				Delay(5); // 硬件要求：写EEPROM必须等5ms
			}
			
			LCD_ShowString(2, 1, "Save OK!");
			UART_SendString("Save OK!\r\n");
			
			// 保存完，清零变量，准备迎接下一次接收
			RxIndex = 0;
			RxFlag = 0;
		}
	}
}


// 串口中断函数：电脑发来一个字母，这里就执行一次

void UART_Routine() interrupt 4
{
	if (RI == 1) // 如果是接收中断
	{
		RI = 0;  // 必须手动清零标志位
		
		// 如果上一次的数据已经处理完了 (RxFlag == 0)
		if (RxFlag == 0) 
		{
			// 如果收到了回车键（\n 或 \r），说明电脑发完了一句话
			if (SBUF == '\n' || SBUF == '\r')
			{
				RxBuf[RxIndex] = '\0'; // 把回车键替换成字符串结束符 \0
				RxFlag = 1;            // 举起标志位，通知 main 函数保存
			}
			// 如果不是回车，且数组还没满（留最后一位给 \0）
			else if (RxIndex < 15)
			{
				RxBuf[RxIndex] = SBUF; // 存入数组
				RxIndex++;             // 索引后移一位
			}
		}
	}
}