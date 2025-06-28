riscv: riscv.s
	riscv64-unknown-elf-gcc -c -march=rv64imafd riscv.s -o rvs.o
	riscv64-unknown-elf-gcc -c -march=rv64imafd -ffreestanding riscv.c -o rvc.o
	riscv64-unknown-elf-ld -T riscv.ld rvs.o rvc.o -o rv_init.elf
	riscv64-unknown-elf-objcopy -O binary rv_init.elf rv_initcode
	riscv64-unknown-elf-objdump -d rv_init.elf
	od -A x1 -t x1 rv_initcode
	riscv64-unknown-elf-gcc init.c -o rv_init -static -nostdlib -e main

la: la.s
	loongarch64-unknown-linux-gnu-as la.s -o la_init.o
	loongarch64-unknown-linux-gnu-ld -Ttext=0 la_init.o -o la_init.elf
	loongarch64-unknown-linux-gnu-objcopy -O binary la_init.elf la_initcode
	loongarch64-unknown-linux-gnu-objdump -d la_init.elf
	od -A x1 -t x1 la_initcode
	loongarch64-unknown-linux-gnu-gcc init.c -o la_init -static -nostdlib -e main

all: la riscv

clean:
	rm -f *.elf *.o la_initcode rv_initcode rv_init la_init