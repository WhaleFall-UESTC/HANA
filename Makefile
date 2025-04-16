ARCH ?= riscv
KERNEL = kernel-$(ARCH)
BUILD_DIR = build/$(ARCH)

ifeq ($(ARCH), riscv)
TOOLPREFIX ?= riscv64-unknown-elf-
QEMU ?= qemu-system-riscv64
RISCV_CFLAGS = -mcmodel=medany -march=rv64imafd -mabi=lp64
RISCV_CFLAGS += -DARCH_RISCV 

else ifeq ($(ARCH), loongarch)
# Newlib does not support LoongArch yet
TOOLPREFIX ?= loongarch64-unknown-linux-gnu-
QEMU ?= qemu-system-loongarch64
LOONGARCH_CFLAGS = -march=loongarch64 -mabi=lp64
LOONGARCH_CFLAGS += -DARCH_LOONGARCH

else
$(error Unsupported ARCH $(ARCH))
endif


CC = $(TOOLPREFIX)gcc
AS = $(TOOLPREFIX)as
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump


KERNEL_SRC = kernel
ARCH_SRC = $(KERNEL_SRC)/arch/$(ARCH)


CFLAGS = -Wall -Werror -O -fno-omit-frame-pointer -ggdb
CFLAGS += $(if $(RISCV_CFLAGS),$(RISCV_CFLAGS),$(LOONGARCH_CFLAGS)) 
CFLAGS += -I $(KERNEL_SRC)/include -I $(ARCH_SRC)/include -I $(KERNEL_SRC)/test/include
# CFLAGS += -MD -MP -MF $(BUILD_DIR)/$(@F).d
CFLAGS += -MD -MP -MF $@.d
CFLAGS += -Wno-unused-variable -Wno-unused-function
CFLAGS += -DDEBUG

ASFLAGS = $(CFLAGS) -D__ASSEMBLY__
LDFLAGS = -nostdlib -T $(KERNEL_SRC)/kernel.ld


SRC_S = $(shell find $(ARCH_SRC) -type f -name *.S)

SRC_C := $(shell find kernel -type f -name '*.c' \
			-not -path '$(KERNEL_SRC)/arch/*' \
			-not -path '$(KERNEL_SRC)/fs/*' \
			-not -path '$(KERNEL_SRC)/lwext4/*') \
		$(shell find $(ARCH_SRC) -type f -name *.c) \

# -not -path '$(KERNEL_SRC)/drivers/virtio/*'

OBJS = $(addprefix $(BUILD_DIR)/, $(SRC_C:.c=.o) $(SRC_S:.S=.o))

# INCLUDE_DIRS := $(shell find $(SRC_DIR) -name '*.h' -exec dirname {} \; | sort -u)
# CFLAGS += $(addprefix -I, $(INCLUDE_DIRS))



all: $(KERNEL)

$(KERNEL): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^
	@echo "[LD] Linked kernel image: $@"

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<
	@echo "[CC] Compiled $<"

$(BUILD_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<
	@echo "[CC] Assembled $<"

-include $(OBJS:.o=.d)

clean:
	rm -rf build

distclean: clean
	rm -f $(KERNEL)


disk:
	qemu-img create -f raw disk.img 2G



VIRTIO_DBG = -d guest_errors,trace:virtio_*

QEUMDBG = \
			# $(VIRTIO_DBG) \
			# -D qemu.log

MEMORY := 128M
SMP := 1
QEMUOPTS = 	$(QEUMDBG) \
			-machine virt \
			-bios none \
			-kernel $(KERNEL) \
			-m $(MEMORY) \
			-smp $(SMP) \
			-nographic \
			-no-reboot \
			-device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0 \
			-drive file=disk.img,if=none,format=raw,id=x0 \
			# -device virtio-net-device,netdev=net -netdev user,id=net

build: $(KERNEL)
	$(OBJDUMP) -S -l -D $(KERNEL) > $(KERNEL).objdump

run: build
	$(QEMU) $(QEMUOPTS)

gdb: build .gdbinit
	$(QEMU) $(QEMUOPTS) -S -gdb tcp::9877


.PHONY: all clean distclean build run gdb disk
