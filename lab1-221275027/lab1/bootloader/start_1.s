# TODO: This is lab1.1
/* Real Mode Hello World */
.code16

.global start
start:
	movw %cs, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %ss
	movw $0x7d00, %ax
	movw %ax, %sp # setting stack pointer to 0x7d00
	# TODO:通过中断输出Hello World, 并且间隔1000ms打印新的一行

loop:
	jmp loop

message:
	.string "Hello, World!\n\0"







