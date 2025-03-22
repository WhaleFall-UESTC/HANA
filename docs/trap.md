# Trap

## trap.c
### register_trap_handler(int interrupt, int code, void* function)
用来注册中断/异常处理函数的接口。interrupt 表示注册的类型，如果是中断，那么其值为 1 （INTERRUPT），若为异常则是 0 （EXCEPTION）。code 为异常号，在 riscv.h 里面定义，function 则是异常处理函数的地址
<br><br>
在 trap.c 中有两个全局变量管理所有的处理函数，interrupt/exception_handler_vector，类型为自定义的 handler（typedef void (handler*)()）
