#ifndef __MEMLAYOUT_H__
#define __MEMLAYOUT_H__

/* ======= RISC-V Linux Kernel SV39 ======= */

#define USER_SPACE_START    0x0000000000000000UL
#define USER_SPACE_END      0x0000003fffffffffUL
#define USER_SPACE_SIZE     0x0000004000000000UL // 256 GB

// PS: USER_SPACE_SIZE is half of Sv39 Virtual Address Space Size
// Virtual address must be sign-extended, which means VA[63:39] must be equals to VA[38]

#define NON_CANONICAL_START 0x0000004000000000UL
#define NON_CANONICAL_END   0xffffffbfffffffffUL


/* ======= Kernel Space ======= */

/* fixmap area */
#define FIXMAP_START    0xffffffc6fea00000UL
#define FIXMAP_END      0xffffffc6feffffffUL
#define FIXMAP_SIZE     0x0000000000600000UL // 6 MB

/* PCI I/O remap */
#define PCI_IO_START    0xffffffc6ff000000UL
#define PCI_IO_END      0xffffffc6ffffffffUL
#define PCI_IO_SIZE     0x0000000001000000UL // 16 MB

/* virtual memory map (sparsemem vmemmap) */
#define VMEMMAP_START   0xffffffc700000000UL
#define VMEMMAP_END     0xffffffc7ffffffffUL
#define VMEMMAP_SIZE    0x0000000100000000UL // 4 GB

/* vmalloc / ioremap area */
#define VMALLOC_START   0xffffffc800000000UL
#define VMALLOC_END     0xffffffd7ffffffffUL
#define VMALLOC_SIZE    0x0000001000000000UL // 64 GB

/* direct mapping of all physical memory */
#define DIRECTMAP_START 0xffffffd800000000UL
#define DIRECTMAP_END   0xfffffff6ffffffffUL

/* kasan */
#define KASAN_START     0xfffffff700000000UL
#define KASAN_END       0xfffffffeffffffffUL

/* kernel */
#define KERNEL_START    0xffffffff80000000UL
#define KERNEL_END      0xffffffffffffffffUL


#endif // __MEMLAYOUT_H__