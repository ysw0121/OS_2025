#include "x86.h"
#include "device.h"

#define INTERRUPT_GATE_32   0xE
#define TRAP_GATE_32        0xF

/* IDT表的内容 */
struct GateDescriptor idt[NR_IRQ]; // NR_IRQ=256, defined in x86/cpu.h

/* 初始化一个中断门(interrupt gate) */
static void setIntr(struct GateDescriptor *ptr, uint32_t selector, uint32_t offset, uint32_t dpl) {
	// TODO: 初始化interrupt gate

	ptr->offset_15_0 = offset & 0xffff;
	ptr->offset_31_16 = (offset >> 16) & 0xffff;
	ptr->segment = selector<<3;
	ptr->pad0 = 0;
	ptr->system = FALSE;
	ptr->type = INTERRUPT_GATE_32;
	ptr->privilege_level = dpl;
	ptr->present = TRUE;

}

/* 初始化一个陷阱门(trap gate) */
static void setTrap(struct GateDescriptor *ptr, uint32_t selector, uint32_t offset, uint32_t dpl) {
	// TODO: 初始化trap gate

	ptr->offset_15_0 = offset & 0xffff;
	ptr->offset_31_16 = (offset >> 16) & 0xffff;
	ptr->segment = selector<<3;
	ptr->pad0 = 0;
	ptr->system = FALSE;
	ptr->type = TRAP_GATE_32;
	ptr->privilege_level = dpl;
	ptr->present = TRUE;

}

/* 声明函数，这些函数在汇编代码里定义 */
void irqEmpty();
void irqErrorCode();

void irqDoubleFault(); // 0x8
void irqInvalidTSS(); // 0xa
void irqSegNotPresent(); // 0xb
void irqStackSegFault(); // 0xc
void irqGProtectFault(); // 0xd
void irqPageFault(); // 0xe
void irqAlignCheck(); // 0x11
void irqSecException(); // 0x1e
void irqKeyboard(); 
void irqTimer(); // 0x20

void irqSyscall();


//256 interrupt vector
void initIdt() {
	int i;
	/* 为了防止系统异常终止，所有irq都有处理函数(irqEmpty)。 */
	for (i = 0; i < NR_IRQ; i ++) {
		setTrap(idt + i, SEG_KCODE, (uint32_t)irqEmpty, DPL_KERN);
	}
	/*init your idt here 初始化 IDT 表, 为中断设置中断处理函数*/

	// TODO: 参考上面第48行(setTrap)代码填好剩下的表项
	setTrap(idt + 0x8, SEG_KCODE, (uint32_t)irqDoubleFault, DPL_KERN);
	setTrap(idt + 0xa, SEG_KCODE, (uint32_t)irqInvalidTSS, DPL_KERN);
	setTrap(idt + 0xb, SEG_KCODE, (uint32_t)irqSegNotPresent, DPL_KERN);
	setTrap(idt + 0xc, SEG_KCODE, (uint32_t)irqStackSegFault, DPL_KERN);
	setTrap(idt + 0xd, SEG_KCODE, (uint32_t)irqGProtectFault, DPL_KERN);
	setTrap(idt + 0xe, SEG_KCODE, (uint32_t)irqPageFault, DPL_KERN);
	setTrap(idt + 0x11, SEG_KCODE, (uint32_t)irqAlignCheck, DPL_KERN);
	setTrap(idt + 0x1e, SEG_KCODE, (uint32_t)irqSecException, DPL_KERN);
	setTrap(idt + 0x21, SEG_KCODE, (uint32_t)irqKeyboard, DPL_KERN);
	setIntr(idt + 0x20, SEG_KCODE, (uint32_t)irqTimer, DPL_KERN); // ?
	setIntr(idt + 0x80, SEG_KCODE, (uint32_t)irqSyscall, DPL_USER);

	/* 写入IDT */
	saveIdt(idt, sizeof(idt));//use lidt
}
