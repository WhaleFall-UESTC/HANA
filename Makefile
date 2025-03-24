ARCH ?= riscv
KERNEL = kernel-$(ARCH)
BUILD_DIR = build/$(ARCH)

ifeq ($(ARCH), riscv)
TOOLPREFIX ?= riscv64-unknown-elf-
QEMU ?= qemu-system-riscv64
RISCV_CFLAGS = -mcmodel=medany -march=rv64imafd -mabi=lp64
RISCV_CFLAGS += -DARCH_RISCV 

else ifeq ($(ARCH), loongarch)
TOOLPREFIX ?= loongarch64-linux-gnu-
QEMU ?= qemu-system-loongarch64
LOONGARCH_CFLAGS = -march=loongarch64 -mabi=lp64

else
$(error Unsupported ARCH $(ARCH))
endif


CC = $(TOOLPREFIX)gcc
AS = $(TOOLPREFIX)as
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy


KERNEL_SRC = kernel
ARCH_SRC = $(KERNEL_SRC)/arch/$(ARCH)


CFLAGS = -Wall -Werror -O -fno-omit-frame-pointer -ggdb 
CFLAGS += $(if $(RISCV_CFLAGS),$(RISCV_CFLAGS),$(LOONGARCH_CFLAGS)) 
CFLAGS += -I $(KERNEL_SRC)/include -I $(ARCH_SRC)/include
CFLAGS += -MD -MP -MF $(BUILD_DIR)/$(@F).d

ASFLAGS = $(CFLAGS) -D__ASSEMBLY__
LDFLAGS = -nostdlib -T $(KERNEL_SRC)/kernel.ld


SRC_S := $(shell find $(ARCH_SRC) -type f -name *.S)

SRC_C := $(shell find kernel -type f -name '*.c' \
			-not -path 'kernel/test/*' \
			-not -path 'kernel/arch/*') \
		$(shell find $(ARCH_SRC) -type f -name *.c)

OBJS = $(addprefix $(BUILD_DIR)/, $(SRC_C:.c=.o) $(SRC_S:.S=.o))

# INCLUDE_DIRS := $(shell find $(SRC_DIR) -name '*.h' -exec dirname {} \; | sort -u)
# CFLAGS += $(addprefix -I, $(INCLUDE_DIRS))



all: $(KERNEL)

$(KERNEL): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $<
	@echo "[LD] Linked kernel image: $@"

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<
	@echo "[CC] Compiled $<"

$(BUILD_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -c -o $@ $<
	@echo "[AS] Assembled $<"

-include $(OBJS:.o=.d)

clean:
	rm -rf build

distclean: clean
	rm -f $(KERNEL)


MEMORY := 128M
SMP := 1
QEMUOPTS = 	-machine virt \
			-bios none \
			-kernel $(KERNEL) \
			-m $(MEMORY) \
			-smp $(SMP) \
			-nographic \
			-no-reboot

build: $(KERNEL)
	@echo "Build kernel image: $(KERNEL)"

run: $(KERNEL)
	$(QEMU) $(QEMUOPTS)

gdb: $(KERNEL) .gdbinit
	$(QEMU) $(QEMUOPTS) -S -gdb tcp::1234



.PHONY: all clean distclean build run gdb	
