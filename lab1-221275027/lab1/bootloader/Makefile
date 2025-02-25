bootloader.s1.bin: start_1.s boot.c boot.h
	gcc -c -m32 start_1.s -o start_1.o
	gcc -c -m32 -O1 -fno-stack-protector boot.c -o boot.o
	ld -m elf_i386 -e start -Ttext 0x7c00 start_1.o boot.o -o bootloader.elf
	objcopy -S -j .text -O binary bootloader.elf bootloader.bin
	../utils/genBoot.pl bootloader.bin

bootloader.s2.bin: start_2.s boot.c boot.h
	gcc -c -m32 start_2.s -o start_2.o
	gcc -c -m32 -O1 -fno-stack-protector boot.c -o boot.o
	ld -m elf_i386 -e start -Ttext 0x7c00 start_2.o boot.o -o bootloader.elf
	objcopy -S -j .text -O binary bootloader.elf bootloader.bin
	../utils/genBoot.pl bootloader.bin

bootloader.s3.bin: start_3.s boot.c boot.h
	gcc -c -m32 start_3.s -o start_3.o
	gcc -c -m32 -O1 -fno-stack-protector boot.c -o boot.o
	ld -m elf_i386 -e start -Ttext 0x7c00 start_3.o boot.o -o bootloader.elf
	objcopy -S -j .text -O binary bootloader.elf bootloader.bin
	../utils/genBoot.pl bootloader.bin

clean:
	rm -rf *.o *.elf *.bin
