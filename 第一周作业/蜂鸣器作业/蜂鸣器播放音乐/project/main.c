#include <REGX52.H>
#include "Delay.h"
#include "Timer0.h"
#include "Key.h"

// 直接定义蜂鸣器引脚
sbit Buzzer = P2^5;

// 引入外部音乐数据
extern unsigned char code *MusicList[];
extern unsigned int FreqTable[];
extern unsigned char SongIndex;
extern unsigned int NoteSelect;
extern unsigned char FreqSelect;

// 播放状态变量
bit Isplaying = 0; 

#define MAX_SONG 8
#define SPEED 500

// 记录当前音符剩余时长的倒计时变量
unsigned int CurrentNoteTimer = 0; 

// 速度控制变量，默认 4 ( SPEED/4 为1倍速)
unsigned char SpeedDivisor = 4;

void main() 
{
    unsigned char KeyNum;
    Timer0_Init();
	Isplaying = 0; // 默认开机暂停

    while (1) 
	{
		// 1. 扫描按键
        KeyNum = Key();
		
		// 2. 按键松开后的逻辑处理
		if(KeyNum != 0)
		{
			if(KeyNum == 4) // P3_3 播放/暂停
			{
				Isplaying = !Isplaying;
			}
			else if(KeyNum == 3) // P3_2 二倍速/正常速度切换
			{
				if(SpeedDivisor == 4) 
				{
					SpeedDivisor = 8;      // 变为2倍速 (时值缩短一半)
					CurrentNoteTimer /= 2; // 让当前还没播完的音符立刻变短
				}
				else 
				{
					SpeedDivisor = 4;      // 恢复1倍速
					CurrentNoteTimer *= 2; // 核心细节：让当前还没播完的音符立刻拉长
				}
			}
			else if(KeyNum == 2) // P3_0 下一首
			{
				SongIndex = (SongIndex + 1) % MAX_SONG;
				NoteSelect = 0;
				CurrentNoteTimer = 0; // 清除当前音符时长，准备读新歌
				Isplaying = 1;        // 切歌后自动播放
			}
			else if(KeyNum == 1) // P3_1 上一首
			{
				if(SongIndex == 0) SongIndex = MAX_SONG - 1;
				else SongIndex--;
				NoteSelect = 0;
				CurrentNoteTimer = 0;
				Isplaying = 1;
			}
		}

		// 3. 播放状态机 (1ms处理一次)
		if(Isplaying)
		{
			// 如果当前音符时长倒计时归零，说明该换下一个音符了
			if(CurrentNoteTimer == 0)
			{
				// 如果不是休止符
				if(MusicList[SongIndex][NoteSelect] != 0xFF)
				{
					// 读取频率
					FreqSelect = MusicList[SongIndex][NoteSelect];
					NoteSelect++;
					
					// 使用 SpeedDivisor 动态计算音符需要持续的时长 (ms)
					CurrentNoteTimer = (SPEED / SpeedDivisor) * MusicList[SongIndex][NoteSelect];
					NoteSelect++;
					
					TR0 = 1; // 开始产生声音频率
				}
				else // 如果是 0xFF 结束标志
				{
					TR0 = 0;
					Buzzer = 1;// 置高电平彻底关闭蜂鸣器
					Delay(1000); // 歌与歌之间停顿1秒
					
					// 自动下一首
					SongIndex = (SongIndex + 1) % MAX_SONG;
					NoteSelect = 0;
				}
			}

			// 如果当前音符还没播完，持续计时
			if(CurrentNoteTimer > 0)
			{
				Delay(1); // 严格等待1ms
				CurrentNoteTimer--; // 倒计时减去1ms
				
				// 短暂停顿，让音符之间有颗粒感 (如果是二倍速，停顿也稍微缩短一点防卡音)
				if(CurrentNoteTimer == (SpeedDivisor == 8 ? 2 : 5)) 
				{
					TR0 = 0; 
					Buzzer = 1;
				}
			}
		}
		else 
		{
			// 暂停状态
			TR0 = 0;
			Buzzer = 1;
		}
    }
}

// 产生声音频率的中断
void Timer0_Routine() interrupt 1 
{
	if(FreqTable[FreqSelect])	//如果不是休止符
	{
		TL0 = FreqTable[FreqSelect]%256;		
		TH0 = FreqTable[FreqSelect]/256;		
		Buzzer=!Buzzer;	//翻转蜂鸣器IO口
	}
}