ARCH ?= riscv
BUILD_DIR := build/$(ARCH)
# TOOLS_DIR := tools
# FS := rootfs.img
SMP := 1
MEM := 128M

MOUNT_PATH := /mnt/mydisk

ifeq ($(ARCH), riscv)
KERNEL := kernel-rv
DISK := disk.img
FS := sdcard-rv.img
TOOLPREFIX := riscv64-unknown-elf-
QEMU := qemu-system-riscv64
QEMUOPTS := -machine virt -kernel $(KERNEL) -m $(MEM) -nographic -smp $(SMP) -bios default -drive file=$(DISK),if=none,format=raw,id=x0 \
        	-device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0 -no-reboot -device virtio-net-device,netdev=net -netdev user,id=net \
        	-rtc base=utc \
#         	-drive file=$(FS),if=none,format=raw,id=x1 -device virtio-blk-device,drive=x1,bus=virtio-mmio-bus.1 \
			# -d trace:virtio*
RISCV_CFLAGS = -mcmodel=medany -march=rv64imafd -mabi=lp64
# RISCV_CFLAGS += -DARCH_RISCV
RISCV_CFLAGS += -DBIOS_SBI
RISCV_CFLAGS += -fno-omit-frame-pointer -DBACKTRACE_USE_FP
ICCOM := rv
OBJTAR := elf64-littleriscv

else ifeq ($(ARCH), loongarch)
KERNEL := kernel-la
DISK := disk-la.img
FS := sdcard-la.img
TOOLPREFIX := loongarch64-unknown-linux-gnu-
QEMU := qemu-system-loongarch64
QEMUOPTS := -kernel $(KERNEL) -m $(MEM) -nographic -smp $(SMP) -drive file=$(FS),if=none,format=raw,id=x0  \
            -device virtio-blk-pci,drive=x0 -no-reboot \
            -rtc base=utc \
			-device virtio-net-pci,netdev=net0 \
            -netdev user,id=net0,hostfwd=tcp::5555-:5555,hostfwd=udp::5555-:5555  \
            -drive file=$(DISK),if=none,format=raw,id=x1 -device virtio-blk-pci,drive=x1,bus=pcie.0 \
			# -d guest_errors,trace:virtio*,trace:pic*,trace:apic*,trace:ioapic*,trace:pci*,trace:loongarch_msi* \
			# -d trace:loongarch_pch_pic_irq_handler,trace:loongarch_extioi* \
			# -d trace:loongarch_pch_pic_low*,trace:loongarch_pch_pic_high*,trace:loongarch_pch_pic_readb,trace:loongarch_pch_pic_writeb \
LOONGARCH_CFLAGS = -march=loongarch64 -mabi=lp64d
# LOONGARCH_CFLAGS += -DARCH_LOONGARCH

ICCOM := la
OBJTAR := elf64-loongarch

else
$(error Unsupported ARCH $(ARCH))
endif


CC = $(TOOLPREFIX)gcc
AS = $(TOOLPREFIX)as
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump

KERNEL_SRC = kernel
USER_SRC = user
INIT_SRC = user/init
ARCH_SRC = $(KERNEL_SRC)/arch/$(ARCH)
ARCH_TEST_SRC = $(KERNEL_SRC)/test/arch/$(ARCH)

KERNELDUMP = $(KERNEL).asm


CFLAGS = -Wall -Werror -O0 -fno-omit-frame-pointer -ggdb -nostdlib
CFLAGS += $(if $(RISCV_CFLAGS),$(RISCV_CFLAGS),$(LOONGARCH_CFLAGS)) 
CFLAGS += -MD -MP -MF $@.d
CFLAGS += -Wno-unused-va$(@F).driable -Wno-unused-function
CFLAGS += -DDEBUG 

U_CFLAGS = CFLAGS

CFLAGS += -I $(KERNEL_SRC)/include \
		  -I $(ARCH_SRC)/include \
		  -I $(KERNEL_SRC)/test/include 

ASFLAGS = $(CFLAGS) -D__ASSEMBLY__
LDFLAGS = -nostdlib -T $(ARCH_SRC)/kernel.ld


SRC_S = $(shell find $(ARCH_SRC) -type f -name '*.S')

SRC_C := $(shell find $(ARCH_SRC) -type f -name '*.c') \
		 $(shell find $(ARCH_TEST_SRC) -type f -name '*.c') \
		 $(shell find $(KERNEL_SRC) -type f -name '*.c' \
		 	-not -path '$(KERNEL_SRC)/test/arch/*' \
			-not -path '$(KERNEL_SRC)/arch/*')   # do not delete this line

OBJS = $(addprefix $(BUILD_DIR)/, $(SRC_C:.c=.o) $(SRC_S:.S=.o))


USRS := $(INIT_SRC)/$(ICCOM).S

USRC := $(shell find $(USER_SRC)/lib -type f -name '*.c')
# 			-not -path '$(USER_SRC)/init/*') 
USRC += $(INIT_SRC)/initcode.c

UOBJS = $(addprefix $(BUILD_DIR)/, $(USRC:.c=.o) $(USRS:.S=.o))

INIT_DST := $(BUILD_DIR)/user
INITCOBJ := $(INIT_DST)/$(ICCOM)_init.o

all: $(KERNEL)

$(KERNEL): $(OBJS) $(INITCOBJ)
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

$(INITCOBJ): $(UOBJS)
	$(LD) $^ -o $(INIT_DST)/$(ICCOM)_init.elf
	@echo "[LD] Linked initcode elf: $(INIT_DST)/$(ICCOM)_init.elf"
	$(OBJCOPY) -O binary $(INIT_DST)/$(ICCOM)_init.elf $(INIT_DST)/initcode
	@echo "[OBJCOPY] Copied to binary $(INIT_DST)/initcode"
	$(OBJCOPY) -I binary -O $(OBJTAR) $(INIT_DST)/initcode $@
	@echo "[OBJCOPY] Copied to binary $@"

-include $(OBJS:.o=.d)

clean:
	rm -rf build kernel-rv kernel-rv.asm kernel-la kernel-la.asm

user:
	

$(FS):
	qemu-img create -f raw $(FS) 2G
	mkfs.ext4 $(FS)
	@echo "[DISK] Created root filesystem image: $(FS)"

$(DISK): $(FS)
	$(MAKE) -C user

$(KERNELDUMP): $(KERNEL)
	$(OBJDUMP) -S -l -D $(KERNEL) > $(KERNELDUMP)
	@echo "[OBJDUMP] Dump kernel: $(KERNELDUMP)"

mount: $(DISK)
	@set -e; \
	if mountpoint -q $(MOUNT_PATH); then \
		echo "[OK] $(MOUNT_PATH) already mounted"; \
		cd $(MOUNT_PATH); \
	else \
		echo "[INFO] mounting $(DISK) -> $(MOUNT_PATH)"; \
		sudo mkdir -p $(MOUNT_PATH); \
		sudo mount -o loop,uid=$(shell id -u),gid=$(shell id -g),umask=000 $(DISK) $(MOUNT_PATH) && echo "[OK] mounted"; \
		cd $(MOUNT_PATH); \
	fi	

umount:
	@if mountpoint -q $(MOUNT_PATH); then \
		sudo umount $(MOUNT_PATH)  && echo "[OK] $(MOUNT_PATH) unmounted"; \
	else \
		echo "[WARN] $(MOUNT_PATH) not mounted"; \
	fi
	@if [ -n "$(LODEV)" ]; then \
		sudo losetup -d $(LODEV) && echo "[OK] loop device freed"; \
	fi

distclean: clean
	rm -f $(KERNEL) $(KERNELDUMP) $(DISK) $(FS)

build: $(KERNEL) $(DISK) $(FS)
	@echo "[BUILD] Kernel and disk images are ready."

build_all: $(KERNELDUMP) $(DISK)

run: build
	$(QEMU) $(QEMUOPTS)

gdb: build_all .gdbinit-$(ARCH)
	$(QEMU) $(QEMUOPTS) -S -gdb tcp::9877


.PHONY: all clean distclean build run gdb disk temp mount umount
