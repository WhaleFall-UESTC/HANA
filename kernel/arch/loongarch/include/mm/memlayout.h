#ifndef __MEMLAYOUT_H__
#define __MEMLAYOUT_H__

#include <platform.h>

#define DMW_MASK    0x9000000000000000UL

#define KERNEL_BASE 0x200000UL
#define RAM_BASE    0x90000000UL

#define KERNELBASE  (DMW_MASK | KERNEL_BASE)
// #define KERNELBASE   DMW_MASK
// #define RAMBASE     (DMW_MASK | RAM_BASE)
#define RAMTOP      (DMW_MASK | 0x7ffffffUL)
#define PHYSTOP     RAMTOP

#define IN_RAM(addr) (((uint64)(addr) >= KERNELBASE) && ((uint64)(addr) < RAMTOP))

#define KERNEL_PA2VA(pa) (((uint64)(pa)) | DMW_MASK)
#define KERNEL_VA2PA(va) (((uint64)(va)) & ~DMW_MASK)

#define UART0       (DMW_MASK | UART0_BASE)

#define LS7A1000    (DMW_MASK | LS7A1000_BASE)

#define LS7A_INT_MASK       (LS7A1000 | LS7A1000_INT_MASK)
#define LS7A_INTEDGE        (LS7A1000 | LS7A1000_INTEDGE)
#define LS7A_INTCLR         (LS7A1000 | LS7A1000_INTCLR)
#define LS7A_HTMSI_VECTOR0  (LS7A1000 | LS7A1000_HTMSI_VECTOR0)
#define LS7A_INTISR         (LS7A1000 | LS7A1000_INTISR)
#define LS7A_INT_POLARITY   (LS7A1000 | LS7A1000_INT_POLARITY)

#define TRAMPOLINE  0xFFFFFFFFFFFFF000UL
// #define TRAPFRAME   (TRAMPOLINE - PGSIZE)

#define MAXVA (1UL << (12 + 9 + 9 + 9 - 1))
#define TRAPFRAME (MAXVA - PGSIZE)

#define KSTACK(n)   (TRAMPOLINE - (2 * (n)) * PGSIZE)


/*
info mtree
address-space: cpu-memory-0
address-space: memory
  0000000000000000-ffffffffffffffff (prio 0, i/o): system
    0000000000000000-0000000007ffffff (prio 0, ram): alias loongarch.lowram @loongarch.ram 0000000000000000-0000000007ffffff
    0000000010000000-00000000100000ff (prio 0, i/o): loongarch_pch_pic.reg32_part1
    0000000010000100-000000001000039f (prio 0, i/o): loongarch_pch_pic.reg8
    00000000100003a0-0000000010000fff (prio 0, i/o): loongarch_pch_pic.reg32_part2
    000000001001041c-000000001001041f (prio -1000, i/o): pci-dma-cfg
    00000000100d0100-00000000100d01ff (prio 0, i/o): ls7a_rtc
    00000000100e0000-00000000100e0003 (prio 0, i/o): acpi-ged
    00000000100e0004-00000000100e001b (prio 0, i/o): memhp container
    00000000100e001c-00000000100e001e (prio 0, i/o): acpi-ged-regs
    0000000016000000-0000000017ffffff (prio 0, i/o): platform bus
    0000000018004000-000000001800ffff (prio 0, i/o): alias pcie-io @gpex_ioport_window 0000000000004000-000000000000ffff
    000000001c000000-000000001cffffff (prio 0, romd): virt.flash0
    000000001d000000-000000001dffffff (prio 0, romd): virt.flash1
    000000001e020000-000000001e020007 (prio 0, i/o): fwcfg.data
    000000001e020008-000000001e020009 (prio 0, i/o): fwcfg.ctl
    000000001e020010-000000001e020017 (prio 0, i/o): fwcfg.dma
    000000001fe001e0-000000001fe001e7 (prio 0, i/o): serial
    000000001fe002e0-000000001fe002e7 (prio 0, i/o): serial
    000000001fe003e0-000000001fe003e7 (prio 0, i/o): serial
    000000001fe004e0-000000001fe004e7 (prio 0, i/o): serial
    0000000020000000-0000000027ffffff (prio 0, i/o): alias pcie-ecam @pcie-mmcfg-mmio 0000000000000000-0000000007ffffff
    000000002ff00000-000000002ff00007 (prio 0, i/o): loongarch_pch_msi
    0000000040000000-000000007fffffff (prio 0, i/o): alias pcie-mmio @gpex_mmio_window 0000000040000000-000000007fffffff

address-space: virtio-net-pci
  0000000000000000-ffffffffffffffff (prio 0, i/o): bus master container

address-space: gpex-root
  0000000000000000-ffffffffffffffff (prio 0, i/o): bus master container

address-space: virtio-pci-cfg-mem-as
  0000000000000000-0000000000003fff (prio 0, i/o): virtio-pci
    0000000000000000-0000000000000fff (prio 0, i/o): virtio-pci-common-virtio-net
    0000000000001000-0000000000001fff (prio 0, i/o): virtio-pci-isr-virtio-net
    0000000000002000-0000000000002fff (prio 0, i/o): virtio-pci-device-virtio-net
    0000000000003000-0000000000003fff (prio 0, i/o): virtio-pci-notify-virtio-net

address-space: I/O
  0000000000000000-000000000000ffff (prio 0, i/o): io

address-space: IOCSR
  0000000000000000-ffffffffffffffff (prio 0, i/o): iocsr
    0000000000000000-0000000000000427 (prio 0, i/o): iocsr_misc
    0000000000001000-0000000000001047 (prio 0, i/o): loongson_ipi_iocsr
    0000000000001048-000000000000115f (prio 0, i/o): loongson_ipi64_iocsr
    0000000000001400-0000000000001cff (prio 0, i/o): extioi_system_mem
    0000000040000000-0000000040000fff (prio 0, i/o): extioi_virt

memory-region: loongarch.ram
  0000000000000000-0000000007ffffff (prio 0, ram): loongarch.ram

memory-region: gpex_ioport_window
  0000000000000000-000000000000ffff (prio 0, i/o): gpex_ioport_window
    0000000000000000-000000000000ffff (prio 0, i/o): gpex_ioport

memory-region: pcie-mmcfg-mmio
  0000000000000000-000000000fffffff (prio 0, i/o): pcie-mmcfg-mmio

memory-region: gpex_mmio_window
  0000000000000000-ffffffffffffffff (prio 0, i/o): gpex_mmio_window
    0000000000000000-ffffffffffffffff (prio 0, i/o): gpex_mmio

memory-region: system
  0000000000000000-ffffffffffffffff (prio 0, i/o): system
    0000000000000000-0000000007ffffff (prio 0, ram): alias loongarch.lowram @loongarch.ram 0000000000000000-0000000007ffffff
    0000000010000000-00000000100000ff (prio 0, i/o): loongarch_pch_pic.reg32_part1
    0000000010000100-000000001000039f (prio 0, i/o): loongarch_pch_pic.reg8
    00000000100003a0-0000000010000fff (prio 0, i/o): loongarch_pch_pic.reg32_part2
    000000001001041c-000000001001041f (prio -1000, i/o): pci-dma-cfg
    00000000100d0100-00000000100d01ff (prio 0, i/o): ls7a_rtc
    00000000100e0000-00000000100e0003 (prio 0, i/o): acpi-ged
    00000000100e0004-00000000100e001b (prio 0, i/o): memhp container
    00000000100e001c-00000000100e001e (prio 0, i/o): acpi-ged-regs
    0000000016000000-0000000017ffffff (prio 0, i/o): platform bus
    0000000018004000-000000001800ffff (prio 0, i/o): alias pcie-io @gpex_ioport_window 0000000000004000-000000000000ffff
    000000001c000000-000000001cffffff (prio 0, romd): virt.flash0
    000000001d000000-000000001dffffff (prio 0, romd): virt.flash1
    000000001e020000-000000001e020007 (prio 0, i/o): fwcfg.data
    000000001e020008-000000001e020009 (prio 0, i/o): fwcfg.ctl
    000000001e020010-000000001e020017 (prio 0, i/o): fwcfg.dma
    000000001fe001e0-000000001fe001e7 (prio 0, i/o): serial
    000000001fe002e0-000000001fe002e7 (prio 0, i/o): serial
    000000001fe003e0-000000001fe003e7 (prio 0, i/o): serial
    000000001fe004e0-000000001fe004e7 (prio 0, i/o): serial
    0000000020000000-0000000027ffffff (prio 0, i/o): alias pcie-ecam @pcie-mmcfg-mmio 0000000000000000-0000000007ffffff
    000000002ff00000-000000002ff00007 (prio 0, i/o): loongarch_pch_msi
    0000000040000000-000000007fffffff (prio 0, i/o): alias pcie-mmio @gpex_mmio_window 0000000040000000-000000007fffffff
*/

#endif