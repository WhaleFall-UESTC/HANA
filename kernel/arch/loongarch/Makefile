# This Makefile is only used for test loongarch kernel

PROJ_BASE=/home/whalefall/Share/Project/WhaleOS

ARCH ?= loongarch
BUILD_DIR := $(PROJ_BASE)/build/$(ARCH)
SMP := 1
MEM := 128M

KERNEL := $(PROJ_BASE)/kernel-la
DISK := disk-la.img
# Newlib does not support LoongArch yet
TOOLPREFIX := loongarch64-unknown-linux-gnu-
QEMU := qemu-system-loongarch64
QEMUOPTS := -kernel $(KERNEL) -m $(MEM) -nographic -smp $(SMP) -no-reboot 
LOONGARCH_CFLAGS = -march=loongarch64 -mabi=lp64d
LOONGARCH_CFLAGS += -DARCH_LOONGARCH

CC = $(TOOLPREFIX)gcc
AS = $(TOOLPREFIX)as
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump

KERNEL_SRC = $(PROJ_BASE)/kernel
ARCH_SRC = $(PROJ_BASE)/kernel/arch/$(ARCH)

CFLAGS = -Wall -Werror -O0 -fno-omit-frame-pointer -ggdb -nostdlib
CFLAGS += $(if $(RISCV_CFLAGS),$(RISCV_CFLAGS),$(LOONGARCH_CFLAGS)) 
CFLAGS += -I $(KERNEL_SRC)/include -I $(ARCH_SRC)/include -I $(KERNEL_SRC)/test/include
CFLAGS += -MD -MP -MF $@.d
CFLAGS += -Wno-unused-va$(@F).driable -Wno-unused-function
CFLAGS += -DDEBUG 

ASFLAGS = $(CFLAGS) -D__ASSEMBLY__
LDFLAGS = -nostdlib -T $(ARCH_SRC)/kernel.ld

SRC_S = $(shell find $(ARCH_SRC) -type f -name '*.S')

SRC_C := $(shell find $(ARCH_SRC) -type f -name '*.c') \
		 $(shell find $(KERNEL_SRC)/lib -type f -name '*.c') \
		 $(shell find $(KERNEL_SRC)/mm -type f -name '*.c') \

BUILD_SRC_C := $(SRC_C:$(PROJ_BASE)%=$(BUILD_DIR)%) 

BUILD_SRC_S := $(SRC_S:$(PROJ_BASE)%=$(BUILD_DIR)%)

OBJS = $(BUILD_SRC_C:.c=.o) $(BUILD_SRC_S:.S=.o)

all: $(KERNEL)

$(KERNEL): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^
	@echo "[LD] Linked kernel image: $@"

$(BUILD_DIR)/%.o: $(PROJ_BASE)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<
	@echo "[CC] Compiled $<"

$(BUILD_DIR)/%.o: $(PROJ_BASE)/%.S
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<
	@echo "[CC] Assembled $<"


dump:
	loongarch64-unknown-linux-gnu-objdump -S -l -D $(KERNEL) > $(KERNEL).asm

run: $(KERNEL)
	$(QEMU) $(QEMUOPTS)

gdb: $(KERNEL) ../../../.gdbinit-$(ARCH)
	$(QEMU) $(QEMUOPTS) -S -gdb tcp::9877

clean:
	rm -rf $(BUILD_DIR)