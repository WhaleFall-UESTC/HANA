rvinit: init.c
	riscv64-unknown-elf-gcc init.c -o rv_init -static -nostdlib -e main
	
lainit: init.c
	loongarch64-unknown-linux-gnu-gcc init.c -o la_init -static -nostdlib -e main

all: la riscv

clean:
	rm -f *.elf *.o la_initcode rv_initcode rv_init la_init