os.s1.img:
	cd bootloader; make bootloader.s1.bin
	cd app; make app.bin
	cat bootloader/bootloader.bin app/app.bin > os.img

os.s2.img:
	cd bootloader; make bootloader.s2.bin
	cd app; make app.bin
	cat bootloader/bootloader.bin app/app.bin > os.img

os.s3.img:
	cd bootloader; make bootloader.s3.bin
	cd app; make app.bin
	cat bootloader/bootloader.bin app/app.bin > os.img

clean:
	cd bootloader; make clean
	cd app; make clean
	rm -f os.img

play:
	qemu-system-i386 os.img

debug:
	qemu-system-i386 -s -S os.img
