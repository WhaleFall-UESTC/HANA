#ifndef __PCI_H__
#define __PCI_H__

#include <common.h>

#define PCI_CONFIG_BASE 0x20000000UL /* PCI配置空间数据端口 */
#define PCI_IO_BASE 0x18000000UL
#define PCI_IOSPACE_STA 0x4000UL
#define PCI_IOSPACE_END 0xFFFFUL
#define PCI_MEM_BASE 0x40000000UL
#define PCI_MEM_SIZE 0x40000000UL

#define PCI_MAX_BAR 6         /* 每个设备最多有6地址信息 */
#define PCI_MAX_BUS 128       /* PCI总共有256个总线，设备树中显示只支持128个 */
#define PCI_MAX_DEV 32        /* PCI每条总线上总共有32个设备 */
#define PCI_MAX_FUN 8         /* PCI设备总共有8个功能号 */
#define PCI_MAX_DEVICE_NR 256 /* 系统最大支持检测多少个设备 */

/* PCI配置空间数据的偏移 */
#define PCI_DEVICE_VENDER 0x00                             /* 设备厂商ID和设备ID的寄存器偏移量 */
#define PCI_STATUS_COMMAND 0x04                            /* 状态和命令寄存器偏移量 */
#define PCI_CLASS_CODE_REVISION_ID 0x08                    /* 类型、子类型、次类型和修订号寄存器偏移量 */
#define PCI_BIST_HEADER_TYPE_LATENCY_TIMER_CACHE_LINE 0x0C /* BIST（Built-In Self-Test，内建自测）、头部类型、延迟计时器和缓存行寄存器偏移量。 */
#define PCI_BARS_ADDRESS0 0x10                             /*（BARs，基地址寄存器），从0～5一共可以存六个地址信息 */
#define PCI_BARS_ADDRESS1 0x14
#define PCI_BARS_ADDRESS2 0x18
#define PCI_BARS_ADDRESS3 0x1C
#define PCI_BARS_ADDRESS4 0x20
#define PCI_BARS_ADDRESS5 0x24
#define PCI_CARD_BUS_POINTER 0x28                 /* 卡总线指针寄存器偏移量 */
#define PCI_SUBSYSTEM_ID 0x2C                     /* 子系统ID寄存器偏移量 */
#define PCI_EXPANSION_ROM_BASE_ADDR 0x30          /* 扩展ROM基地址寄存器偏移量 */
#define PCI_CAPABILITY_LIST 0x34                  /* 能力列表寄存器偏移量 */
#define PCI_RESERVED 0x38                         /* 保留寄存器偏移量 */
#define PCI_MAX_LNT_MIN_GNT_IRQ_PIN_IRQ_LINE 0x3C /* 最大的总线延迟时间、最小的总线延迟时间、最大的访问延迟时间、中断引脚、中断线寄存器偏移量 */

/* 向command中写入的控制位 */
#define PCI_COMMAND_IO 0x1             /* I/O 空间的响应使能 */
#define PCI_COMMAND_MEMORY 0x2         /* 内存空间的响应使能 */
#define PCI_COMMAND_MASTER 0x4         /* 总线主控制使能 */
#define PCI_COMMAND_SPECIAL 0x8        /* 特殊事务响应使能 */
#define PCI_COMMAND_INVALIDATE 0x10    /* 使用内存写入并且使其失效 */
#define PCI_COMMAND_VGA_PALETTE 0x20   /* 启用VGA调色板的监听 */
#define PCI_COMMAND_PARITY 0x40        /* 启用奇偶校验检查 */
#define PCI_COMMAND_WAIT 0x80          /* 启用地址/数据步进 */
#define PCI_COMMAND_SERR 0x100         /* 启用系统错误（SERR） */
#define PCI_COMMAND_FAST_BACK 0x200    /* 启用快速回写 */
#define PCI_COMMAND_INTX_DISABLE 0x400 /* INTx模拟禁用 */

/* IO地址和MEM地址的地址mask */
#define PCI_BAR_MEM_ATTR_MASK 0x0FUL
#define PCI_BAR_IO_ATTR_MASK 0x03UL
#define PCI_BAR_MEM_ADDR_MASK (~0x0FUL) // 屏蔽低四位
#define PCI_BAR_IO_ADDR_MASK (~0x03UL)  // 屏蔽低两位

/* PCI设备ID */
struct pci_device_id
{
    uint32 vendor, device;           // 供应商和设备 ID 或 PCI_ANY_ID
    uint32 subvendor, subdevice;     // 子系统的 id 或 PCI_ANY_ID
    uint32 device_class, class_mask;
};

/* PCI地址bar结构体，保存Bass Address（0~5）的信息 */
typedef struct pci_device_bar
{
    uint8 type;       /* 地址 bar 的类型（IO地址/MEM地址）*/
    uint64 base_addr; /* bar 的物理地址 */
    uint64 length;    /* 地址的长度 */
} pci_device_bar_t;

#define PCI_DEVICE_INVALID 0 /*被定义为 0，表示无效的PCI设备*/
#define PCI_DEVICE_USING 1   /*被定义为 1，表示正在使用的PCI设备*/

/* PCI BAR的类型 */
#define PCI_BAR_TYPE_INVALID 0
#define PCI_BAR_TYPE_MEM 1
#define PCI_BAR_TYPE_IO 2

typedef struct msix_table_entry {
    uint32 msg_addr_low;     /* 消息地址低32位 */
    uint32 msg_addr_high;    /* 消息地址高32位 */
    uint32 msg_data;         /* 消息数据 */
    uint32 vector_ctrl;      /* 向量控制寄存器 */
} msix_table_entry_t;

#define PCI_CAPABILITY_ID_MSI_X 0x11

#define PCI_MSIX_CAP_BASE0 0x0
#define PCI_MSIX_CAP_CAPID_MASK 0xFFU
#define PCI_MSIX_CAP_NXT_PTR_MASK 0xFF00U
#define PCI_MSIX_CAP_MSG_CTRL_MASK 0xFFFF0000U
#define PCI_MSIX_CAP_MSG_CTRL_SHIFT 16

#define PCI_MSIX_CAP_ENABLE_MASK 0x80000000U

#define PCI_MSIX_CAP_MC_TBSIZE_MASK 0x7FFU
#define PCI_MSIX_CAP_MC_FUNCMASK_MASK 0x4000U
#define PCI_MSIX_CAP_MC_FUNCMASK_SHIFT 14

#define PCI_MSIX_CAP_BASE1 0x4
#define PCI_MSIX_CAP_BIR_MASK 0x7
#define PCI_MSIX_CAP_TABLE_OFS_MASK 0xFFFFFFF8

#define PCI_MSIX_CAP_BASE2 0x8
#define PCI_MSIX_CAP_PENDING_BIT_BIR_MASK 0x7
#define PCI_MSIX_CAP_PENDING_BIR_OFS_MASK 0xFFFFFFF8

#define PCI_MSIX_ADDR_LOW_MASK 0xFFFFFFFCUL
#define PCI_MSIX_ADDR_HIGH_MASK 0xFFFFFFFF00000000UL

#define PCI_MSIX_MSG_ADDR 0x2ff00000UL
#define PCI_MSIX_VEC_BASE 0x20

/*
    PCI设备结构体，用于保存我们所需要的pci信息，并不是和硬件的一样
*/
typedef struct pci_device
{
    int flags; /*device flags*/

    unsigned char bus;      /*bus总线号*/
    unsigned char dev;      /*device号*/
    unsigned char function; /*功能号*/

    unsigned short vendor_id; /*配置空间:Vendor ID*/
    unsigned short device_id; /*配置空间:Device ID*/
    unsigned short command;   /*配置空间:Command*/
    unsigned short status;    /*配置空间:Status*/

    unsigned int class_code;      /*配置空间:Class Code*/
    unsigned char revision_id;    /*配置空间:Revision ID*/
    unsigned char multi_function; /*多功能标志*/
    unsigned int card_bus_pointer;
    unsigned short subsystem_vendor_id;
    unsigned short subsystem_device_id;
    unsigned int expansion_rom_base_addr;
    unsigned int capability_list;

    unsigned char irq_line; /*配置空间:IRQ line*/
    unsigned char irq_pin;  /*配置空间:IRQ pin*/
    unsigned char min_gnt;
    unsigned char max_lat;
    pci_device_bar_t bar[PCI_MAX_BAR]; /*有6个地址信息*/

    /* MSI-X 支持 */
    unsigned char msix_cap_offset;      /* MSI-X 能力寄存器的偏移 */
    unsigned char msix_enabled;         /* 是否启用 MSI-X */
    uint8 msix_table_bir;               /* 存放 MSI-X 表的 BAR 索引 */
    uint32 msix_table_offset;           /* MSI-X 表在 BAR 中的偏移 */
    uint16 msix_table_size;             /* MSI-X 表项数量（实际是 msix_table_size + 1） */
    uint8 msix_pba_bir;                 /* 存放 Pending Bit Array 的 BAR 索引 */
    uint32 msix_pba_offset;             /* Pending Bit Array 在 BAR 中的偏移 */
    msix_table_entry_t *msix_table;     /* MSI-X 表指针 */
} pci_device_t;

unsigned int pci_device_get_io_addr(pci_device_t *device);
unsigned int pci_device_get_mem_addr(pci_device_t *device);
unsigned int pci_device_get_mem_len(pci_device_t *device);
unsigned int pci_device_get_irq_line(pci_device_t *device);
unsigned int pci_device_get_irq_pin(pci_device_t *device);
void pci_device_set_irq_line(pci_device_t *device, unsigned int irq);
unsigned int pci_device_get_intc(pci_device_t* device);
void pci_enable_bus_mastering(pci_device_t *device);
pci_device_t *pci_get_device_by_class_code(unsigned int class, unsigned int sub_class);
pci_device_t *pci_get_next_device(devid_t* pdevid);

pci_device_t *pci_locate_device(unsigned short vendor, unsigned short device);
pci_device_t *pci_locate_class(unsigned short class, unsigned short _subclass);

void pci_device_bar_dump(pci_device_bar_t *bar);
void pci_device_dump(pci_device_t *device);

unsigned int pci_device_read(pci_device_t *device, unsigned int reg);
void pci_device_write(pci_device_t *device, unsigned int reg, unsigned int value);
pci_device_t *pci_get_device_by_bus(unsigned int bus,
                                    unsigned int dev,
                                    unsigned int function);

int pci_msix_add_vector(pci_device_t *device, uint32 vector, uint64 msg_addr, uint32 msg_data);
int pci_enable_msix(pci_device_t *device);
int pci_disable_msix(pci_device_t *device);

void pci_init(void);

#define pci_for_using_device(device_ptr) \
    for(devid_t __devid = 0; (device_ptr = pci_get_next_device(&__devid)) != NULL; )

#endif // __PCI_H__