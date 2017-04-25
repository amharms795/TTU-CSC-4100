all: boot2 boot1

boot1: boot2.exe boot1.S
	nasm -l boot1.list -DENTRY=`./getaddr.sh main` boot1.S

boot2: boot2.exe
	objcopy -S -O binary boot2.exe boot2

boot2.exe: boot2_S.o boot2_c.o
	ld  -g -melf_i386 -Ttext 0x10000 -e main -o boot2.exe boot2_S.o boot2_c.o

boot2_S.o: boot2.S
	gcc -g -m32 -c -masm=intel -o boot2_S.o boot2.S

boot2_c.o: boot2.c
	gcc -g -m32 -c -o boot2_c.o boot2.c

install:
	bximage -fd -size=1.44 -q a.img
	mformat a:
	dd if=boot1 of=a.img bs=1 count=512 conv=notrunc
	mcopy -o boot2 a:BOOT2

run:
	qemu-system-i386 -boot a -fda a.img

launch:
	${MAKE} clean
	${MAKE} all
	${MAKE} install
	${MAKE} run
	${MAKE} clean

dubug:
	qemu-system-i386 -S -s -boot a -fda a.img

clean:
	rm -rf *o *exe boot2
	rm -rf *o *.list boot1
	rm -rf *img a
