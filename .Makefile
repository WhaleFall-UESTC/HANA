TOOLPREFIX := riscv64-unknown-elf-

CC := $(TOOLPREFIX)gcc
AS := $(TOOLPREFIX)as
LD := $(TOOLPREFIX)ld
OBJCOPY := $(TOOLPREFIX)objcopy
OBJDUMP := $(TOOLPREFIX)objdump

CFLAGS = -Wall -Werror -O -fno-omit-frame-pointer -ggdb
CFLAGS +=  -mcmodel=medany
CFLAGS += -I $K/include
LDFLAGS = -z max-page-size=4096


K=kernel
KERNEL = $K/kernel

QEMU := qemu-system-riscv64
MEMORY := 128M
SMP := 1
QEMUOPTS = -machine virt -bios none -kernel $(KERNEL) -m $(MEMORY) -smp $(SMP) -nographic -no-reboot

CSRCS := $(shell find -type f -name *.c)

COBJS := $(CSRCS:.c=.o)

SSRCS := $(shell find -type f -name *.S | grep -v start.S)

SOBJS := $(SSRCS:.S=.o)

OBJS =  $K/start.o $(COBJS) $(SOBJS)

run: $(KERNEL)
	$(QEMU) $(QEMUOPTS)

gdb: $(KERNEL) .gdbinit
	$(QEMU) $(QEMUOPTS) -S -gdb tcp::1234

# build: $(OBJS) # $K/kernel.ld
# 	$(LD) $(LDFLAGS) -Ttext 0x80000000 -o $(KERNEL) $(OBJS) && \
# 	$(OBJDUMP) -S -l -D $(KERNEL) > $K/kernel.objdump

# $(KERNEL): $(OBJS) # $K/kernel.ld
# 	$(LD) $(LDFLAGS) -Ttext 0x80000000 -o $(KERNEL) $(OBJS) && \
# 	$(OBJDUMP) -S -l -D $(KERNEL) > $K/kernel.objdump

build: $(OBJS) $K/kernel.ld
	$(LD) $(LDFLAGS) -T $K/kernel.ld -o $(KERNEL) $(OBJS) && \
	$(OBJDUMP) -S -l -D $(KERNEL) > $K/kernel.objdump

$(KERNEL): $(OBJS) $K/kernel.ld
	$(LD) $(LDFLAGS) -T $K/kernel.ld -o $(KERNEL) $(OBJS) && \
	$(OBJDUMP) -S -l -D $(KERNEL) > $K/kernel.objdump

$K/start.o: $K/start.S
	$(CC) $(CFLAGS) -c $< -o $@

%.o : %.c %.S
	$(CC) $(CFLAGS) -c -o $@ $<


clean:
	rm -f $(OBJS) $(KERNEL) 

