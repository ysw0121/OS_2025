#include "x86.h"
#include "device.h"

SegDesc gdt[NR_SEGMENTS];       // the new GDT, NR_SEGMENTS=10, defined in x86/memory.h
TSS tss;

ProcessTable pcb[MAX_PCB_NUM]; // pcb
int current; // current process

Semaphore sem[MAX_SEM_NUM];
Device dev[MAX_DEV_NUM];
SharedVariable sharedVar[MAX_SHARED_VAR_NUM];

// only for the use of initIdle()
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
		gdt[1+i*2] = SEG(STA_X | STA_R, (i)*0x100000,0x00200000, DPL_USER);
		gdt[2+i*2] = SEG(STA_W,         (i)*0x100000,0x00200000, DPL_USER);
	}

	// "virtual" segment descripers 
	// 	: they point to the same part of SEG_KCODE and SEG_KDATA
	// 	  but they're with DPL_USER
	// 	  so that idle becomes a USER process
	gdt[SEG_IDLE_CODE] = SEG(STA_X | STA_R, 0, 	0xffffffff, DPL_USER);
	gdt[SEG_IDLE_DATA] = SEG(STA_W, 		0, 	0xffffffff, DPL_USER);
	
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

void initSem() {
	int i;
	for (i = 0; i < MAX_SEM_NUM; i++) {
		sem[i].state = 0; // 0: not in use; 1: in use;
		sem[i].value = 0; // >=0: no process blocked; -1: 1 process blocked; -2: 2 process blocked;...
		sem[i].pcb.next = &(sem[i].pcb);
		sem[i].pcb.prev = &(sem[i].pcb);
	}
}

void initDev() {
	int i;
	for (i = 0; i < MAX_DEV_NUM; i++) {
		dev[i].state = 1; // 0: not in use; 1: in use;
		dev[i].value = 0; // >=0: no blocked; -1: 1 process blocked; -2: 2 process blocked;...
		dev[i].pcb.next = &(dev[i].pcb);
		dev[i].pcb.prev = &(dev[i].pcb);
	}
}

void initSharedVariable() {
	// TODO: init Shared Variable list

}

void initProc() {
	int i;
	for (i = 0; i < MAX_PCB_NUM; i++) {
		pcb[i].state = STATE_DEAD;
	}
}

void idle() {
	// syscall(SYS_FORK, 0, 0, 0, 0, 0)
	int32_t ret = syscall(2, 0, 0, 0, 0, 0);

	if (ret == 0) {
		// child: exec
		// syscall(SYS_EXEC, Begin_Sector, Sector_Num, 0, 0, 0)
		syscall(3, (uint32_t)201, (uint32_t)(200), 0, 0, 0);
	} else {
		// parent: run only when no other process is Runnable

		while (TRUE){
			// waitForInterrupt();
			// What's the function of `waitForInterrupt()`? 
			// Why we commented it out ?
			// you can also try a `putChar('i');` here
		};
	}
}

void initIdle(void) {
	// init idle and make it a user process, while it's text is the text of kernel

	// 1. idle's kernel stack
    pcb[0].stackTop = (uint32_t) & (pcb[0].stackTop) - sizeof(struct StackFrame);
	/*
	WHY `- sizeod(struct StackFrame)` ?
		: We need to fake a frame later.
		------------------------------------------
			struct ProcessTable {
				uint32_t stack[MAX_STACK_SIZE];
				struct StackFrame regs;				<- (uint32_t) & (pcb[i].stackTop) - sizeof(struct StackFrame)
				uint32_t stackTop;					<- (uint32_t) & (pcb[i].stackTop)
				uint32_t prevStackTop;
				...
			};
		------------------------------------------
	*/
	pcb[0].prevStackTop = (uint32_t)&(pcb[0].stackTop);

	// 2. some status and message of process 0 control
	pcb[0].state = STATE_RUNNING;
	pcb[0].timeCount = MAX_TIME_COUNT;
	pcb[0].sleepTime = 0;
	pcb[0].pid = 0;

	// 3. init segment selectors
	pcb[0].regs.ss = USEL(SEG_IDLE_DATA);
    pcb[0].regs.cs = USEL(SEG_IDLE_CODE);
    pcb[0].regs.ds = USEL(SEG_IDLE_DATA);
    pcb[0].regs.es = USEL(SEG_IDLE_DATA);
    pcb[0].regs.fs = USEL(SEG_IDLE_DATA);
    pcb[0].regs.gs = USEL(SEG_IDLE_DATA);

	// 4. init important env registers
    pcb[0].regs.esp = 0x1fffff;
    pcb[0].regs.eip = (uint32_t)idle;
    pcb[0].regs.ebp = 0x1fffff;
    pcb[0].regs.eflags = 0x202;

	// 5. fake `iret`
	// 		: pretend you had get interrupted into kernel from user idle process
    current = 0;
    uint32_t tmp = pcb[current].stackTop;
    pcb[current].stackTop = pcb[current].prevStackTop;
    tss.esp0 = pcb[current].stackTop;
    asm volatile("movl %0, %%esp" ::"m"(tmp));
    asm volatile("popl %%gs\n\t"      // 恢复 GS
                 "popl %%fs\n\t"      // 恢复 FS
                 "popl %%es\n\t"      // 恢复 ES
                 "popl %%ds\n\t"      // 恢复 DS
                 "popal\n\t"          // 恢复通用寄存器（EAX, EBX...）
                 "addl $4, %%esp\n\t" // intr
                 "addl $4, %%esp\n\t" // error node
                 "iret"               // 中断返回
                 ::
                     : "memory", "cc");

	while(1);
	return;
}

/*
#define SYS_WRITE 0
#define SYS_FORK 2
#define SYS_EXEC 3

#define STD_OUT 0
#define STD_IN 1
*/
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
