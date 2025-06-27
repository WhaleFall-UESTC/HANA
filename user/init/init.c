#include <stddef.h>
#include "../include/syscall.h"

// 汇编包装函数声明
long sys_execve(const char *pathname, char *const argv[], char *const envp[]);

int main(void) {
    // const char *path = "/test";
    // char *const argv[] = { NULL };
    // char *const envp[] = { NULL };
    
    // sys_execve(path, argv, envp);

    printf("hello, world!\n");
    
    while (1) {

    }
}

#if defined(__riscv)
__attribute__((naked)) long sys_execve(const char *pathname, char *const argv[], char *const envp[]) {
    __asm__ volatile (
        "li a7, %0\n\t"          // 加载系统调用号
        "ecall\n\t"               // 执行系统调用
        "ret\n\t"                 // 返回（理论上不会执行）
        : 
        : "i" (SYS_execve)
        : "memory", "a7"
    );
}
#elif defined(__loongarch64)
__attribute__((naked)) long sys_execve(const char *pathname, char *const argv[], char *const envp[]) {
    __asm__ volatile (
        "ori $a7, $zero, %0\n\t" // 加载系统调用号到a7
        "syscall 0\n\t"          // 执行系统调用
        "jr $ra\n\t"             // 返回
        : 
        : "i" (SYS_execve)
        : "memory", "$a7"
    );
}
#else
    #error "Unsupported architecture"
#endif
