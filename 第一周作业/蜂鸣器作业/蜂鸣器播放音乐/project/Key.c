#include <REGX52.H>
#include "Delay.h"

//定义播放引脚
sbit Buzzer = P2^5;

/**
  * @brief  获取独立按键键码
  * @retval 按下按键的键码，范围：0~4，无按键按下时返回值为0
  */
unsigned char Key()
{
	unsigned char KeyNumber = 0;
	
	// 只要有按键按下，第一时间关闭定时器，停止发声！
	if(P3_1==0 || P3_0==0 || P3_2==0 || P3_3==0)
	{
		TR0 = 0;     // 关闭定时器0
		Buzzer = 1;  // 将蜂鸣器引脚拉高，防止杂音
		
		// 接着进行常规的消抖和松开检测
		Delay(20);
		if(P3_1==0){while(P3_1==0); Delay(20); KeyNumber=1;} // 上一首
		else if(P3_0==0){while(P3_0==0); Delay(20); KeyNumber=2;} // 下一首
		else if(P3_2==0){while(P3_2==0); Delay(20); KeyNumber=3;} 
		else if(P3_3==0){while(P3_3==0); Delay(20); KeyNumber=4;} // 播放/暂停
	}
	
	return KeyNumber;
}