# MM 内存管理

这一部分负责的是物理内存与虚拟地址的管理，目前已完成物理内存部分

## kalloc.c
### void* kalloc(uint64 sz)
提供需要的内存大小（单位字节），返回一个指针，指向一段连续的物理内存
<br><br>
背后的实现是使用了伙伴系统和 SLAB 分配器。如果 sz > 3904，那么会使用伙伴系统分配，伙伴系统只会分配 4KB, 8KB, 16KB, 32KB, 64KB ... 4MB 大小的连续物理内存；反之，则会使用 SLAB 分配器。SLAB 分配器会向伙伴系统申请 4KB 大小的块，称为 slab，又将 slab 分成数个小块，称为 object。我自己实现了一个 SLAB，每个 object 大小为 64B，其中又拿出 192B 的空间在 slab 底部存放元数据，所以一个 slab 可以分配 61 个 object（所以区分用什么分配器的时候，不是以 PGSIZE 判断，而是 61 * 64B）。SLAB 会以 object 为基本单位分配内存，实际分配的大小是 64B 的倍数
<br><br>

### void kfree(void* addr)
传入用 kalloc 返回的指针，kfree 会自动识别是以那种方式分配的，然后自动获取需要释放的大小，调用对应的释放函数
<br><br>
区分的方式是，伙伴系统分配而返回的指针一定是 PGALIGNED 的，而 slab 的顶部放的是元数据，而 object 的起始地址在元数据之后，所以返回的地址并不页对齐
<br><br>
对于伙伴系统，在内存中存放着 page 结构体的数组，管理内存中每个页的状态，其中的 order 指定了伙伴系统分配空间的大小（PGSIZE << order）；SLAB 分配的大小可以通过元数据知晓
<br><br>

## vm.c
### pte_t* walk(pagetable_t pgtbl, uint64 va, int alloc);
这个函数传入页表（L2）的地址，一个虚拟地址，以及一个是否分配的标志，返回此虚拟地址在页表中的 PTE 的地址。当选择 alloc 的时候，如果此虚拟地址在页表中尚未记录，则会自动完善页表，若此时 alloc 为 0, 那么 walk 会返回 NULL
<br><br>
pte_t 的本质是 uint64，pagetable_t 的本质是 pte_t 的数组（uint64*）。这个系统采用 Sv39，一张页表的大小为 PGSIZE，共有 512 条 PTE
若此时 alloc 为 0, 那么 walk 会返回 NULL
<br><br>

### void mappages(pagetable_t pgtbl, uint64 va, uint64 pa, uint64 sz, int flag);
功能如其名：在给定的页表中分配 va 与 pa 的映射关系，并设定所分配页的权限