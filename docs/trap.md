# Trap

Trap系统负责处理处理器中断和异常，实现用户态与内核态之间的安全切换。系统由以下核心组件组成：

1. **Trap入口处理**
   - `kernelvec`：内核态trap入口点
   - `uservec/userret`：用户态trap入口/出口
2. **Trap分发机制**
   - `trap.c` 使用中断/异常处理向量表实现核心分发逻辑
3. **上下文管理**
   - 陷阱帧（trapframe）保存处理器状态
4. **系统调用处理**
   - 详细请见 kernel

<br/>

## 初始化

### register_trap_handler(int interrupt, int code, void* function)

用来注册中断/异常处理函数的接口。interrupt 表示注册的类型，如果是中断，那么其值为 1 （INTERRUPT），若为异常则是 0 （EXCEPTION）。code 为异常号，在 riscv.h 里面定义，function 则是异常处理函数的地址

在 trap.c 中有两个全局变量管理所有的处理函数，interrupt/exception_handler_vector，类型为自定义的 handler（typedef void (handler*)()）

可以说所有的异常都委托到了 S mode, 所有 S mode 的中断都委托到了 S mode（在未配置的情况下，所有的 trap 都由 M mode 处理）。但是，timer 被配置为触发 M 中断，没有委托，故而先由 M mode 的 timervec 处理，再通过触发 S mode 软中断的方式通知内核；PLIC 则被设置为触发 S 中断，这个是被委托到 S mode 处理的S

如果需要注册，在 trap_init 里面调用 register_trap_handler 就好，异常号见 trap.h 里面的两个 enum

```C
void trap_init() {
    for (int i = 0; i < NR_EXCEPTION; i++)
        exception_handler_vector[i] = NULL;
    for (int i = 0; i < NR_INTERRUPT; i++)
        interrupt_handler_vector[i] = NULL;

    // register handler
    register_trap_handler(INTERRUPT, TI, timer_isr);
    register_trap_handler(INTERRUPT, HWI0, irq_response);

    register_trap_handler(EXCEPTION, PME, store_page_fault_handler);
    register_trap_handler(EXCEPTION, PIL, page_unmap_handler);
    register_trap_handler(EXCEPTION, PIS, page_unmap_handler);
    register_trap_handler(EXCEPTION, SYS, syscall);
}
```

<br/>

## 核心处理流程

### 内核态trap处理流程

```
1. 发生trap → 2. kernelvec 保存寄存器到内核栈 → 3. 调用kernel_trap() → 4. 调用trap() 分发到具体处理程序
→ 5.返回 kernelvec 读取寄存器并 sret/ertn 到中断点
```

### 用户态trap处理流程

```
1. 发生trap → 2. uservec 保存用户寄存器 → 3. 切换到内核栈，读取 user_trap() 地址（riscv 切换回内核栈）
→ 4. 跳转 user_trap() → 5. trap() 分发处理 → 6. dive_to_user
→ 7. userret 保存内核寄存器并恢复用户寄存器 → 8. sret/ertn 到用户程序
```