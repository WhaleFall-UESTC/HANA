# Trap

## trap.c
### register_trap_handler(int interrupt, int code, void* function)
用来注册中断/异常处理函数的接口。interrupt 表示注册的类型，如果是中断，那么其值为 1 （INTERRUPT），若为异常则是 0 （EXCEPTION）。code 为异常号，在 riscv.h 里面定义，function 则是异常处理函数的地址
<br><br>
在 trap.c 中有两个全局变量管理所有的处理函数，interrupt/exception_handler_vector，类型为自定义的 handler（typedef void (handler*)()）
<br><br>
可以说所有的异常都委托到了 S mode, 所有 S mode 的中断都委托到了 S mode（在未配置的情况下，所有的 trap 都由 M mode 处理）。但是，timer 被配置为触发 M 中断，没有委托，故而先由 M mode 的 timervec 处理，再通过触发 S mode 软中断的方式通知内核；PLIC 则被设置为触发 S 中断，这个是被委托到 S mode 处理的S
<br><br>
如果需要注册，在 trap_init 里面调用 register_trap_handler 就好，异常号见 trap.h 里面的两个 enum
<br><br>
补充：现在换成了 OpenSBI 启动，只有部分中断异常委托到了 S mode



