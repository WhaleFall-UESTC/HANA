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

PCI 中的 BAR 寄存器（ Base Address Register ）指向的空间通常分为 I/O 空间与 MEM 空间两种类型。在 QEMU 的龙芯编程中，这俩中空间都是通过地址映射进行访问，二者的映射基址和范围如下：

```c
#define PCI_IO_BASE 0x18000000UL
#define PCI_IOSPACE_STA 0x4000UL
#define PCI_IOSPACE_END 0xFFFFUL
#define PCI_MEM_BASE 0x40000000UL
#define PCI_MEM_SIZE 0x40000000UL
```

BAR 寄存器的头信息和两种布局如下：

1. 基地址寄存器

    Bit0: 标志位，若为1则为io空间，若为0则为mem空间

2. I/O基地址寄存器:

    Bit1: 保留

    Bit31-2: 基地址单元

    Bit63-32: 保留

3. MEM 基地址存储器:

    Bit2-1: MEM 基地址寄存器-译码器宽度单元,00-32 位,10-64 位

    Bit3: 预提取属性

    Bit64-4: 基地址单元

初始化时，PCI 驱动将 BAR 寄存器写全 1，并再次读取它的值，将读取的值取反加一即为设备要求的空间大小。PCI 驱动将在 BAR 寄存器指定类型的空间范围，根据长度要求分配对齐的一段地址。

## PCI 传统中断的配置

PCI 设备的中断号是由中断引脚和 SLOT ID 决定的，后者等于设备的 dev 号。该映射在设备树中进行了规定，其关系为：

```c
static struct slot_pin_info slot_info[][4] = {
	{
		{0x01, 0x10},
		{0x02, 0x11},
		{0x03, 0x12},
		{0x04, 0x13},
	},
	{
		{0x01, 0x11},
		{0x02, 0x12},
		{0x03, 0x13},
		{0x04, 0x10},
	},
	{
		{0x01, 0x12},
		{0x02, 0x13},
		{0x03, 0x10},
		{0x04, 0x11},
	},
	{
		{0x01, 0x13},
		{0x02, 0x10},
		{0x03, 0x11},
		{0x04, 0x12},
	}
};
```

其中 slot_info 第一维下标为 SLOT ID，每个`struct slot_pin_info`中记录了中断引脚和中断号的对应，其中中断引脚号 0x1 对应 INTA#, 0x2 对应 INTB#, 0x3 对应 INTC#, 0x4 对应 INTD#。

获取中断号后，写入偏移为 0x3C 的 Interrupt Line 寄存器即可完成设置。

## PCI MSI-X 中断

PCI 设备初始化时，`pci_detect_msix`函数遍历设备的 PCI capability 链表并找出 MSI-X capability 的位置，将相应的信息填入`pci_device_t`的 MSI-X 相关字段，然后`pci_map_msix_table`函数从 BAR 空间中找出计算出`msix_table`的地址。

设备要使用 MSI-X 中断时，首先调用`pci_enable_msix`函数，将 MSI-X 中断使能并禁用传统中断，然后调用`pci_msix_add_vector`函数将新的 MSI-X vector 加入到 MSI-X table 中。

在 MSI-X vector 加入到 MSI-X table 的步骤中，需要`msg_addr`和`msg_data`两个信息。对于龙芯来说，这两个信息应该分别填入 PCH-MSI 控制器的 64 位寄存器的地址，以及设备的 MSI-X 中断向量号。