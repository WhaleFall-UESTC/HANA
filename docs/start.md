# 花。关于 HANA 的启动

HANA 是一个支持 riscv64 与 loongarch64 两种架构的宏内核操作系统。本篇将讲述在 uboot 将控制权交给内核，从 entrypoint 到 main 函数再到调用 scheduler 开始运行任务的过程中 HANA 所做的操作。这里只是简要介绍启动的流程，详细的配置与原因请见对应模块的文档。

由于这一部分与架构息息相关，故分为 riscv 与 loongarch 两个部分来讲。

<br/>

## start

### riscv

代码位于 kernel/arch/riscv/boot/start.S

```
# qemu -kernel will load kernel at 0x80200000
# and causes each CPU to jump there
.section .text._start
.global _start
_start:
    addi tp, a0, 0

    # set initial stack
    la sp, init_stack
    li t0, 8192
    mul a0, tp, t0
    add a0, a0, t0
    add sp, sp, a0

    call timer_init

    j main

```

qemu -bios default 默认使用 OpenSBI 作为固件平台，并在启动时完成了一些初始化操作，最终将控制转交给内核，将 pc 置为 0x80200000。因此，我们在链接脚本中将 _start 链接到 0x80200000 作为内核的入口代码

由于 OpenSBI 已经完成了从 M-mode 的切换，委托部分异常与中断到 S mode，以及对 S-mode 对物理内存访问的设置等工作，所以我们在 _start 中只需要为每一个核准备一个栈空间（在 kernel/main.c 定义），初始化时钟，然后跳转到 main 函数即可

<br/>

### loongarch

代码位于 kernel/arch/loongarch/boot 下的 _start.S 与 start.c

```
.extern init_stack
.extern start
.section .text._start
.global _start
_start:
    # initialize stack
    la      $sp, init_stack 
    csrrd   $tp, 0x20  // CSR_CPUID
    li.d    $t0, 4096
    addi.d  $a0, $tp, 1
    mul.d   $a0, $a0, $t0
    add.d   $sp, $sp, $a0

    bl start

spin:
    b spin
```

_start 被 kernel/arch/loongarch/kernel.ld 链接到了 0x9000000000200000。在完成了栈的初始化操作之后，跳转到 start 

```c_cpp
void start() {
    /* set direct mapping windows */
    /* in PLV0, map 0x9000_0000_0000_0000 - 0x9000_FFFF_FFFF_FFFF to
           0x0 - 0xFFFF_FFFF_FFFF 
       MAT = CC (Coherent Cache)
       if virtual address is not in the range, it will be translated by page table
    */
    w_csr_dmw0(DMW_MASK | CSR_DMW_PLV0 | CSR_DMW_MAT_CC);
    w_csr_dmw1(0);
    w_csr_dmw2(0);
    w_csr_dmw3(0);

    // set current mode PLV0 & disable global interrupt
    uint64 crmd = (CSR_CRMD_PLV0 & ~CSR_CRMD_IE);
    // Enable address mapping
    crmd |= CSR_CRMD_PG;
    // when MMU in direct translation mode, 
    // the instruction access type depends on DATF, load/store access type depends on DATM
    // which is set to CC (Coherent Cache).
    // In mapping mode, if instruction/load/store address is in one of Direct Mapping Windows,
    // the access type depends on DMW MAT
    // else, it depends on page table entry MAT
    crmd |= (CSR_CRMD_DATF_CC | CSR_CRMD_DATM_CC);
    w_csr_crmd(crmd);

    invtlb();

    timer_init();

    main();

    panic("_start should never return");
}
```

首先设置 Direct Mapping Windows，在 PLV0 下，将从 0x9000000000000000 开始的一段虚拟地址映射到 0x0 开始的一段物理地址。接着设置当前模式为 PLV0（内核模式），关闭中断。接下来针对内存访问进行设置：首先开启地址映射模式，使用 DMW（直接映射窗口）与页表进行地址翻译，接着设置取指与访存的存储访问类型位一致可缓存，最后刷新 tlb 完成配置。在上述配置写入之后，start 调用 timer_init 初始化计时器，最终跳转到 main 函数

<br/>

<br/>

## main

main 承担了各个模块的初始化操作：串口初始化，启用打印输出功能；物理内存分配器，与虚拟内存管理的初始化；中断异常处理初始化； init 进程初始化；块设备初始化；虚拟文件系统初始化。最后调用 scheduler 调度 RUNNABLE 状态的 init 进程，开启下一个加阶段的运行

新关于各个模块初始化的详细过程，以及 init 被调度之后又作了什么事情，请参照相关文档的详细说明
