#include "x86.h"
#include "device.h"

SegDesc gdt[NR_SEGMENTS];       // the new GDT, NR_SEGMENTS=10, defined in x86/memory.h
TSS tss;

ProcessTable pcb[MAX_PCB_NUM]; // pcb
int current; // current process

// For initIdle() only
int32_t syscall(int num, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5);

/*
MACRO
SEG(type, base, lim, dpl) (SegDesc) {...};
SEG_KCODE=1
SEG_KDATA=2
SEG_TSS=NR_SEGMENTS-1
DPL_KERN=0
DPL_USER=3
KSEL(desc) (((desc)<<3) | DPL_KERN)
USEL(desc) (((desc)<<3) | DPL_USER)
asm [volatile] (AssemblerTemplate : OutputOperands [ : InputOperands [ : Clobbers ] ])
asm [volatile] (AssemblerTemplate : : InputOperands : Clobbers : GotoLabels)
*/
void initSeg() { // setup kernel segements
	int i;

	gdt[SEG_KCODE] = SEG(STA_X | STA_R, 0,       0xffffffff, DPL_KERN);
	gdt[SEG_KDATA] = SEG(STA_W,         0,       0xffffffff, DPL_KERN);

	for (i = 1; i < MAX_PCB_NUM; i++) {
		gdt[1+i*2] = SEG(STA_X | STA_R, (i+1)*0x100000,0x00100000, DPL_USER);
		gdt[2+i*2] = SEG(STA_W,         (i+1)*0x100000,0x00100000, DPL_USER);
	}
	
	gdt[SEG_TSS] = SEG16(STS_T32A,      &tss, sizeof(TSS)-1, DPL_KERN);
	gdt[SEG_TSS].s = 0;

	setGdt(gdt, sizeof(gdt)); // gdt is set in bootloader, here reset gdt in kernel

	/* initialize TSS */
	tss.ss0 = KSEL(SEG_KDATA);
	asm volatile("ltr %%ax":: "a" (KSEL(SEG_TSS)));

	/* reassign segment register */
	asm volatile("movw %%ax,%%ds":: "a" (KSEL(SEG_KDATA)));
	asm volatile("movw %%ax,%%ss":: "a" (KSEL(SEG_KDATA)));

	lLdt(0);
	
}

void initProc() {
	int i;
	for (i = 0; i < MAX_PCB_NUM; i++) {
		pcb[i].state = STATE_DEAD;
	}
}

void initIdle(void){
	// Initialize the idle process as a user process, using kernel text section as its code segment
	pcb[0].stackTop = (uint32_t)&(pcb[0].stackTop);
	pcb[0].prevStackTop = (uint32_t)&(pcb[0].stackTop);
	pcb[0].state = STATE_RUNNING;
	pcb[0].timeCount = MAX_TIME_COUNT;
	pcb[0].sleepTime = 0;
	pcb[0].pid = 0;
	pcb[0].regs.ss = USEL(2);
	// no need to change esp
	pcb[0].regs.cs = USEL(1);
	// no need to change eip
	pcb[0].regs.ds = USEL(2);
	pcb[0].regs.es = USEL(2);
	pcb[0].regs.fs = USEL(2);
	pcb[0].regs.gs = USEL(2);
	current = 0;
	asm volatile("movl %0, %%esp"::"m"(pcb[0].stackTop)); // switch to kernel stack for kernel idle process
	enableInterrupt();
	
	// System call to create new process via fork()
	int32_t ret = syscall(1, 0, 0, 0, 0, 0);
	
	// Process differentiation after fork
	if(ret == 0) {
		// Child Process: Execute new program
		/*
		kernel is loaded to location 0x100000, i.e., 1MB
		size of kernel is not greater than 200*512 bytes, i.e., 100KB
		size of user program is not greater than 200*512 bytes(200 Sectors), i.e., 100KB
		*/
		// SYS_EXEC, 起始扇区号， 扇区数量，0，0，0
		syscall(2, (uint32_t)201, (uint32_t)(200), 0, 0, 0);
	} else {
		// Parent Process: Idle scheduler
		// This runs only when no other processes are runnable.
		// LOG: enter idle
		char buffer[46] = "First fork: First entering the idle process\n";
		syscall(0, 0, (uint32_t)buffer, 45, 0, 0);
		// Infinite idle loop
		while(1);
		// waitForInterrupt();
		// What's the function of `waitForInterrupt`? 
		// Why we commented it out ?
	}
}

int32_t syscall(int num, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5){

	int32_t ret = 0;
	
	uint32_t eax, ecx, edx, ebx, esi, edi;
	asm volatile("movl %%eax, %0" : "=m"(eax));
	asm volatile("movl %%ecx, %0" : "=m"(ecx));
	asm volatile("movl %%edx, %0" : "=m"(edx));
	asm volatile("movl %%ebx, %0" : "=m"(ebx));
	asm volatile("movl %%esi, %0" : "=m"(esi));
	asm volatile("movl %%edi, %0" : "=m"(edi));
	asm volatile("movl %0, %%eax" ::"m"(num));
	asm volatile("movl %0, %%ecx" ::"m"(a1));
	asm volatile("movl %0, %%edx" ::"m"(a2));
	asm volatile("movl %0, %%ebx" ::"m"(a3));
	asm volatile("movl %0, %%esi" ::"m"(a4));
	asm volatile("movl %0, %%edi" ::"m"(a5));
	asm volatile("int $0x80");
	asm volatile("movl %%eax, %0" : "=m"(ret));
	asm volatile("movl %0, %%eax" ::"m"(eax));
	asm volatile("movl %0, %%ecx" ::"m"(ecx));
	asm volatile("movl %0, %%edx" ::"m"(edx));
	asm volatile("movl %0, %%ebx" ::"m"(ebx));
	asm volatile("movl %0, %%esi" ::"m"(esi));
	asm volatile("movl %0, %%edi" ::"m"(edi));

	return ret;
}