#include "x86.h"
#include "device.h"

extern TSS tss;
extern ProcessTable pcb[MAX_PCB_NUM];
extern int current;

extern int displayRow;
extern int displayCol;

void GProtectFaultHandle(struct StackFrame *sf);

void timerHandle(struct StackFrame *sf);

void syscallHandle(struct StackFrame *sf);

void sysWrite(struct StackFrame *sf);
void sysPrint(struct StackFrame *sf);

void sysFork(struct StackFrame *sf);
void sysExec(struct StackFrame *sf);
void sysSleep(struct StackFrame *sf);
void sysExit(struct StackFrame *sf);
void sysGetPid(struct StackFrame *sf);

uint32_t schedule();
uint32_t loadUMain(uint32_t first_sector, uint32_t sector_count, uint32_t pid);

/*
kernel is loaded to location 0x100000, i.e., 1MB
size of kernel is not greater than 200*512 bytes, i.e., 100KB
user program is loaded to location 0x200000, i.e., 2MB
size of user program is not greater than 200*512 bytes, i.e., 100KB
*/
uint32_t loadUMain(uint32_t first_sector, uint32_t sector_count, uint32_t pid) 
{
	int i = 0;
	int phoff = 0x34; // program header offset
	int offset = 0x1000; // .text section offset
	uint32_t elf = 0x100000*(pid+1); // physical memory addr to load
	uint32_t uMainEntry = 0x100000*(pid+1);

	for (i = 0; i < sector_count; i++) {
		readSect((void*)(elf + i*512), first_sector+i);
	}
	
	uMainEntry = ((struct ELFHeader *)elf)->entry; // entry address of the program
	phoff = ((struct ELFHeader *)elf)->phoff;
	offset = ((struct ProgramHeader *)(elf + phoff))->off;

	for (i = 0; i < sector_count * 512; i++) {
		*(uint8_t *)(elf + i) = *(uint8_t *)(elf + i + offset);
	}

	return uMainEntry;
}


/*
 * Schedules the next process to run
 * 
 * Returns:
 *   pid_t - The process ID of the scheduled process
 *           Returns -1 if no process is available for scheduling
 */
uint32_t schedule() {
	// TODO: Select the next process and perform context switching
	int i = (current + 1) % MAX_PCB_NUM;
	while (i != current) {
		if (i != 0 && pcb[i].state == STATE_RUNNABLE)
			break;
		i = (i + 1) % MAX_PCB_NUM;
	}
	if (pcb[i].state != STATE_RUNNABLE)
		i = 0;
	current = i;
    pcb[current].state = STATE_RUNNING;
    uint32_t tmpStackTop = pcb[current].stackTop;
    tss.esp0 = pcb[current].prevStackTop;
    pcb[current].stackTop = pcb[current].prevStackTop;
	asm volatile("movl %0, %%esp"::"m"(tmpStackTop));
    asm volatile("popl %gs");
	asm volatile("popl %fs");
	asm volatile("popl %es");
	asm volatile("popl %ds");
	asm volatile("popal");
	asm volatile("addl $8, %esp");
	asm volatile("iret");


	return -1;
}

void irqHandle(struct StackFrame *sf)
{ // pointer sf = esp
	/* Reassign segment register */
	asm volatile("movw %%ax, %%ds" ::"a"(KSEL(SEG_KDATA)));
	/*XXX Save esp to stackTop */
	uint32_t tmpStackTop = pcb[current].stackTop;
	pcb[current].prevStackTop = pcb[current].stackTop;
	pcb[current].stackTop = (uint32_t)sf;

	switch (sf->irq)
	{
	case -1:
		break;
	case 0xd:
		GProtectFaultHandle(sf);
		break;
	case 0x20:
		timerHandle(sf);
		break;
	case 0x80:
		syscallHandle(sf);
		break;
	default:
		assert(0);
	}
	/*XXX Recover stackTop */
	pcb[current].stackTop = tmpStackTop;
}

void GProtectFaultHandle(struct StackFrame *sf)
{
	assert(0);
	return;
}

void timerHandle(struct StackFrame *sf)
{
	// TODO
}

void syscallHandle(struct StackFrame *sf)
{
	switch (sf->eax)
	{ // syscall number
	case 0:
		sysWrite(sf);
		break; // for SYS_WRITE
	/*TODO Add Fork, Sleep, Exit, Exec... */

	case 1:
		sysFork(sf);
		break; // for SYS_FORK
	
	case 2:
		sysExec(sf);
		break; // for SYS_EXEC

	case 3:
		sysSleep(sf);
		break; // for SYS_SLEEP

	case 4:
		sysExit(sf);
		break; // for SYS_EXIT

	case 5:
		sysGetPid(sf);
		break; // for SYS_GETPID
	
	default:
		break;
	}
}

void sysWrite(struct StackFrame *sf)
{
	switch (sf->ecx)
	{ // file descriptor
	case 0:
		sysPrint(sf);
		break; // for STD_OUT
	default:
		break;
	}
}

void sysPrint(struct StackFrame *sf)
{
	int sel = sf->ds; // segment selector for user data, need further modification
	char *str = (char *)sf->edx;
	int size = sf->ebx;
	int i = 0;
	int pos = 0;
	char character = 0;
	uint16_t data = 0;
	asm volatile("movw %0, %%es" ::"m"(sel));
	for (i = 0; i < size; i++)
	{
		asm volatile("movb %%es:(%1), %0" : "=r"(character) : "r"(str + i));
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
		}
		else
		{
			data = character | (0x0c << 8);
			pos = (80 * displayRow + displayCol) * 2;
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
		}
		// asm volatile("int $0x20"); //XXX Testing irqTimer during syscall
		// asm volatile("int $0x20":::"memory"); //XXX Testing irqTimer during syscall
	}

	updateCursor(displayRow, displayCol);
	// take care of return value
	return;
}


void sysFork(struct StackFrame *sf)
{
	// TODO: finish fork

	// 要做的是在寻找一个空闲的pcb做为子进程的进程控制块，
	// 将父进程的资源复制给子进程。如果没有空闲pcb，则fork失败，父进程返回-1，成功则子进程返回0，父进程返回子进程pid


	int pid = -1;
	for (int i = 0; i < MAX_PCB_NUM; i++){
		if (pcb[i].state == STATE_DEAD){
            pid = i;
			break;
        }
	}
	if (pid != -1){
		for (int j = 0; j < 0x100000; j++) {
			*(uint8_t *)(j + (pid+1)*0x100000) = *(uint8_t *)(j + (current+1)*0x100000);
		}
		pcb[pid].pid=pid;
		pcb[pid].prevStackTop=pcb[current].prevStackTop+(uint32_t)&pcb[pid]-(uint32_t)&pcb[current];
        pcb[pid].stackTop=pcb[current].stackTop + (uint32_t)&pcb[pid]-(uint32_t)&pcb[current];
		pcb[pid].sleepTime=0;		
		pcb[pid].state = STATE_RUNNABLE;
		pcb[pid].timeCount = 0;
		pcb[pid].regs.edi = sf->edi;
		pcb[pid].regs.esi = sf->esi;
		pcb[pid].regs.ebp = sf->ebp;
		pcb[pid].regs.xxx = sf->xxx;
		pcb[pid].regs.ebx = sf->ebx;
		pcb[pid].regs.edx = sf->edx;
		pcb[pid].regs.ecx = sf->ecx;
		pcb[pid].regs.eax = sf->eax;
		pcb[pid].regs.irq = sf->irq;
		pcb[pid].regs.error = sf->error;
		pcb[pid].regs.eip = sf->eip;
		pcb[pid].regs.esp = sf->esp;
		pcb[pid].regs.eflags = sf->eflags;
		pcb[pid].regs.cs = USEL((1 + pid * 2));
		pcb[pid].regs.ss = USEL((2 + pid * 2));
		pcb[pid].regs.ds = USEL((2 + pid * 2));
		pcb[pid].regs.es = USEL((2 + pid * 2));
		pcb[pid].regs.fs = USEL((2 + pid * 2));
		pcb[pid].regs.gs = USEL((2 + pid * 2));
		pcb[pid].regs.eax = 0; 
		pcb[current].regs.eax = pid; 
	}
	else pcb[current].regs.eax = -1; 
	return;


}

void sysExec(struct StackFrame *sf)
{
	// TODO: finish exec
}

void sysSleep(struct StackFrame *sf)
{
	// TODO: finish sleep

	// 将当前的进程的sleepTime设置为传入的参数，将当前进程的状态设置为STATE_BLOCKED
	pcb[current].sleepTime = sf->ebx;
	pcb[current].state = STATE_BLOCKED;
	asm("int $0x20");
	return;

}

void sysExit(struct StackFrame *sf)
{
	// TODO: finish exit
	// 将当前进程的状态设置为STATE_DEAD，然后模拟时钟中断进行进程切换
	pcb[current].state = STATE_DEAD;
	asm("int $0x20");
	return;
}

void sysGetPid(struct StackFrame *sf)
{
	// TODO: finish getpid

	// 将当前进程的pid返回
	sf->eax =pcb[current].pid;
	return;

}