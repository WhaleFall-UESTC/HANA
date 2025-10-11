PREFIX := riscv64-unknown-elf-

CC := $(PREFIX)gcc
AR := $(PREFIX)ar

DISKSZ := 1G
FS := ext4

SRC_DIR := src
LIB_DIR := lib
INC_DIR := include
BUILD_DIR := build
DATA_DIR := data
IMG_NAME := disk.img
OUT_IMG := ../$(IMG_NAME)

CFLAGS = -I$(INC_DIR) -Wall -Wextra -g -O0 -static -ffreestanding -nostdlib
LDFLAGS = -e main

SRCS = $(wildcard $(SRC_DIR)/*.c)
LIB_SRCS = $(wildcard $(LIB_DIR)/*.c)
LIB_OBJS = $(patsubst $(LIB_DIR)/%.c,$(BUILD_DIR)/%.o,$(LIB_SRCS))
LIB_A = $(BUILD_DIR)/libmylib.a

BINS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%,$(SRCS))

.PHONY: all disk clean

all: $(OUT_IMG)

$(BUILD_DIR)/%: $(SRC_DIR)/%.c $(LIB_A) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -L$(BUILD_DIR) -o $@ $< -lmylib

$(BUILD_DIR)/%.o: $(LIB_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(LIB_A): $(LIB_OBJS) | $(BUILD_DIR)
	$(AR) rcs $@ $^

$(BUILD_DIR):
	mkdir -p $@

$(IMG_NAME): $(BINS) | $(BUILD_DIR)
	qemu-img create -f raw $(IMG_NAME) 2G
	mkfs.$(FS) $(IMG_NAME)
	@LODEV=$$(sudo losetup -f --show $(IMG_NAME)); \
	sudo mkdir -p /tmp/hana_mount; \
	sudo mount $$LODEV /tmp/hana_mount; \
	for bin in $(BINS); do \
		sudo cp $$bin /tmp/hana_mount/$$(basename $$bin); \
	done; \
	if [ -d "$(DATA_DIR)" ]; then \
		sudo mkdir -p /tmp/hana_mount/data; \
		for f in $(DATA_DIR)/*; do \
			if [ -e "$$f" ]; then \
				sudo cp -r $$f /tmp/hana_mount/data/; \
			fi \
		done \
	fi; \
	sudo umount /tmp/hana_mount; \
	sudo rmdir /tmp/hana_mount; \
	sudo losetup -d $$LODEV

$(OUT_IMG): $(IMG_NAME)
	mv $< $@

disk: $(OUT_IMG)

clean:
	rm -rf $(BUILD_DIR) $(IMG_NAME) $(OUT_IMG)