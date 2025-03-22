#include "lib.h"
#include "types.h"

typedef char *  va_list;
#define _INTSIZEOF(n)   ( (sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1) )
#define va_start(ap, format) ( ap = (va_list)&format+ _INTSIZEOF(format) )
#define va_arg(ap, type) ( *(type*)((ap += _INTSIZEOF(type)) -_INTSIZEOF(type)) )
#define va_end(ap)  ( ap = (va_list)0 )

/*
 * io lib here
 * 库函数写在这
 */


//static inline int32_t syscall(int num, uint32_t a1,uint32_t a2,
int32_t syscall(int num, uint32_t a1,uint32_t a2,
		uint32_t a3, uint32_t a4, uint32_t a5)
{
	int32_t ret = 0;
	//Generic system call: pass system call number in AX
	//up to five parameters in DX,CX,BX,DI,SI
	//Interrupt kernel with T_SYSCALL
	//
	//The "volatile" tells the assembler not to optimize
	//this instruction away just because we don't use the
	//return value
	//
	//The last clause tells the assembler that this can potentially
	//change the condition and arbitrary memory locations.

	/*
		 Note: ebp shouldn't be flushed
	    May not be necessary to store the value of eax, ebx, ecx, edx, esi, edi
	*/
	uint32_t eax, ecx, edx, ebx, esi, edi;
	uint16_t selector;
	
	asm volatile("movl %%eax, %0":"=m"(eax));
	asm volatile("movl %%ecx, %0":"=m"(ecx));
	asm volatile("movl %%edx, %0":"=m"(edx));
	asm volatile("movl %%ebx, %0":"=m"(ebx));
	asm volatile("movl %%esi,  %0":"=m"(esi));
	asm volatile("movl %%edi,  %0":"=m"(edi));
	asm volatile("movl %0, %%eax": :"m"(num));
	asm volatile("movl %0, %%ecx"::"m"(a1));
	asm volatile("movl %0, %%edx"::"m"(a2));
	asm volatile("movl %0, %%ebx"::"m"(a3));
	asm volatile("movl %0, %%esi" ::"m"(a4));
	asm volatile("movl %0, %%edi" ::"m"(a5));
	asm volatile("int $0x80");
	asm volatile("movl %%eax, %0":"=m"(ret));
	asm volatile("movl %0, %%eax"::"m"(eax));
	asm volatile("movl %0, %%ecx"::"m"(ecx));
	asm volatile("movl %0, %%edx"::"m"(edx));
	asm volatile("movl %0, %%ebx"::"m"(ebx));
	asm volatile("movl %0, %%esi"::"m"(esi));
	asm volatile("movl %0, %%edi"::"m"(edi));
	
	asm volatile("movw %%ss, %0":"=m"(selector)); //%ds is reset after iret
	//selector = 16;
	asm volatile("movw %%ax, %%ds"::"a"(selector));
	
	return ret;
}

char getChar(){ // 对应SYS_READ STD_IN
	// TODO: 实现getChar函数，方式不限
	char c = '\0';
    int ret;
    
    // 使用volatile确保所有操作按顺序执行
    asm volatile(
        "int $0x80" 
        : "=a" (ret)        // 返回值通过eax获取
        : "a" (SYS_READ),   // 系统调用号（假设为3）
          "b" (0),          // 文件描述符0（标准输入）
          "c" (&c),         // 缓冲区地址
          "d" (1)           // 读取长度1字节
        : "memory"          // 告知编译器内存被修改
    );
    
    return (ret == 1) ? c : '\0'; // 返回字符或空字符（失败时）

}

void getStr(char *str, int size){ // 对应SYS_READ STD_STR
	// TODO: 实现getStr函数，方式不限

	// 参数有效性检查
    if (!str|| size <= 1) {  // 至少保留1字节给null终止符
        if (str && size > 0) str[0] = '\0';
        return;
    }

    // 系统调用约定（基于x86架构）
    asm volatile (
        "int $0x80"                 // 触发系统调用
        : "+a" (size)               // 输入输出寄存器：系统调用号+返回长度
        : "a" (SYS_READ),           // 系统调用号（假设已定义）
          "b" (STD_STR),            // 标准输入描述符（假设0）
          "c" (str),                // 缓冲区地址
          "d" (size-1)              // 最大读取长度（保留null空间）
        : "memory"                  // 防止编译器优化内存访问
    );

    // 处理结果（基于系统调用返回值）
    if (size <= 0) {                // 系统调用返回实际读取长度
        str[0] = '\0';              // 错误时清空字符串
    } else {
        str[size-1] = '\0';         // 确保null终止
        if (str[size-2] == '\n') {  // 去除换行符
            str[size-2] = '\0';
        }
    }
}

int dec2Str(int decimal, char *buffer, int size, int count);
int hex2Str(uint32_t hexadecimal, char *buffer, int size, int count);
int str2Str(char *string, char *buffer, int size, int count);

void printf(const char *format,...){
	int i=0; // format index
	char buffer[MAX_BUFFER_SIZE];
	int count=0; // buffer index
	//int index=0; // parameter index
	void *paraList=(void*)&format; // address of format in stack
	//int state=0; // 0: legal character; 1: '%'; 2: illegal format
	int decimal=0;
	uint32_t hexadecimal=0;
	char *string=0;
	char character=0;
	while(format[i]!=0){
		// TODO: support format %d %x %s %c
		buffer[count]=format[i];
		count++;
		if(format[i]=='%'){
			count--;
			i++;
			paraList+=sizeof(format);
			switch(format[i]){
				case 'd':
					decimal=*(int*)paraList;
					count=dec2Str(decimal, buffer, MAX_BUFFER_SIZE, count);
					break;
				case 'x':
					hexadecimal=*(uint32_t*)paraList;
					count=hex2Str(hexadecimal, buffer, MAX_BUFFER_SIZE, count);
					break;
				case 's':
					string=*(char**)paraList;
					count=str2Str(string, buffer, MAX_BUFFER_SIZE, count);
					break;
				case 'c':
					character=*(char*)paraList;
					buffer[count]=character;
					count++;
					break;
				case '%':
					paraList-=sizeof(format);
					count++;
					break;
			}
		}
		if(count==MAX_BUFFER_SIZE) {
			syscall(SYS_WRITE, STD_OUT, (uint32_t)buffer, (uint32_t)MAX_BUFFER_SIZE, 0, 0);
			count=0;
		}
		i++;


	}
	if(count!=0)
		syscall(SYS_WRITE, STD_OUT, (uint32_t)buffer, (uint32_t)count, 0, 0);
}

void sleep(unsigned int seconds) {
	// TODO: 实现一个阻塞方式的sleep库函数
	
}

void now(struct TimeInfo *tm_info) {
	// TODO: 实现now函数，使用RTC方式
	
}
int dec2Str(int decimal, char *buffer, int size, int count) {
	int i=0;
	int temp;
	int number[16];

	if(decimal<0){
		buffer[count]='-';
		count++;
		if(count==size) {
			syscall(SYS_WRITE, STD_OUT, (uint32_t)buffer, (uint32_t)size, 0, 0);
			count=0;
		}
		temp=decimal/10;
		number[i]=temp*10-decimal;
		decimal=temp;
		i++;
		while(decimal!=0){
			temp=decimal/10;
			number[i]=temp*10-decimal;
			decimal=temp;
			i++;
		}
	}
	else{
		temp=decimal/10;
		number[i]=decimal-temp*10;
		decimal=temp;
		i++;
		while(decimal!=0){
			temp=decimal/10;
			number[i]=decimal-temp*10;
			decimal=temp;
			i++;
		}
	}

	while(i!=0){
		buffer[count]=number[i-1]+'0';
		count++;
		if(count==size) {
			syscall(SYS_WRITE, STD_OUT, (uint32_t)buffer, (uint32_t)size, 0, 0);
			count=0;
		}
		i--;
	}
	return count;
}

int hex2Str(uint32_t hexadecimal, char *buffer, int size, int count) {
	int i=0;
	uint32_t temp=0;
	int number[16];

	temp=hexadecimal>>4;
	number[i]=hexadecimal-(temp<<4);
	hexadecimal=temp;
	i++;
	while(hexadecimal!=0){
		temp=hexadecimal>>4;
		number[i]=hexadecimal-(temp<<4);
		hexadecimal=temp;
		i++;
	}

	while(i!=0){
		if(number[i-1]<10)
			buffer[count]=number[i-1]+'0';
		else
			buffer[count]=number[i-1]-10+'a';
		count++;
		if(count==size) {
			syscall(SYS_WRITE, STD_OUT, (uint32_t)buffer, (uint32_t)size, 0, 0);
			count=0;
		}
		i--;
	}
	return count;
}

int str2Str(char *string, char *buffer, int size, int count) {
	int i=0;
	while(string[i]!=0){
		buffer[count]=string[i];
		count++;
		if(count==size) {
			syscall(SYS_WRITE, STD_OUT, (uint32_t)buffer, (uint32_t)size, 0, 0);
			count=0;
		}
		i++;
	}
	return count;
}

