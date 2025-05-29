ARCH ?= loongarch
BUILD_DIR := build/$(ARCH)
FS := rootfs.img
SMP := 1
MEM := 128M

ifeq ($(ARCH), riscv)
KERNEL := kernel-rv
DISK := disk.img
TOOLPREFIX := riscv64-unknown-elf-
QEMU := qemu-system-riscv64
QEMUOPTS := -machine virt -kernel $(KERNEL) -m $(MEM) -nographic -smp $(SMP) -bios default -drive file=$(FS),if=none,format=raw,id=x0 \
        	-device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0 -no-reboot -device virtio-net-device,netdev=net -netdev user,id=net \
        	-rtc base=utc \
        	-drive file=$(DISK),if=none,format=raw,id=x1 -device virtio-blk-device,drive=x1,bus=virtio-mmio-bus.1
RISCV_CFLAGS = -mcmodel=medany -march=rv64imafd -mabi=lp64
RISCV_CFLAGS += -DARCH_RISCV 
RISCV_CFLAGS += -DBIOS_SBI

else ifeq ($(ARCH), loongarch)
KERNEL := kernel-la
DISK := disk-la.img
# Newlib does not support LoongArch yet
TOOLPREFIX := loongarch64-unknown-linux-gnu-
QEMU := qemu-system-loongarch64
QEMUOPTS := -kernel $(KERNEL) -m $(MEM) -nographic -smp $(SMP) -drive file=$(FS),if=none,format=raw,id=x0  \
            -device virtio-blk-pci,drive=x0,bus=virtio-mmio-bus.0 -no-reboot  -device virtio-net-pci,netdev=net0 \
            -netdev user,id=net0,hostfwd=tcp::5555-:5555,hostfwd=udp::5555-:5555  \
            -rtc base=utc \
            -drive file=$(DISK),if=none,format=raw,id=x1 -device virtio-blk-pci,drive=x1,bus=virtio-mmio-bus.1
LOONGARCH_CFLAGS = -march=loongarch64 -mabi=lp64d
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
ARCH_TEST_SRC = $(KERNEL_SRC)/test/arch/$(ARCH)

KERNELDUMP = $(KERNEL).asm


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
		 $(shell find $(ARCH_TEST_SRC) -type f -name '*.c') \
		 $(shell find $(KERNEL_SRC) -type f -name '*.c' \
		 	-not -path '$(KERNEL_SRC)/test/arch/*' \
		 	-not -path '$(KERNEL_SRC)/drivers/virtio/*' \
			-not -path '$(KERNEL_SRC)/arch/*')   # do not delete this line

OBJS = $(addprefix $(BUILD_DIR)/, $(SRC_C:.c=.o) $(SRC_S:.S=.o))



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
	rm -rf build kernel-rv kernel-rv.asm

$(FS):
	qemu-img create -f raw $(FS) 2G
	mkfs.ext4 $(FS)
	@echo "[DISK] Created root filesystem image: $(FS)"

$(DISK): $(FS)
	qemu-img create -f raw $(DISK) 2G
	@echo "[DISK] Created disk image: $(DISK)"

$(KERNELDUMP): $(KERNEL)
	$(OBJDUMP) -S -l -D $(KERNEL) > $(KERNELDUMP)
	@echo "[OBJDUMP] Dump kernel: $(KERNELDUMP)"

distclean: clean
	rm -f $(KERNEL) $(KERNELDUMP) $(DISK) $(FS)

build: $(KERNEL) $(DISK) $(FS)
	@echo "[BUILD] Kernel and disk images are ready."

build_all: $(KERNELDUMP) $(DISK)

run: build
	$(QEMU) $(QEMUOPTS)

gdb: build_all .gdbinit-$(ARCH)
	$(QEMU) $(QEMUOPTS) -S -gdb tcp::9877


.PHONY: all clean distclean build run gdb disk temp
