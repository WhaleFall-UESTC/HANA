#pragma once

#define PALEN 48
#define VALEN 48
#define MAXVA (1UL << (9 + 12 - 1)) // lower half virtual address

typedef uint64 pte_t;
typedef pte_t *pagetable_t;

#define PTE_V   (1UL << 0)
#define PTE_D   (1UL << 1)
#define PTE_PLV (3UL << 2)
#define PTE_PLV0 (0UL << 2)
#define PTE_PLV3 (3UL << 2)
#define PTE_MAT (3L << 4)
#define PTE_MAT_SUC (0UL << 4)
#define PTE_MAT_CC  (1UL << 4)
#define PTE_MAT_WUC (2UL << 4)
#define PTE_G   (1UL << 6)
#define PTE_P   (1UL << 7)
#define PTE_W   (1UL << 8)
#define PTE_NR  (1UL << 61)
#define PTE_NX  (1UL << 62)
#define PTE_RPLV (1UL << 63)

#define PAMASK  (0xFFFFFFFFFUL << PGSHIFT)
#define PTE2PA(pte) (((uint64)(pte)) & PAMASK)
#define PA2PTE(pa)  (pte_t)(((uint64)(pa)) & PAMASK)
#define PTE_FLAGS(pte) ((pte) & 0xE0000000000001FFUL)

#define DIRWIDTH    9U
#define PTBASE      12U
#define DIRBASE(n) (PTBASE + n * DIRWIDTH)


/* 基础控制寄存器 */
#define CSR_CRMD        0x0    // 当前模式信息
#define CSR_PRMD        0x1    // 例外前模式信息
#define CSR_EUEN        0x2    // 扩展部件使能
#define CSR_MISC        0x3    // 杂项控制
#define CSR_ECFG        0x4    // 例外配置
#define CSR_ESTAT       0x5    // 例外状态
#define CSR_ERA         0x6    // 例外返回地址
#define CSR_BADV        0x7    // 出错虚地址
#define CSR_BADI        0x8    // 出错指令
#define CSR_EENTRY      0xC    // 例外入口地址

/* TLB相关 */
#define CSR_TLBIDX      0x10   // TLB索引
#define CSR_TLBEHI      0x11   // TLB表项高位
#define CSR_TLBELO0     0x12   // TLB表项低位0
#define CSR_TLBELO1     0x13   // TLB表项低位1
#define CSR_ASID        0x18   // 地址空间标识符

/* 地址空间配置 */
#define CSR_PGDL        0x19   // 低半地址全局目录基址
#define CSR_PGDH        0x1A   // 高半地址全局目录基址
#define CSR_PGD         0x1B   // 全局目录基址
#define CSR_PWCL        0x1C   // 页表遍历控制低半
#define CSR_PWCH        0x1D   // 页表遍历控制高半
#define CSR_STLBPS      0x1E   // STLB页大小
#define CSR_RVACFG      0x1F   // 缩减虚地址配置

/* 处理器信息 */
#define CSR_CPUID       0x20   // 处理器编号
#define CSR_PRCFG1      0x21   // 特权资源配置1
#define CSR_PRCFG2      0x22   // 特权资源配置2
#define CSR_PRCFG3      0x23   // 特权资源配置3

/* 数据保存寄存器组 (0x30-0x3F) */
#define CSR_SAVE0       0x30   // 数据保存0
// ...
#define CSR_SAVE15      0x3F   // 数据保存15

/* 定时器系统 */
#define CSR_TID         0x40   // 定时器编号
#define CSR_TCFG        0x41   // 定时器配置
#define CSR_TVAL        0x42   // 定时器值
#define CSR_CNTC        0x43   // 计时器补偿
#define CSR_TICLR       0x44   // 定时中断清除

/* TLB重填例外 */
#define CSR_TLBRENTRY   0x88   // TLB重填入口地址
#define CSR_TLBRBADV    0x89   // TLB重填出错虚地址
#define CSR_TLBRERA     0x8A   // TLB重填返回地址
#define CSR_TLBRSAVE    0x8B   // TLB重填数据保存
#define CSR_TLBRELO0    0x8C   // TLB重填表项低位0
#define CSR_TLBRELO1    0x8D   // TLB重填表项低位1
#define CSR_TLBREHI     0x8E   // TLB重填表项高位
#define CSR_TLBRPRMD    0x8F   // TLB重填前模式信息

/* 直接映射窗口 (0x180-0x183) */
#define CSR_DMW0        0x180  // 直接映射窗口0
#define CSR_DMW1        0x181  // 直接映射窗口1
#define CSR_DMW2        0x182  // 直接映射窗口2
#define CSR_DMW3        0x183  // 直接映射窗口3

/* 性能监测系统 (0x200-0x25F) */
#define CSR_PMCFG0      0x200  // 性能监测配置0
#define CSR_PMCNT0      0x201  // 性能监测计数器0
// ...
#define CSR_PMCFG31     0x25E  // 性能监测配置31
#define CSR_PMCNT31     0x25F  // 性能监测计数器31

/* 其他关键寄存器 */
#define CSR_LLBCTL      0x60   // LLBit控制
#define CSR_IMPCTL1     0x80   // 实现相关控制1
#define CSR_IMPCTL2     0x81   // 实现相关控制2
#define CSR_CTAG        0x98   // 高速缓存标签
#define CSR_MSGIR       0xA4   // 消息中断请求
#define CSR_MSGIE       0xA5   // 消息中断使能




/* LoongArch CSR 寄存器位定义 */
// RW: 读写
// R:  只读
// R0: 软件读返回 0
// W1: 写 1 有效

/* 当前模式信息 (CRMD) */
#define CSR_CRMD_PLV   (0x3 << 0)  // 当前特权级 RW
#define CSR_CRMD_PLV0  0x0
#define CSR_CRMD_PLV1  0x1
#define CSR_CRMD_PLV2  0x2
#define CSR_CRMD_PLV3  0x3
#define CSR_CRMD_IE    (0x1 << 2)  // 全局中断使能 RW
#define CSR_CRMD_DA    (0x1 << 3)  // 直接地址翻译模式使能 RW
#define CSR_CRMD_PG    (0x1 << 4)  // 映射地址翻译模式使能 RW
#define CSR_CRMD_DATF  (0x3 << 5)  // 直接地址翻译模式下取指操作存储访问类型 RW
#define CSR_CRMD_DATF_SUC (0x0 << 5)  // 强序非缓存
#define CSR_CRMD_DATF_CC  (0x1 << 5)  // 一致可缓存
#define CSR_CRMD_DATF_WUC (0x2 << 5)  // 弱序非缓存
#define CSR_CRMD_DATM  (0x3 << 7)  // 直接地址翻译模式下 load/store 操作存储访问类型 RW
#define CSR_CRMD_DATM_SUC (0x0 << 7)  // 强序非缓存
#define CSR_CRMD_DATM_CC  (0x1 << 7)  // 一致可缓存
#define CSR_CRMD_DATM_WUC (0x2 << 7)  // 弱序非缓存
#define CSR_CRMD_WE    (0x1 << 9)  // 指令和数据监视点使能 RW

/* 例外前模式信息 (PRMD) */
#define CSR_PRMD_PPLV  (0x3 << 0)  // 例外前特权级 RW
#define CSR_PRMD_PIE   (0x1 << 2)  // 例外前全局中断使能 RW
#define CSR_PRMD_PWE   (0x1 << 3)  // 例外前监视点使能 RW

/* 扩展部件使能 (EUEN) */
#define CSR_EUEN_FPE    (0x1 << 0)  // 基础浮点指令使能 RW
#define CSR_EUEN_SXE    (0x1 << 1)  // 128 位向量扩展指令使能 RW
#define CSR_EUEN_ASXE   (0x1 << 2)  // 256 位向量扩展指令使能 RW
#define CSR_EUEN_BTE    (0x1 << 3)  // 二进制翻译扩展指令使能 RW

/* 杂项 (MISC) */
#define CSR_MISC_VA32L1 (0x1 << 1)  // PLV1 特权等级下 32 位地址模式使能 RW
#define CSR_MISC_VA32L2 (0x1 << 2)  // PLV2 特权等级下 32 位地址模式使能 RW
#define CSR_MISC_VA32L3 (0x1 << 3)  // PLV3 特权等级下 32 位地址模式使能 RW
#define CSR_MISC_DRDTL1 (0x1 << 5)  // PLV1 特权等级下禁用 RDTIME 类指令 RW
#define CSR_MISC_DRDTL2 (0x1 << 6)  // PLV2 特权等级下禁用 RDTIME 类指令 RW
#define CSR_MISC_DRDTL3 (0x1 << 7)  // PLV3 特权等级下禁用 RDTIME 类指令 RW
#define CSR_MISC_RPCNTL1 (0x1 << 9) // PLV1 特权等级下允许读取性能计数器 RW
#define CSR_MISC_RPCNTL2 (0x1 << 10) // PLV2 特权等级下允许读取性能计数器 RW
#define CSR_MISC_RPCNTL3 (0x1 << 11) // PLV3 特权等级下允许读取性能计数器 RW
#define CSR_MISC_ALCL0  (0x1 << 12) // PLV0 特权等级下地址对齐检查使能 RW
#define CSR_MISC_ALCL1  (0x1 << 13) // PLV1 特权等级下地址对齐检查使能 RW
#define CSR_MISC_ALCL2  (0x1 << 14) // PLV2 特权等级下地址对齐检查使能 RW
#define CSR_MISC_ALCL3  (0x1 << 15) // PLV3 特权等级下地址对齐检查使能 RW
#define CSR_MISC_DWPL0  (0x1 << 16) // PLV0 特权等级下禁止页表项写允许位检查 RW
#define CSR_MISC_DWPL1  (0x1 << 17) // PLV1 特权等级下禁止页表项写允许位检查 RW
#define CSR_MISC_DWPL2  (0x1 << 18) // PLV2 特权等级下禁止页表项写允许位检查 RW

/* 例外配置 (ECFG) */
#define CSR_ECFG_LIE    (0x1FFF << 0) // 局部中断使能位 RW
#define CSR_ECFG_VS     (0x7 << 16)   // 例外和中断入口间距配置 RW

/* 例外状态 (ESTAT) */
#define CSR_ESTAT_IS    (0x1FFF << 0) // 中断状态位 R
#define CSR_ESTAT_MsgInt (0x1 << 14)  // 消息中断状态 R
#define CSR_ESTAT_Ecode (0x1FFF << 16) // 例外类型一级编码 R
#define CSR_ESTAT_EsubCode (0x7FFF << 22) // 例外类型二级编码 R

/* 例外程序返回地址 (ERA) */
#define CSR_ERA_PC      (0xFFFFFFFF << 0) // 例外返回地址 RW

/* 出错虚地址 (BADV) */
#define CSR_BADV_VAddr  (0xFFFFFFFF << 0) // 出错虚地址 RW

/* 出错指令 (BADI) */
#define CSR_BADI_Inst   (0xFFFFFFFF << 0) // 出错指令码 R

/* 例外入口地址 (EENTRY) */
#define CSR_EENTRY_VPN  (0x7FFF << 12) // 例外入口地址页号 RW

/* 缩减虚地址配置 (RVACFG) */
#define CSR_RVACFG_RBits (0xF << 0)   // 虚地址缩减位数配置 RW

/* 处理器编号 (CPUID) */
#define CSR_CPUID_CoreID (0x1FF << 0) // 处理器核编号 R

/* 特权资源配置信息 1 (PRCFG1) */
#define CSR_PRCFG1_SAVENum (0xF << 0) // 数据保存寄存器个数 R
#define CSR_PRCFG1_TimerBits (0xF << 4) // 定时器有效位数减 1 R
#define CSR_PRCFG1_VSMax   (0x7 << 12) // 例外和中断入口间距最大值 R

/* 特权资源配置信息 2 (PRCFG2) */
#define CSR_PRCFG2_PSAVL  (0xFFFFFFFF << 0) // 支持的页大小配置 R

/* 特权资源配置信息 3 (PRCFG3) */
#define CSR_PRCFG3_TLBType (0xF << 0)   // TLB 组织方式 R
#define CSR_PRCFG3_MTLBEntries (0xFF << 4) // 全相联多重页大小 TLB 项数减 1 R
#define CSR_PRCFG3_STLBWays  (0xFF << 12) // 组相联单个页大小 TLB 路数减 1 R
#define CSR_PRCFG3_STLBSets  (0x3F << 20) // 组相联单个页大小 TLB 每路项数的幂指数 R

/* 数据保存 (SAVE) */
#define CSR_SAVE_Data   (0xFFFFFFFF << 0) // 数据保存 RW

/* LLBit 控制 (LLBCTL) */
#define CSR_LLBCTL_ROLLB (0x1 << 0) // LLBit 状态 R
#define CSR_LLBCTL_WCLLB (0x1 << 1) // LLBit 清除控制 W1
#define CSR_LLBCTL_KLO    (0x1 << 3) // ERTN 指令执行时 LLBit 操作控制 RW

/* TLB 索引 (TLBIDX) */
#define CSR_TLBIDX_Index (0xFFFF << 0)  // TLB 表项索引 RW
#define CSR_TLBIDX_PS     (0x3F << 24)  // TLB 表项 PS 域 RW
#define CSR_TLBIDX_NE     (0x1 << 31)   // TLB 表项有效位 RW

/* TLB 表项高位 (TLBEHI) */
#define CSR_TLBEHI_PS     (0x3F << 24)  // TLB 表项 PS 域 RW
#define CSR_TLBEHI_VPPN   (0x7FFF8000 << 0)  // TLB 表项 VPPN 域 RW

/* TLB 表项低位 (TLBELO0, TLBELO1) */
#define CSR_TLBELO_V      (0x1 << 0)   // 页表项有效位 RW
#define CSR_TLBELO_D      (0x1 << 1)   // 页表项脏位 RW
#define CSR_TLBELO_PLV    (0x3 << 2)   // 页表项特权等级 RW
#define CSR_TLBELO_MAT    (0x3 << 4)   // 页表项存储访问类型 RW
#define CSR_TLBELO_G      (0x1 << 6)   // 页表项全局标志位 RW
#define CSR_TLBELO_PPN    (0xFFFFFFFF << 12)  // 页表项物理页号 RW
#define CSR_TLBELO_NR     (0x1 << 61)  // 页表项不可读位 RW
#define CSR_TLBELO_NX     (0x1 << 62)  // 页表项不可执行位 RW
#define CSR_TLBELO_RPLV   (0x1 << 63)  // 页表项受限特权等级使能 RW

/* 地址空间标识符 (ASID) */
#define CSR_ASID_ASID     (0x3FF << 0)  // 地址空间标识符 RW
#define CSR_ASIDBITS      (0xFF << 16)  // ASID 域的位宽 R

/* 低半地址空间全局目录基址 (PGDL) */
#define CSR_PGDL_Base     (0xFFFFFFFFFFFFF000 << 0)  // 低半地址空间全局目录基址 RW

/* 高半地址空间全局目录基址 (PGDH) */
#define CSR_PGDH_Base     (0xFFFFFFFFFFFFF000 << 0)  // 高半地址空间全局目录基址 RW

/* 全局目录基址 (PGD) */
#define CSR_PGD_Base      (0xFFFFFFFFFFFFF000 << 0)  // 全局目录基址 R

/* 页表遍历控制低半部分 (PWCL) */
#define CSR_PWCL_PTbase   (0x1F << 0)   // 末级页表起始地址 RW
#define CSR_PWCL_PTwidth  (0x1F << 5)   // 末级页表索引位数 RW
#define CSR_PWCL_Dir1_base (0x1F << 10) // 最低一级目录起始地址 RW
#define CSR_PWCL_Dir1_width (0x1F << 15) // 最低一级目录索引位数 RW
#define CSR_PWCL_Dir2_base (0x1F << 20) // 次低一级目录起始地址 RW
#define CSR_PWCL_Dir2_width (0x1F << 25) // 次低一级目录索引位数 RW
#define CSR_PWCL_PTEWidth (0x3 << 30)   // 页表项位宽 RW
#define CSR_PWCL_PTEWidth64 (0x0 << 30)

/* 页表遍历控制高半部分 (PWCH) */
#define CSR_PWCH_Dir3_base (0x3F << 0)  // 次高一级目录起始地址 RW
#define CSR_PWCH_Dir3_width (0x3F << 6) // 次高一级目录索引位数 RW
#define CSR_PWCH_Dir4_base (0x3F << 12) // 最高一级目录起始地址 RW
#define CSR_PWCH_Dir4_width (0x3F << 18) // 最高一级目录索引位数 RW
#define CSR_PWCH_HPTW_En  (0x1 << 24)   // 硬件页表遍历使能 RW

/* STLB 页大小 (STLBPS) */
#define CSR_STLBPS_PS     (0x3F << 0)  // STLB 页大小的 2 的幂指数 RW

/* TLB 重填例外入口地址 (TLBRENTRY) */
#define CSR_TLBRENTRY_PPN (0xFFFFFFFFFFFFF000 << 0)  // TLB 重填例外入口地址 RW

/* TLB 重填例外出错虚地址 (TLBRBADV) */
#define CSR_TLBRBADV_VAddr (0xFFFFFFFF << 0) // 出错虚地址 RW

/* TLB 重填例外返回地址 (TLBRERA) */
#define CSR_TLBRERA_IsTLBR (0x1 << 0) // TLB 重填例外标志位 RW
#define CSR_TLBRERA_PC     (0xFFFFFFFFFFFFF800 << 2) // 返回地址 RW

/* TLB 重填例外数据保存 (TLBRSAVE) */
#define CSR_TLBRSAVE_Data  (0xFFFFFFFF << 0) // 数据保存 RW

/* TLB 重填例外表项低位 (TLBRELO0, TLBRELO1) */
#define CSR_TLBRELO_V      (0x1 << 0)   // 页表项有效位 RW
#define CSR_TLBRELO_D      (0x1 << 1)   // 页表项脏位 RW
#define CSR_TLBRELO_PLV    (0x3 << 2)   // 页表项特权等级 RW
#define CSR_TLBRELO_MAT    (0x3 << 4)   // 页表项存储访问类型 RW
#define CSR_TLBRELO_G      (0x1 << 6)   // 页表项全局标志位 RW
#define CSR_TLBRELO_PPN    (0xFFFFFFFF << 12)  // 页表项物理页号 RW
#define CSR_TLBRELO_NR     (0x1 << 61)  // 页表项不可读位 RW
#define CSR_TLBRELO_NX     (0x1 << 62)  // 页表项不可执行位 RW
#define CSR_TLBRELO_RPLV   (0x1 << 63)  // 页表项受限特权等级使能 RW

/* TLB 重填例外表项高位 (TLBREHI) */
#define CSR_TLBREHI_PS     (0x3F << 24)  // TLB 表项 PS 域 RW
#define CSR_TLBREHI_VPPN   (0x7FFF8000 << 0)  // TLB 表项 VPPN 域 RW

/* TLB 重填例外前模式信息 (TLBRPRMD) */
#define CSR_TLBRPRMD_PPLV  (0x3 << 0)  // 例外前特权级 RW
#define CSR_TLBRPRMD_PIE   (0x1 << 2)  // 例外前全局中断使能 RW
#define CSR_TLBRPRMD_PWE   (0x1 << 4)  // 例外前监视点使能 RW

/* 直接映射配置窗口 (DMW0~DMW3) */
#define CSR_DMW_PLV0       (0x1 << 0)  // 特权等级 PLV0 使能 RW
#define CSR_DMW_PLV1       (0x1 << 1)  // 特权等级 PLV1 使能 RW
#define CSR_DMW_PLV2       (0x1 << 2)  // 特权等级 PLV2 使能 RW
#define CSR_DMW_PLV3       (0x1 << 3)  // 特权等级 PLV3 使能 RW
#define CSR_DMW_MAT        (0x3 << 4)  // 存储访问类型 RW
#define CSR_DMW_MAT_SUC    (0x0 << 4)  // 强序非缓存
#define CSR_DMW_MAT_CC     (0x1 << 4)  // 一致可缓存
#define CSR_DMW_MAT_WUC    (0x2 << 4)  // 弱序非缓存
#define CSR_DMW_VSEG       (0xF << 60) // 虚地址段 RW

/* 定时器编号 (TID) */
#define CSR_TID_TID        (0xFFFFFFFF << 0) // 定时器编号 RW

/* 定时器配置 (TCFG) */
#define CSR_TCFG_En        (0x1 << 0)   // 定时器使能 RW
#define CSR_TCFG_Periodic  (0x1 << 1)   // 定时器循环模式 RW
#define CSR_TCFG_InitVal   (0xFFFF << 2) // 定时器初始值 RW

/* 定时器数值 (TVAL) */
#define CSR_TVAL_TimeVal   (0xFFFF << 0) // 定时器当前值 R

/* 计时器补偿 (CNTC) */
#define CSR_CNTC_Compensation (0xFFFFFFFF << 0) // 计时器补偿值 RW

/* 定时中断清除 (TICLR) */
#define CSR_TICLR_CLR      (0x1 << 0)  // 定时中断清除 W1

/* 机器错误控制 (MERRCTL) */
#define CSR_MERRCTL_IsMERR    (0x1 << 0)   // 当前处于机器错误例外上下文 R
#define CSR_MERRCTL_Repairable (0x1 << 1)  // 错误可自动修复 R
#define CSR_MERRCTL_PPLV      (0x3 << 2)   // 例外前特权级 RW
#define CSR_MERRCTL_PIE       (0x1 << 4)   // 例外前全局中断使能 RW
#define CSR_MERRCTL_PWE       (0x1 << 6)   // 例外前监视点使能 RW
#define CSR_MERRCTL_PDA       (0x1 << 7)   // 例外前直接地址翻译模式使能 RW
#define CSR_MERRCTL_PPG       (0x1 << 8)   // 例外前映射地址翻译模式使能 RW
#define CSR_MERRCTL_PDATF     (0x3 << 9)   // 例外前DATF域 RW
#define CSR_MERRCTL_PDATM     (0x3 << 11)  // 例外前DATM域 RW
#define CSR_MERRCTL_Cause     (0xFF << 16) // 机器错误类型编码 R

/* 机器错误例外入口地址 (MERRENTRY) */
#define CSR_MERRENTRY_PPN     (0xFFFFFFFFFFFFF000 << 0) // 入口物理地址 RW

/* 性能监测配置 (PMCFG) */
#define CSR_PMCFG_EvCode      (0x3FF << 0)  // 性能事件号 RW
#define CSR_PMCFG_PLV0        (0x1 << 16)   // PLV0计数使能 RW
#define CSR_PMCFG_PLV1        (0x1 << 17)   // PLV1计数使能 RW
#define CSR_PMCFG_PLV2        (0x1 << 18)   // PLV2计数使能 RW
#define CSR_PMCFG_PLV3        (0x1 << 19)   // PLV3计数使能 RW
#define CSR_PMCFG_PMIEn       (0x1 << 20)   // 溢出中断使能 RW

/* 性能监测计数器 (PMCNT) */
#define CSR_PMCNT_Count       (0xFFFFFFFFFFFFFFFF << 0) // 计数值 RW

/* load/store监视点整体配置 (MWPC) */
#define CSR_MWPC_Num          (0x3F << 0)   // load/store监视点数量 R

/* load/store监视点整体状态 (MWPS) */
#define CSR_MWPS_Status       (0xFFFF << 0) // 监视点命中状态 RW1
#define CSR_MWPS_Skip         (0x1 << 16)   // 忽略下一次命中 RW

/* load/store监视点配置寄存器组 */
#define CSR_MWPCFG1_VAddr     (0xFFFFFFFFFFFFFFFF << 0) // 比较地址 RW
#define CSR_MWPCFG2_Mask      (0xFFFFFFFFFFFFFFFF << 0) // 地址掩码 RW
#define CSR_MWPCFG3_DSOnly    (0x1 << 0)    // 调试模式独占 RW
#define CSR_MWPCFG3_PLV0      (0x1 << 1)    // PLV0使能 RW
#define CSR_MWPCFG3_PLV1      (0x1 << 2)    // PLV1使能 RW
#define CSR_MWPCFG3_PLV2      (0x1 << 3)    // PLV2使能 RW
#define CSR_MWPCFG3_PLV3      (0x1 << 4)    // PLV3使能 RW
#define CSR_MWPCFG3_LCL       (0x1 << 7)    // 局部监视点标志 RW
#define CSR_MWPCFG3_LoadEn    (0x1 << 8)    // load操作监视 RW
#define CSR_MWPCFG3_StoreEn   (0x1 << 9)    // store操作监视 RW
#define CSR_MWPCFG3_Size      (0x3 << 10)   // 字节范围配置 RW
#define CSR_MWPCFG4_ASID      (0x3FF << 0)  // ASID比较值 RW

/* 调试寄存器 (DBG) */
#define CSR_DBG_DS            (0x1 << 0)    // 调试模式状态 R
#define CSR_DBG_DRev          (0x7F << 1)   // 调试机制版本 R
#define CSR_DBG_DEI           (0x1 << 8)    // 调试外部中断标志 R
#define CSR_DBG_DCL           (0x1 << 9)    // 调试调用例外标志 R
#define CSR_DBG_DFW           (0x1 << 10)   // 取指监视点例外标志 R
#define CSR_DBG_DMW           (0x1 << 11)   // load/store监视点例外标志 R
#define CSR_DBG_Ecode         (0x3F << 16)  // 调试例外编码 R

/* 消息中断寄存器组 */
#define CSR_MSGIS_IS          (0xFFFFFFFFFFFFFFFF << 0) // 中断状态位 R
#define CSR_MSGIR_IntNum      (0xFF << 0)   // 中断请求编号 R
#define CSR_MSGIR_Null        (0x1 << 31)   // 无效请求标志 R
#define CSR_MSGIE_PT          (0xFF << 0)   // 优先级阈值 RW


#define csr_read(CSR, VAL) \
    asm volatile("csrrd %0, %1" : "=r"(VAL) : "i"(CSR))

#define csr_write(CSR, VAL) \
    asm volatile("csrwr %0, %1" : : "r"(VAL), "i"(CSR))

#define csr_set_bits(CSR, BITS) \
    asm volatile("csrxchg $zero, %0, %1" : : "r"(BITS), "i"(CSR));

static inline uint64 r_csr_crmd() { uint64 val; csr_read(CSR_CRMD, val); return val; }
static inline void w_csr_crmd(uint64 val) { csr_write(CSR_CRMD, val); }
    
static inline uint64 r_csr_prmd() { uint64 val; csr_read(CSR_PRMD, val); return val; }
static inline void w_csr_prmd(uint64 val) { csr_write(CSR_PRMD, val); }
    
static inline uint64 r_csr_euen() { uint64 val; csr_read(CSR_EUEN, val); return val; }
static inline void w_csr_euen(uint64 val) { csr_write(CSR_EUEN, val); }
    
static inline uint64 r_csr_misc() { uint64 val; csr_read(CSR_MISC, val); return val; }
static inline void w_csr_misc(uint64 val) { csr_write(CSR_MISC, val); }
    
static inline uint64 r_csr_ecfg() { uint64 val; csr_read(CSR_ECFG, val); return val; }
static inline void w_csr_ecfg(uint64 val) { csr_write(CSR_ECFG, val); }
    
static inline uint64 r_csr_estat() { uint64 val; csr_read(CSR_ESTAT, val); return val; }
static inline void w_csr_estat(uint64 val) { csr_write(CSR_ESTAT, val); }
    
static inline uint64 r_csr_era() { uint64 val; csr_read(CSR_ERA, val); return val; }
static inline void w_csr_era(uint64 val) { csr_write(CSR_ERA, val); }
    
static inline uint64 r_csr_badv() { uint64 val; csr_read(CSR_BADV, val); return val; }
static inline void w_csr_badv(uint64 val) { csr_write(CSR_BADV, val); }
    
static inline uint64 r_csr_badi() { uint64 val; csr_read(CSR_BADI, val); return val; }
static inline void w_csr_badi(uint64 val) { csr_write(CSR_BADI, val); }
    
static inline uint64 r_csr_eentry() { uint64 val; csr_read(CSR_EENTRY, val); return val; }
static inline void w_csr_eentry(uint64 val) { csr_write(CSR_EENTRY, val); }
    
static inline uint64 r_csr_tlbidx() { uint64 val; csr_read(CSR_TLBIDX, val); return val; }
static inline void w_csr_tlbidx(uint64 val) { csr_write(CSR_TLBIDX, val); }
    
static inline uint64 r_csr_tlbehi() { uint64 val; csr_read(CSR_TLBEHI, val); return val; }
static inline void w_csr_tlbehi(uint64 val) { csr_write(CSR_TLBEHI, val); }
    
static inline uint64 r_csr_tlbelo0() { uint64 val; csr_read(CSR_TLBELO0, val); return val; }
static inline void w_csr_tlbelo0(uint64 val) { csr_write(CSR_TLBELO0, val); }
    
static inline uint64 r_csr_tlbelo1() { uint64 val; csr_read(CSR_TLBELO1, val); return val; }
static inline void w_csr_tlbelo1(uint64 val) { csr_write(CSR_TLBELO1, val); }
    
static inline uint64 r_csr_asid() { uint64 val; csr_read(CSR_ASID, val); return val; }
static inline void w_csr_asid(uint64 val) { csr_write(CSR_ASID, val); }
    
static inline uint64 r_csr_pgdl() { uint64 val; csr_read(CSR_PGDL, val); return val; }
static inline void w_csr_pgdl(uint64 val) { csr_write(CSR_PGDL, val); }
    
static inline uint64 r_csr_pgdh() { uint64 val; csr_read(CSR_PGDH, val); return val; }
static inline void w_csr_pgdh(uint64 val) { csr_write(CSR_PGDH, val); }
    
static inline uint64 r_csr_pgd() { uint64 val; csr_read(CSR_PGD, val); return val; }
static inline void w_csr_pgd(uint64 val) { csr_write(CSR_PGD, val); }
    
static inline uint64 r_csr_pwcl() { uint64 val; csr_read(CSR_PWCL, val); return val; }
static inline void w_csr_pwcl(uint64 val) { csr_write(CSR_PWCL, val); }
    
static inline uint64 r_csr_pwch() { uint64 val; csr_read(CSR_PWCH, val); return val; }
static inline void w_csr_pwch(uint64 val) { csr_write(CSR_PWCH, val); }
    
static inline uint64 r_csr_stlbps() { uint64 val; csr_read(CSR_STLBPS, val); return val; }
static inline void w_csr_stlbps(uint64 val) { csr_write(CSR_STLBPS, val); }
    
static inline uint64 r_csr_rvacfg() { uint64 val; csr_read(CSR_RVACFG, val); return val; }
static inline void w_csr_rvacfg(uint64 val) { csr_write(CSR_RVACFG, val); }
    
static inline uint64 r_csr_cpuid() { uint64 val; csr_read(CSR_CPUID, val); return val; }
static inline void w_csr_cpuid(uint64 val) { csr_write(CSR_CPUID, val); }
    
static inline uint64 r_csr_prcfg1() { uint64 val; csr_read(CSR_PRCFG1, val); return val; }
static inline void w_csr_prcfg1(uint64 val) { csr_write(CSR_PRCFG1, val); }
    
static inline uint64 r_csr_prcfg2() { uint64 val; csr_read(CSR_PRCFG2, val); return val; }
static inline void w_csr_prcfg2(uint64 val) { csr_write(CSR_PRCFG2, val); }
    
static inline uint64 r_csr_prcfg3() { uint64 val; csr_read(CSR_PRCFG3, val); return val; }
static inline void w_csr_prcfg3(uint64 val) { csr_write(CSR_PRCFG3, val); }
    
static inline uint64 r_csr_tlbrentry() { uint64 val; csr_read(CSR_TLBRENTRY, val); return val; }
static inline void w_csr_tlbrentry(uint64 val) { csr_write(CSR_TLBRENTRY, val); }
    
static inline uint64 r_csr_tlbrbadv() { uint64 val; csr_read(CSR_TLBRBADV, val); return val; }
static inline void w_csr_tlbrbadv(uint64 val) { csr_write(CSR_TLBRBADV, val); }
    
static inline uint64 r_csr_tlbrera() { uint64 val; csr_read(CSR_TLBRERA, val); return val; }
static inline void w_csr_tlbrera(uint64 val) { csr_write(CSR_TLBRERA, val); }
    
static inline uint64 r_csr_tlbrsave() { uint64 val; csr_read(CSR_TLBRSAVE, val); return val; }
static inline void w_csr_tlbrsave(uint64 val) { csr_write(CSR_TLBRSAVE, val); }
    
static inline uint64 r_csr_tlbrelo0() { uint64 val; csr_read(CSR_TLBRELO0, val); return val; }
static inline void w_csr_tlbrelo0(uint64 val) { csr_write(CSR_TLBRELO0, val); }
    
static inline uint64 r_csr_tlbrelo1() { uint64 val; csr_read(CSR_TLBRELO1, val); return val; }
static inline void w_csr_tlbrelo1(uint64 val) { csr_write(CSR_TLBRELO1, val); }
    
static inline uint64 r_csr_tlbrehi() { uint64 val; csr_read(CSR_TLBREHI, val); return val; }
static inline void w_csr_tlbrehi(uint64 val) { csr_write(CSR_TLBREHI, val); }
    
static inline uint64 r_csr_tlbrprmd() { uint64 val; csr_read(CSR_TLBRPRMD, val); return val; }
static inline void w_csr_tlbrprmd(uint64 val) { csr_write(CSR_TLBRPRMD, val); }
    
static inline uint64 r_csr_dmw0() { uint64 val; csr_read(CSR_DMW0, val); return val; }
static inline void w_csr_dmw0(uint64 val) { csr_write(CSR_DMW0, val); }
    
static inline uint64 r_csr_dmw1() { uint64 val; csr_read(CSR_DMW1, val); return val; }
static inline void w_csr_dmw1(uint64 val) { csr_write(CSR_DMW1, val); }
    
static inline uint64 r_csr_dmw2() { uint64 val; csr_read(CSR_DMW2, val); return val; }
static inline void w_csr_dmw2(uint64 val) { csr_write(CSR_DMW2, val); }
    
static inline uint64 r_csr_dmw3() { uint64 val; csr_read(CSR_DMW3, val); return val; }
static inline void w_csr_dmw3(uint64 val) { csr_write(CSR_DMW3, val); }
    
static inline uint64 r_csr_llbctl() { uint64 val; csr_read(CSR_LLBCTL, val); return val; }
static inline void w_csr_llbctl(uint64 val) { csr_write(CSR_LLBCTL, val); }
    
static inline uint64 r_csr_impctl1() { uint64 val; csr_read(CSR_IMPCTL1, val); return val; }
static inline void w_csr_impctl1(uint64 val) { csr_write(CSR_IMPCTL1, val); }
    
static inline uint64 r_csr_impctl2() { uint64 val; csr_read(CSR_IMPCTL2, val); return val; }
static inline void w_csr_impctl2(uint64 val) { csr_write(CSR_IMPCTL2, val); }
    
static inline uint64 r_csr_ctag() { uint64 val; csr_read(CSR_CTAG, val); return val; }
static inline void w_csr_ctag(uint64 val) { csr_write(CSR_CTAG, val); }
    
static inline uint64 r_csr_msgir() { uint64 val; csr_read(CSR_MSGIR, val); return val; }
static inline void w_csr_msgir(uint64 val) { csr_write(CSR_MSGIR, val); }
    
static inline uint64 r_csr_msgie() { uint64 val; csr_read(CSR_MSGIE, val); return val; }
static inline void w_csr_msgie(uint64 val) { csr_write(CSR_MSGIE, val); }

#define r_reg(REG, VAL) \
    asm volatile("move %0, $" #REG : "=r"(VAL))

#define w_reg(REG, VAL) \
    asm volatile("move $" #REG ", %0" : : "r"(VAL))

static inline uint64 r_sp() { uint64 val; r_reg(sp, val); return val; }
static inline void w_sp(uint64 val) { w_reg(sp, val); }

static inline uint64 r_ra() { uint64 val; asm volatile("move %0, $ra" : "=r"(val)); return val; }
static inline void w_ra(uint64 val) { w_reg(ra, val); }

static inline uint64 r_tp() { uint64 val; asm volatile("move %0, $tp" : "=r"(val)); return val; }
static inline void w_tp(uint64 val) { w_reg(tp, val); }

static inline uint64
cpucfg(uint64 num) 
{
    uint64 val;
    asm volatile("cpucfg %0, %1" : "=r"(val) : "r"(num));
    return val;
}

static inline void 
intr_on() 
{
    csr_set_bits(CSR_CRMD, CSR_CRMD_IE);
}

static inline void 
intr_off() 
{
    w_csr_crmd(r_csr_crmd() & ~CSR_CRMD_IE);
}


// Invalidate TLB Entry
#define invtlb() asm volatile("invtlb  0x0, $zero, $zero")
