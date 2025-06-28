#include <common.h>
#include <klib.h>
#include <debug.h>
#include <arch.h>
#include <irq/interrupt.h>
#include <trap/context.h>
#include <proc/proc.h>
#include <trap/trap.h>
#include <proc/sched.h>
#include <mm/buddy.h>
#include <mm/mm.h>
#include <mm/memlayout.h>
#include <fs/file.h>
#include <fs/fcntl.h>
#include <fs/kernel.h>
#include <syscall.h>
#include <elf.h>

void test_execve() {
    kernel_mount("/dev/sda", "/", "ext4", 0, NULL);

    struct file* file = kernel_open("/init");

    Elf64_Ehdr elf = {};
    kernel_read(file, &elf, sizeof(Elf64_Ehdr));

    Elf64_Phdr phdr = {};

    kernel_lseek(file, elf.e_phoff, SEEK_SET);
    for (int i = 0, off = elf.e_phoff; i < elf.e_phnum; i++, off += sizeof(Elf64_Phdr)) {
        kernel_lseek(file, off, SEEK_SET);
        kernel_read(file, &phdr, sizeof(Elf64_Ehdr));
        log("InGot phdr type=%u, flags=%u, off=%lx, va=%lx, filesz=%lx, memsz=%lx", 
            phdr.p_type, phdr.p_flags, phdr.p_offset, phdr.p_vaddr, phdr.p_filesz, phdr.p_memsz);
    }

    kernel_close(file);
}