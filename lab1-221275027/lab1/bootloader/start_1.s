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


	# 将 clock_handle 的偏移地址存储到 0x70（偏移量）
	movw $clock_handle, 0x70          
	movw $0, 0x72 
	
	movw %ax, %sp # setting stack pointer to 0x7d00
    pushw $13 # pushing the size to print into stack
    pushw $message # pushing the address of message into stack
    callw displayStr # calling the display function


loop:
	jmp loop

message:
	.string "Hello, World!\n\0"

displayStr:
    pushw %bp
    movw 4(%esp), %ax
    movw %ax, %bp
    movw 6(%esp), %cx
    movw $0x1301, %ax
    movw $0x000c, %bx
    movw $0x0000, %dx
    int $0x10
    popw %bp
    ret


clock_handle:
   # 发送命令字节到控制寄存器端口0x43
    movw $0x36, %ax         #方式3 ， 用于定时产生中断00110110b
    movw $0x43, %dx
    out %al, %dx 
            # 计算计数值， 产生20 毫秒的时钟中断， 时钟频率为1193180 赫兹
            # 计数值 = ( 时钟频率/ 每秒中断次数) − 1
            #       = (1193180 / (1 / 0.02 ) ) − 1= 23863
    movw $23863, %ax
            # 将计数值分为低字节和高字节， 发送到计数器0的数据端口（ 端口0x40 ）
    movw $0x40, %dx
    out %al, %dx 
    mov %ah, %al
    out %al, %dx
    sti



