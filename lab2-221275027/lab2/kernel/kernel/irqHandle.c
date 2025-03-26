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
			// bufferTail--;
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
			putChar(ch);//put char into serial
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
	char asc = getChar(code);
	uint16_t data = asc| (0x0c << 8);
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
	//asm volatile("movb %0, %%es:(%1)"::"r"(asc),"r"(str));
	//asm volatile("movb %0, %%eax":"=m"(asc));

	tf->eax = asc;


}

void sysGetStr(struct TrapFrame *tf){
	// TODO: 自由实现

	char* str=(char*)(tf->edx);//str pointer
	int size=(int)(tf->ebx);//str size
	//int t = 10;	
	bufferHead=0;
	bufferTail=0;
	for(int j=0;j<MAX_KEYBUFFER_SIZE;j++)keyBuffer[j]=0;//init
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
	int k=0;
	for(int p=bufferHead;p<i-1;p++){
		asm volatile("movb %0, %%es:(%1)"::"r"(keyBuffer[p]),"r"(str+k));
		k++;
	}
	asm volatile("movb $0x00, %%es:(%0)"::"r"(str+i));
	return;
}



#define USER_BASE  0x40000000U  // 示例用户空间基址
#define USER_SIZE  0x40000000U  // 示例用户空间大小
#define USER_END  (USER_BASE + USER_SIZE)

void sysGetTimeFlag(struct TrapFrame *tf) {
    // TODO: 自由实现
   
}
void sysSetTimeFlag(struct TrapFrame *tf) {
    // TODO: 自由实现


}
