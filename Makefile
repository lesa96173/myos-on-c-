all:
	nasm -f elf32 boot.s -o boot.o
	g++ -m32 -c drivers/vga.cpp -o drivers/vga.o -ffreestanding -O2 -Wall -Wextra -fno-exceptions -fno-rtti
	g++ -m32 -c fs/fs.cpp -o fs/fs.o -ffreestanding -O2 -Wall -Wextra -fno-exceptions -fno-rtti
	g++ -m32 -c kernel/memory.cpp -o kernel/memory.o -ffreestanding -O2 -Wall -Wextra -fno-exceptions -fno-rtti
	g++ -m32 -c libc/stdio.cpp -o libc/stdio.o -ffreestanding -O2 -Wall -Wextra -fno-exceptions -fno-rtti
	g++ -m32 -c kernel/kernel.cpp -o kernel/kernel.o -ffreestanding -O2 -Wall -Wextra -fno-exceptions -fno-rtti
	ld -m elf_i386 -T linker.ld -nostdlib boot.o drivers/vga.o fs/fs.o kernel/memory.o libc/stdio.o kernel/kernel.o -o myos.bin

run: all
	qemu-system-i386 -kernel myos.bin

clean:
	rm -f *.o drivers/*.o fs/*.o kernel/*.o libc/*.o myos.bin