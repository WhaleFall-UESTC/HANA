.section .text
.globl _start
_start:
    # 挂载根文件系统
    auipc a0, 0x0             # 计算/dev/sda字符串地址
    addi a0, a0, 56           # 偏移量指向字符串"/dev/sda"
    auipc a1, 0x0             # 计算挂载点根目录"/"字符串地址
    addi a1, a1, 65           # 偏移量指向字符串"/"
    auipc a2, 0x0             # 计算文件系统类型"ext4"字符串地址
    addi a2, a2, 67           # 偏移量指向字符串"ext4"
    li a3, 0                  # 挂载标志flags=0
    li a4, 0                  # data参数=NULL
    li a7, 40                 # mount系统调用号=40
    ecall                     # 执行系统调用

    # 执行/init程序
    auipc a0, 0x0             # 计算/init字符串地址
    addi a0, a0, 72           # 偏移量指向字符串"/init"
    addi sp, sp, -16          # 栈上分配16字节空间
    sd a0, 0(sp)              # 将/init地址存入栈中（argv[0]）
    sd zero, 8(sp)            # argv[1]=NULL
    mv a1, sp                 # a1=argv数组地址
    li a2, 0                  # envp=NULL
    li a7, 221                # execve系统调用号=221
    ecall                     # 执行系统调用

1:  j 1b                      # 死循环（防止execve失败）

# 字符串数据（严格保持顺序）
.string "/dev/sda"            # 偏移量56
.string "/"                   # 偏移量65（56+9）
.string "ext4"                # 偏移量67（65+2）
.string "/init"               # 偏移量72（67+5）


# This is compiled to:
# 000000 17 05 00 00 13 05 85 03 97 05 00 00 93 85 15 04
# 000010 17 06 00 00 13 06 36 04 93 06 00 00 13 07 00 00
# 000020 93 08 80 02 73 00 00 00 17 05 00 00 13 05 85 04
# 000030 13 01 01 ff 23 30 a1 00 23 34 01 00 93 05 01 00
# 000040 13 06 00 00 93 08 d0 0d 73 00 00 00 6f 00 00 00
# 000050 2f 64 65 76 2f 73 64 61 00 2f 00 65 78 74 34 00
# 000060 2f 69 6e 69 74 00 01 00
# 000068
#   following the string
