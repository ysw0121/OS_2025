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
		if(bufferHead!=bufferTail){
			bufferTail--;
			displayCol--;
		}
	}else if(code == 0x1c){ // 回车符
		//处理回车情况
		displayRow++;
		displayCol=0;
		keyBuffer[bufferTail]='\n';
		bufferTail=(bufferTail+1)%MAX_KEYBUFFER_SIZE;
		if(bufferTail==bufferHead)bufferHead=(bufferHead+1)%MAX_KEYBUFFER_SIZE;
	}else if(code < 0x81){ 
		// TODO: 处理正常的字符
		
		displayCol++;
		keyBuffer[bufferTail]=code;
		bufferTail=(bufferTail+1)%MAX_KEYBUFFER_SIZE;
		if(bufferTail==bufferHead)bufferHead=(bufferHead+1)%MAX_KEYBUFFER_SIZE;
	}

	if(displayCol==80){
		displayRow++;
		displayCol=0;
	}
	if(displayRow==25) {
		displayRow=24;
		displayCol=0;
		scrollScreen();
	}
	updateCursor(displayRow, displayCol);

}

void timerHandler(struct TrapFrame *tf) {
	// TODO
	static uint32_t ticks = 0;       // 系统时钟滴答计数器
    static uint32_t seconds = 0;     // 系统运行秒数
    
    // 发送EOI到PIC控制器
	__asm__ __volatile__ (
        "outb %%al, %%dx" 
        : 
        : "a" (0x20),    // 立即数0x20存入al寄存器
          "d" (0x20)     // 端口号0x20存入dx寄存器
    );
    
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

	asm volatile("sti");
	while(bufferHead==bufferTail||keyBuffer[bufferTail-1]!='\n')waitForInterrupt();
	asm volatile("cli");
	tf->eax=getChar(keyBuffer[bufferHead]);
	bufferHead=bufferTail;


}

void sysGetStr(struct TrapFrame *tf){
	// TODO: 自由实现

	int sel = USEL(SEG_UDATA); 
	char *str = (char*)tf->edx;
	int size = tf->ebx;
	int i = 0;
	char c=0;
	asm volatile("sti");
	while(bufferHead==bufferTail||keyBuffer[bufferTail-1]!='\n')waitForInterrupt();
	asm volatile("cli");
	asm volatile("movw %0, %%es"::"m"(sel));
	for (i = 0; i < size; i++) {
		if(bufferHead==bufferTail)break;
		if(keyBuffer[bufferHead]=='\n')break;
		c=getChar(keyBuffer[bufferHead]);
		if(c!=0)asm volatile("movb %0, %%es:(%1)"::"r"(c),"r"(str+i));
		bufferHead=(bufferHead+1)%MAX_KEYBUFFER_SIZE;
	}
	asm volatile("movb $0x00, %%es:(%0)"::"r"(str+i));
	bufferHead=bufferTail;
}


#define USER_BASE  0x40000000U  // 示例用户空间基址
#define USER_SIZE  0x40000000U  // 示例用户空间大小
#define USER_END  (USER_BASE + USER_SIZE)

static volatile uint32_t system_time_flags = 0;

void sysGetTimeFlag(struct TrapFrame *tf) {
	// TODO: 自由实现
	// 用户空间内存范围定义（需与内存管理模块一致）
    

    // 错误码定义
    enum { EINVAL = -1, EFAULT = -2 };

    // 参数解析
    uint32_t *user_ptr = (uint32_t*)tf->ecx;  // 用户缓冲区地址
    uint32_t buf_size = tf->edx;              // 缓冲区大小

    // 参数有效性校验
    if (!user_ptr || buf_size < sizeof(uint32_t)) {
        tf->eax = EINVAL;
        return;
    }

    // 用户地址空间验证
    uint32_t target_addr = (uint32_t)user_ptr;
    if (target_addr < USER_BASE || 
        target_addr >= USER_END ||
        (target_addr + sizeof(uint32_t)) > USER_END) {
        tf->eax = EFAULT;
        return;
    }

    // 原子读取时间标志
    extern volatile uint32_t system_time_flags;
    uint32_t kernel_value;
    asm volatile (
        "movl %1, %0\n\t"   // 直接内存读取保证原子性
        : "=r"(kernel_value)
        : "m"(system_time_flags)
    );

    // 直接写入用户内存（需确保关闭MMU保护或具有权限）
    __asm__ __volatile__ (
        "movl %1, (%0)\n\t"  // 将内核值写入用户地址
        : 
        : "r"(user_ptr), "r"(kernel_value)
        : "memory"
    );

    tf->eax = 0;  // 成功返回

}

void sysSetTimeFlag(struct TrapFrame *tf) {
	// TODO: 自由实现



    // 复用错误码定义
    enum { EINVAL = -1, EFAULT = -2 };

    // 解析寄存器参数
    uint32_t *user_ptr = (uint32_t*)tf->ecx;  // 用户空间数据指针
    uint32_t data_size = tf->edx;             // 数据尺寸参数

    // 参数有效性校验（复用校验逻辑）
    if (!user_ptr || data_size != sizeof(uint32_t)) {
        tf->eax = EINVAL;
        return;
    }

    // 用户地址空间验证（完全复用地址检查逻辑）
    uint32_t target_addr = (uint32_t)user_ptr;
    if (target_addr < USER_BASE || 
        target_addr >= USER_END ||
        (target_addr + sizeof(uint32_t)) > USER_END) {
        tf->eax = EFAULT;
        return;
    }

    // 从用户空间直接读取数据（复用内核访问方式）
    uint32_t user_value;
    asm volatile (
        "movl (%1), %0\n\t"   // 直接读取用户内存
        : "=r"(user_value)
        : "r"(user_ptr)
        : "memory"
    );

    // 原子写入内核变量（复用全局变量声明）
    extern volatile uint32_t system_time_flags;
    asm volatile (
        "movl %1, %0\n\t"     // 直接写入内核变量
        : "=m"(system_time_flags)
        : "r"(user_value)
    );

    tf->eax = 0;  // 复用成功返回码

}
