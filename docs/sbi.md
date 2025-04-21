# OpenSBI
OpenSBI 是 qemu 默认的固件，在位于 0x1000 的 ROM 代码结束之后，就会来到 0x80000000 即 OpenSBI 被加载到的位置。OpenSBI 做了初始化操作之后，便会跳到 0x80200000 处，也就是内核加载的地址，进入 _start
<br><br>
除了初始化操作之外，OpenSBI 还起到了内核与机器之间交互的中间层作用，其方式是通过 S mode 下的 ecall，比如当我们希望通过 SBI 设置下一个时钟中断到来的时间，可以将时间放入 a1 寄存器，将 timer 的设备 id（ext_id）放入 a7，将函数 id（func_id，set_timer 对应的值是 0）放入 a6，然后 ecall，这样就可以委托 OpenSBI  完成 sbi_set_timer(uint64 time) 的操作
<br><br>
所以 sbi 提供的接口的原型都是 sbi_ecall
```C
sbiret_t 
sbi_ecall(int ext_id, int func_id, 
    uint64 arg0, uint64 arg1, uint64 arg2,
    uint64 arg3, uint64 arg4, uint64 arg5)
{
    sbiret_t ret;

    register uint64 a0 asm("a0") = arg0;
    register uint64 a1 asm("a1") = arg1;
    register uint64 a2 asm("a2") = arg2;
    register uint64 a3 asm("a3") = arg3;
    register uint64 a4 asm("a4") = arg4;
    register uint64 a5 asm("a5") = arg5;
    register uint64 a6 asm("a6") = func_id;
    register uint64 a7 asm("a7") = ext_id;

    asm volatile(
        "ecall"
        : "+r"(a0), "=r"(a1)
        : "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5),
          "r"(a6), "r"(a7)
        : "memory"
    );

    ret.error = a0;
    ret.value = a1;
    return ret;
}
```
<br>

sbiret_t 是一个结构体，包括一个返回值以及错误状态
```C
struct sbiret {
    long error;
    long value;
};
typedef struct sbiret sbiret_t;
```