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
		if(displayCol>0&&displayCol>tail){
			displayCol--;
			uint16_t data = 0 | (0x0c << 8);
			int pos = (80*displayRow+displayCol)*2;
			asm volatile("movw %0, (%1)"::"r"(data),"r"(pos+0xb8000));
		}
	}else if(code == 0x1c){ // 回车符
		//处理回车情况
		keyBuffer[bufferTail++]='\n';
		displayRow++;
		displayCol=0;
		tail=0;
		if(displayRow==25){
			scrollScreen();
			displayRow=24;
			displayCol=0;
		}
	}else if(code < 0x81){ 
		// TODO: 处理正常的字符
		keyBuffer[bufferTail++]=code;
		if(displayCol==80){
			displayRow++;
			displayCol=0;
			if(displayRow==25){
				scrollScreen();
				displayRow=24;
				displayCol=0;
			}
		}
		else{
			displayCol++;
		}
		

	}
	updateCursor(displayRow, displayCol);

}

void timerHandler(struct TrapFrame *tf) {
	// TODO
	static uint32_t ticks = 0;       // 系统时钟滴答计数器
    static uint32_t seconds = 0;     // 系统运行秒数
    
    // 发送EOI到PIC控制器
    outb(0x20, 0x20);               // 主PIC的EOI端口
    
    // 更新系统时钟
    if (++ticks % 1000 == 0) {  // 假设CLK_FREQ=1000（1kHz时钟）
        seconds++;                   // 每秒递增
    }
    
    
    

}

void syscallHandle(struct TrapFrame *tf) {
	switch(tf->eax) { // syscall number
		case 0:
			sysWrite(tf);
			break; // for SYS_WRITE
		case 1:
			sysRead(tf);
			break; // for SYS_READ
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
			continue;
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
	tail=displayCol;
	updateCursor(displayRow, displayCol);
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

	// 从键盘缓冲区获取字符
    uint32_t code= getKeyCode();
    
    // 通过陷阱帧返回结果
    tf->eax = code;
	

}

void sysGetStr(struct TrapFrame *tf){
	// TODO: 自由实现

	// 获取用户空间参数（根据系统调用约定）
    char *buffer = (char*)tf->ecx;    // 缓冲区地址（a1参数）
    int max_len = tf->edx;            // 最大长度（a2参数）
    int count = 0;
    char ch;

    // 参数校验
    if (max_len <= 0 || !buffer) {
        tf->eax = -1;  // 错误码：无效参数
        return;
    }

    // 循环读取字符
    while (count < max_len - 1) {

        uint32_t keyCode = getKeyCode();
        ch = getChar(keyCode);

        // 处理退格键（ASCII 8）
        if (ch == 8 && count > 0) {
            count--;
            continue;
        }
        
        // 回车或换行结束输入
        if (ch == '\r' || ch == '\n') {
            break;
        }
        
        // 只接受可打印字符
        if (ch >= 32 && ch <= 126) {
            buffer[count++] = ch;
        }
    }

    // 添加字符串终结符
    buffer[count] = '\0';
    
    // 通过陷阱帧返回实际读取长度
    tf->eax = count;

}

void sysGetTimeFlag(struct TrapFrame *tf) {
	// TODO: 自由实现

}

void sysSetTimeFlag(struct TrapFrame *tf) {
	// TODO: 自由实现

}
