# PCI 驱动

HANAOS 在龙芯下实现了 PCI 驱动，用于基于 PCI 总线的 VIRTIO 设备。PCI 驱动参考了前人不完整的 PCI 驱动实现（引用链接和协议见 pci.c 和 pci.h），相比于原来的半成品，增加了 IO 和 MEM 两种 BAR 空间的配置和映射、PCI 传统中断的配置和中断号的映射、MSI-X 中断的配置等功能和模块，修正了配置空间读写、PCI 设备初始化等模块的错误。

## 配置空间的读写

PCI 驱动直接对映射后的配置空间地址进行读写。给定 PCI 设备的`bus`、`device`、`function`三个用于路由的编号以及要读写的寄存器地址`reg_id`，计算读写地址如下：

```c
addr = phys_to_virt(base_cfg_addr | (bus << 20) | (device << 15) | (function << 12) | (reg_id & 0xFFC));
```

其中 `base_cfg_addr = PCI_CONFIG_BASE`，是在 qemu 下的 PCI 配置头基址的物理地址，值为`0x20000000UL`。

实际上，这里路由的是在 PCIE 总线上的 PCIE 设备的 PCI 配置头地址。

## PCI 总线枚举和扫描

`pci_scan_buses`函数枚举`bus`、`device`、`function`三个参数，路由到每个 PCI 设备的配置头，对配置头中的关键参数和信息读取到 PCI 设备结构体并进行初始化，PCI 设备结构体定义如下：

```c
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
```

## BAR 空间的配置和映射

