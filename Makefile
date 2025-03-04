TOOLPREFIX = riscv64-unknown-elf-

CC = $(TOOLPREFIX)gcc
AS = $(TOOLPREFIX)as
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump

CFLAGS = -Wall -Werror -O -fno-omit-frame-pointer -ggdb
CFLAGS +=  -mcmodel=medany
LDFLAGS = -z max-page-size=4096


K=kernel

QEMU = qemu-system-riscv64
MEMORY = 128M
SMP = 1
KERNEL = $K/kernel
QEMUOPTS = -machine virt -bios none -kernel $(KERNEL) -m $(MEMORY) -smp $(SMP) -nographic -no-reboot

OBJS = 	$K/start.o \
		$K/main.o \
		$K/uart.o

run: $(KERNEL)
	$(QEMU) $(QEMUOPTS)

gdb: $(KERNEL) .gdbinit
	$(QEMU) $(QEMUOPTS) -S -gdb tcp::1234

$(KERNEL): $(OBJS) $K/kernel.ld
	$(LD) $(LDFLAGS) -T $K/kernel.ld -o $@ $(OBJS)

$K/start.o: $K/start.S
	$(CC) $(CFLAGS) -c $< -o $@

$K/%.o: $K/%.c
	$(CC) $(CFLAGS) -c -o $@ $<


clean:
	rm -f $K/*.o $K/*.bin $K/*.elf $K/*.dis $K/*.dump $K/*.map $K/*.sym $K/*.srec $K/*.hex $K/*.lst $K/*.out $K/*.img $K/*.elf

