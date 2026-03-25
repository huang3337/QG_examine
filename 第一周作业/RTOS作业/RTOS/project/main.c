#include <REGX52.H>
#include <INTRINS.H>

#define MAX_TASKS    2				//任务的数量
#define MAX_TASK_DEP 32				//每个任务栈的深度（字节），决定每个任务最多能嵌套调用多少层函数

#define TASK_READY     0			//任务的状态，READY就切换过去、SUSPENDED就短暂开中断（让定时器跑），再检查下一个
#define TASK_SUSPENDED 1

unsigned char idata task_sp[MAX_TASKS];				//存各个人物的SP
unsigned char idata task_stack[MAX_TASKS][MAX_TASK_DEP];//在内部RAM划分两个数组用于充当独立栈区。idata强制放在内部RAM是因为sp只能寻址内部RAM
unsigned char task_id;//当前任务的编号
unsigned char task_state[MAX_TASKS];//任务的状态数组
unsigned int  task_delay[MAX_TASKS];//每个任务的剩余时长，被os_delay()设定时长，由定时器每1ms-1

void task_load(unsigned int fn, unsigned char tid);
void task_switch(void);
void os_delay(unsigned int ticks);
void task0(void);
void task1(void);

void Timer0_ISR() interrupt 1 {
    unsigned char i;

    //重装初值，维持 1ms 周期
	
	TL0 = 0x66;				//设置定时初始值
	TH0 = 0xFC;				//设置定时初始值

    //遍历所有挂起任务，延时计数减1；减到0则恢复就绪
    for (i = 0; i < MAX_TASKS; i++) {
        if (task_state[i] == TASK_SUSPENDED && task_delay[i] > 0) {
            task_delay[i]--;
            if (task_delay[i] == 0)
                task_state[i] = TASK_READY;
        }
    }
}

//
void main() {
    //初始化定时器0，模式1（16位），1ms中断，11.0592MHz
	TMOD &= 0xF0;			//设置定时器模式
	TMOD |= 0x01;			//设置定时器模式
	TL0 = 0x66;				//设置定时初始值
	TH0 = 0xFC;				//设置定时初始值
	TF0 = 0;				//清除TF0标志
	TR0 = 1;				//定时器0开始计时
	ET0=1;
	EA=1;
	PT0=0;
    //为两个任务构造初始栈帧
    task_load(task0, 0);
    task_load(task1, 1);

    //切换SP到task0，return时RET指令跳入task0
    task_id = 0;
    SP = task_sp[0];
}

//初始化任务栈帧函数
//手动将函数地址写入栈，模拟CALL压栈后的状态
//8051压栈顺序：PCL先压（低地址），PCH后压（高地址）
//RET弹栈顺序相反：先弹PCH，再弹PCL
void task_load(unsigned int fn, unsigned char tid) {
    task_stack[tid][0] = (unsigned char)fn;        // PCL，低字节
    task_stack[tid][1] = (unsigned char)(fn >> 8); // PCH，高字节
    task_sp[tid]       = (unsigned char)&task_stack[tid][1]; // SP指向栈顶
    task_state[tid]    = TASK_READY;
    task_delay[tid]    = 0;
}

//任务切换函数
//保存当前SP，轮询找下一个就绪任务，切换SP后return跳入新任务
void task_switch() {
    unsigned char next;

    EA = 0;
    task_sp[task_id] = SP; // 保存当前任务的栈指针

    //轮询找就绪任务；若全部挂起则短暂开中断等待定时器唤醒
    next = task_id;
    do {
        next = (next + 1) % MAX_TASKS;
        if (task_state[next] == TASK_READY) break;
        EA = 1;
        _nop_();
        EA = 0;
    } while (1);

    task_id = next;
    SP = task_sp[task_id]; // 切换到新任务的栈
    EA = 1;
    // return → RET → 从新任务栈弹出地址 → 跳入新任务
}

//任务延时挂起函数
//将自己挂起ticks毫秒，立刻让出CPU
//定时器负责计时，到期后自动恢复就绪
void os_delay(unsigned int ticks) {
    EA = 0;
    task_delay[task_id] = ticks;
    task_state[task_id] = TASK_SUSPENDED;
    EA = 1;
    task_switch();
}

//task0：P20~P23每0.5秒闪烁
void task0() {
    P2_0 = 0;
	P2_1 = 0;
	P2_2 = 0;
	P2_3 = 0;
    while (1) {
        P2_0 = ~P2_0;
		P2_1 = ~P2_1;
		P2_2 = ~P2_2;
		P2_3 = ~P2_3;
        os_delay(500); //挂起500ms，期间CPU运行task1
    }
}

//task1：P24~P27每1秒闪烁
void task1() {
    P2_4 = 0;
	P2_5 = 0;
	P2_6 = 0;
	P2_7 = 0;
    while (1) {
        P2_4 = ~P2_4;
		P2_5 = ~P2_5;
		P2_6 = ~P2_6;
		P2_7 = ~P2_7;
        os_delay(1000); // 挂起1000ms，期间CPU运行task0
    }
}