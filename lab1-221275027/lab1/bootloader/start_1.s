/* Real Mode Hello World */
.code16

.global start
start:
    movw %cs, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movw $0x7d00, %sp       # set stack pointer 0x7d00

    # 初始化显存段寄存器GS为0xB800
    movw $0xB800, %ax
    movw %ax, %gs

    # 设置时钟中断向量0x70 (段地址:偏移地址)
    movw $clock_handle, 0x70    # 偏移地址
    movw %cs, 0x72             # 段地址=CS

    # 配置8253定时器通道0，模式3，50Hz（20ms中断一次）
    movb $0x36, %al            # 控制字：00110110b
    outb %al, $0x43            # 写入控制寄存器
    movw $23863, %ax           # 计数值=1193180/50 -1
    outb %al, $0x40            # 低字节
    movb %ah, %al
    outb %al, $0x40            # 高字节

    sti                        # 启用中断

loop:
    jmp loop                   # 等待中断

message:
    .string "Hello, World!\n\0"

count:
    .word 0                    # 中断计数器
line_pos:
    .word 5                   # 当前显示行号（从第5行开始）


displayStr:
    push %bp
    mov %sp, %bp
    movw 4(%bp), %bx 
    movw 6(%bp), %cx 
    movw (line_pos), %ax 
    imul $160, %ax, %di
    movb $0x0D, %ah
.nextChar:
    movb (%bx), %al
    movw %ax, %gs:(%di) 
    addw $2, %di             # 下一个字符位置
    incw %bx                 # 下一个字符
    loop .nextChar
    incw (line_pos)          # 行号+1，下次显示到下一行
    pop %bp
    ret


# 时钟中断处理程序
clock_handle:
    pusha                      # 保存寄存器
    push %ds

    movw %cs, %ax
    movw %ax, %ds             # 设置DS=CS

    # 发送EOI到8259 PIC
    movb $0x20, %al
    outb %al, $0x20

    # 更新中断计数器
    incw (count)
    cmpw $50, (count)         # 检查是否达到50次（1秒）
    jb .done

    # 打印"Hello, World!"
    movw $0, (count)          # 重置计数器
    pushw $13                 # 字符串长度（2字节压栈）
    pushw $message            # 字符串地址（2字节压栈）
    call displayStr           # 正确调用函数
    add $4, %sp              # 清理栈（2个pushw共4字节）

.done:
    pop %ds
    popa
    iret                      # 中断返回