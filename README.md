# HANA

HANA是一个基于 xv6 开发的宏内核操作系统，支持 RISC-V 和 LoongArch 两种架构。它实现了从底层硬件初始化到用户进程管理、文件系统、网络栈、设备驱动等完整的操作系统功能，具备类 Unix 的多任务并发执行能力，并提供了丰富的系统调用接口供用户程序使用

<br/>

## 项目结构

```
├── docs                        项目文档
│   └── img               
├── kernel                      内核代码
│   ├── arch                    架构相关代码
│   │   ├── loongarch           loongarch 相关代码
│   │   │   ├── boot            loongarch 启动
│   │   │   ├── drivers         loongarch 驱动
│   │   │   ├── include         
│   │   │   ├── mm              loongarch 内存管理模块
│   │   │   ├── proc            loongarch 进程管理
│   │   │   └── trap            异常处理
│   │   └── riscv               riscv 相关代码
│   │       ├── boot            
│   │       ├── drivers         
│   │       ├── include
│   │       │   ├── drivers
│   │       │   ├── mm
│   │       │   ├── proc
│   │       │   ├── sbi
│   │       │   └── trap
│   │       ├── mm
│   │       ├── proc
│   │       ├── sbi
│   │       └── trap
│   ├── drivers                 驱动代码
│   ├── fs                      文件系统
│   ├── include
│   ├── io
│   ├── irq                     中断
│   ├── lib                     klib
│   ├── locking                 锁
│   ├── mm                      内存管理
│   ├── net                     网络栈
│   ├── proc                    进程管理
│   ├── sys 
│   └── test
└── user                        用户代码                      
```