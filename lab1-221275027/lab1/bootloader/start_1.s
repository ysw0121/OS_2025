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

	# output Hello World
	movw $message, %bp
	movw $13, %cx
	movw $0x1301, %ax
	movw $0x000c, %bx
	movw $0x0000, %dx
	int $0x10


loop:
	# 设置40ms定时器中断（约25次=1000ms）
	movw $0x8600, %ax
	movw $0x0028, %cx  # 40ms = 40000μs (0x9C40)
	movw $0x9C40, %dx
	int $0x15
	
	# 输出新行
	movw $newline, %bp
	movw $1, %cx
	int $0x10
	jmp loop

message:
	.string "Hello, World!\n\0"

newline:
	.string "\n\0"







