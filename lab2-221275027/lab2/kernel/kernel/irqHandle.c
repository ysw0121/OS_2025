#include "x86.h"
#include "device.h"

extern int displayRow;
extern int displayCol;

extern uint32_t keyBuffer[MAX_KEYBUFFER_SIZE];
extern int bufferHead;
extern int bufferTail;


int tail=0;

void GProtectFaultHandle(struct TrapFrame *tf);

void KeyboardHandle(struct TrapFrame *tf);

void timerHandler(struct TrapFrame *tf);
void syscallHandle(struct TrapFrame *tf);
void sysWrite(struct TrapFrame *tf);
void sysPrint(struct TrapFrame *tf);
void sysRead(struct TrapFrame *tf);
void sysGetChar(struct TrapFrame *tf);
void sysGetStr(struct TrapFrame *tf);
void sysSetTimeFlag(struct TrapFrame *tf);
void sysGetTimeFlag(struct TrapFrame *tf);

// ============================ self defined =============================
#define GET_TIME_FLAG 4 // 获取时间标志
#define SET_TIME_FLAG 5 // 设置时间标志
#define NOW_TIME_FLAG 6
unsigned char getCMOS(uint8_t addr);
static inline int BCD2DEC(int bcd);
void sysNowTime(struct TrapFrame *tf);
int timeFlag=0;

struct TimeInfo {
	int second;
	int minute;
	int hour;
	int m_day;
	int month;
	int year;
};

// ======================================================================

void irqHandle(struct TrapFrame *tf) { // pointer tf = esp
	/*
	 * 中断处理程序
	 */
	/* Reassign segment register */
	asm volatile("movw %%ax, %%ds"::"a"(KSEL(SEG_KDATA)));

	switch(tf->irq) {
		// TODO: 填好中断处理程序的调用
		case 0xd:
			GProtectFaultHandle(tf);
			break;
		case 0x20:
			timerHandler(tf);
			break;
		case 0x21:
			KeyboardHandle(tf);
			break;
		case 0x80:
			syscallHandle(tf);
			break;
		case -1:
			break;


		default:assert(0);
	}
}

void GProtectFaultHandle(struct TrapFrame *tf){
	assert(0);
	return;
}

void KeyboardHandle(struct TrapFrame *tf){
	uint32_t code = getKeyCode();

	if(code == 0xe){ // 退格符
		//要求只能退格用户键盘输入的字符串，且最多退到当行行首
		if(displayCol > 0 && bufferTail > tail){
			displayCol--;
			bufferTail--; // backspace
			uint16_t data = 0 | (0x0c << 8);
			int pos = (80 * displayRow + displayCol) * 2;
			asm volatile("movw %0, (%1)" ::"r"(data), "r"(pos + 0xb8000));
		}
	}else if(code == 0x1c){ // 回车符
		//处理回车情况
		keyBuffer[bufferTail++]='\n';
		displayRow++;
		displayCol=0;
		tail=0;
		if (displayRow == 25)
		{
			displayRow = 24;
			displayCol = 0;
			scrollScreen();
		}
		
	}else if(code < 0x81){ 
		// TODO: 处理正常的字符
		
		
		char ch = getChar(code);
		if(ch!=0){
			// putChar(ch); // print to screen & cmd
			uint16_t data=ch|(0x0c<<8);
			keyBuffer[bufferTail]=ch;
			bufferTail++;
			bufferTail%=MAX_KEYBUFFER_SIZE;
			int pos=(80*displayRow+displayCol)*2;
			asm volatile("movw %0, (%1)"::"r"(data),"r"(pos+0xb8000));
			displayCol+=1;
			if(displayCol==80){
				displayCol=0;
				displayRow++;
				if(displayRow==25){
					scrollScreen();
					displayRow=24;
					displayCol=0;
				}
			}
		}
	}
	updateCursor(displayRow, displayCol);

}

void timerHandler(struct TrapFrame *tf) {
	// TODO:
	timeFlag=1;
}

void syscallHandle(struct TrapFrame *tf) {
	switch(tf->eax) { // syscall number
		case 0:
			sysWrite(tf);
			break; // for SYS_WRITE
		case 1:
			sysRead(tf);
			break; // for SYS_READ
		case 2:  // SYS_TIME
			switch(tf->ecx) {
				case GET_TIME_FLAG:  // 获取时间
					sysGetTimeFlag(tf);
					break;
				case SET_TIME_FLAG:  // 设置时间
					sysSetTimeFlag(tf);
					break;
				default:
					break;
			}
			break;

		case NOW_TIME_FLAG:
			// timeFlag=1;
			sysNowTime(tf);
			break;

		default:break;
	}
}

void sysWrite(struct TrapFrame *tf) {
	switch(tf->ecx) { // file descriptor
		case 0:
			sysPrint(tf);
			break; // for STD_OUT
		default:break;
	}
}

void sysPrint(struct TrapFrame *tf) {
	int sel =  USEL(SEG_UDATA);
	char *str = (char*)tf->edx;
	int size = tf->ebx;
	int i = 0;
	int pos = 0;
	char character = 0;
	uint16_t data = 0;
	asm volatile("movw %0, %%es"::"m"(sel));
	for (i = 0; i < size; i++) {
		asm volatile("movb %%es:(%1), %0":"=r"(character):"r"(str+i));
		// TODO: 完成光标的维护和打印到显存

		if(character=='\n'){
			displayRow++;
			displayCol=0;
			if(displayRow==25){
				scrollScreen();
				displayRow=24;
				displayCol=0;
			}
		}

		else{
			data = character | (0x0c << 8);
			pos = (80*displayRow+displayCol)*2;
			asm volatile("movw %0, (%1)"::"r"(data),"r"(pos+0xb8000));
			displayCol++;
			if(displayCol==80){
				displayRow++;
				displayCol=0;
				if(displayRow==25){
					scrollScreen();
					displayRow=24;
					displayCol=0;
				}
			}
		}
		

	}
	// tail=displayCol;
	updateCursor(displayRow, displayCol);
	return;
}

void sysRead(struct TrapFrame *tf){
	switch(tf->ecx){ //file descriptor
		case 0:
			sysGetChar(tf);
			break; // for STD_IN
		case 1:
			sysGetStr(tf);
			break; // for STD_STR
		default:break;
	}
}

void sysGetChar(struct TrapFrame *tf){
	// TODO: 自由实现

	uint32_t code = getKeyCode();
	while(!code){
		code = getKeyCode();
	}
	char ch = getChar(code);
	uint16_t data = ch|(0x0c << 8);
	int pos = (80 * displayRow + displayCol) * 2;
	asm volatile("movw %0, (%1)" ::"r"(data), "r"(pos + 0xb8000));
	displayCol++;
	if (displayCol == 80)
	{
		displayRow++;
		displayCol = 0;
		if (displayRow == 25)
		{
			displayRow = 24;
			displayCol = 0;
			scrollScreen();
		}
	}
	updateCursor(displayRow, displayCol);
	while(TRUE){
		code = getKeyCode();
		char character = getChar(code);
		if (character == '\n')
		{
			displayRow++;
			displayCol = 0;
			if (displayRow == 25)
			{
				displayRow = 24;
				displayCol = 0;
				scrollScreen();
			}
			updateCursor(displayRow, displayCol);
			break;
		}

	}


	tf->eax = ch;


}

void sysGetStr(struct TrapFrame *tf){
	// TODO: 自由实现

	int size=(int)(tf->ebx);//str size	
	char* str=(char*)(tf->edx);//str pointer
	bufferHead=0;
	bufferTail=0;
	for(int j=0;j<MAX_KEYBUFFER_SIZE;j++)keyBuffer[j]=0; // init
	
	int i=0;
	char tpc=0;
	while(tpc!='\n' && i<size){

		while(keyBuffer[i]==0){
			enableInterrupt();
		}
		tpc=keyBuffer[i];
		i++;
		disableInterrupt();
	}

	int selector=USEL(SEG_UDATA);
	asm volatile("movw %0, %%es"::"m"(selector));
	
	for(int p=bufferHead;p<i-1;p++){
		asm volatile("movb %0, %%es:(%1)"::"r"(keyBuffer[p]),"r"(str+p-bufferHead));
	}
	asm volatile("movb $0x00, %%es:(%0)"::"r"(str+i));
	return;
}





// 从CMOS读取时间
unsigned char getCMOS(uint8_t addr) {
    outByte(0x70, addr);
	char c = inByte(0x71);
    return c;
}

// BCD转十进制
static inline int BCD2DEC(int bcd) {
    return ((bcd >> 4)* 10) + (bcd & 0x0f);
}


void sysGetTimeFlag(struct TrapFrame *tf) {
    // TODO: 自由实现
	tf->eax = timeFlag;
	
	return;
    
}
void sysSetTimeFlag(struct TrapFrame *tf) {
    // TODO: 自由实现
	timeFlag=0;

}

void sysNowTime(struct TrapFrame *tf){
    int selector = USEL(SEG_UDATA);
    struct TimeInfo *time_p = (struct TimeInfo *)tf->ecx;
    asm volatile("movw %0, %%es"::"m"(selector));

    // 读取CMOS中的时间信息并转换为BCD
    int second = BCD2DEC(getCMOS(0x00));
    int minute = BCD2DEC(getCMOS(0x02));
    int hour = BCD2DEC(getCMOS(0x04));
    int day = BCD2DEC(getCMOS(0x07));
    int month = BCD2DEC(getCMOS(0x08));
    int year = BCD2DEC(getCMOS(0x09));

    // UTC+8时区转换
    hour += 8;
    if (hour >= 24) {
        hour -= 24;
        day += 1;
        // 处理月份日期
        int monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        // 处理闰年
        if (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) {
            monthDays[1] = 29;
        }
        if (day > monthDays[month - 1]) {
            day = 1;
            month += 1;
            if (month > 12) {
                month = 1;
                year += 1;
            }
        }
    }

    // 写回用户空间
    asm volatile("movl %0, %%es:(%1)"::"r"(second),"r"(&(time_p->second)));
    asm volatile("movl %0, %%es:(%1)"::"r"(minute),"r"(&(time_p->minute)));
    asm volatile("movl %0, %%es:(%1)"::"r"(hour),"r"(&(time_p->hour)));
    asm volatile("movl %0, %%es:(%1)"::"r"(day),"r"(&(time_p->m_day)));
    asm volatile("movl %0, %%es:(%1)"::"r"(month),"r"(&(time_p->month)));
    asm volatile("movl %0, %%es:(%1)"::"r"(year),"r"(&(time_p->year)));

    return;
}