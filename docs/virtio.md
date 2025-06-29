# VIRTIO 驱动

VIRTIO 是 HANAOS 采用的底层 I/O 协议和网络栈物理层协议。VIRTIO 驱动参考了 SOS (Stephen's OS) 的基于 MMIO 的 VIRTIO 1.0 代码，实现了遵守 VIRTIO 0.95 版本，在 RISCV 架构下基于 MMIO 总线，在 Loongarch 架构下基于 PCI 总线并支持 MSI-X 中断的，Virtio-blk 设备和 Virtio-net 设备的驱动。

## Virtio / MMIO

### MMIO 设备寄存器

在 MMIO 模式中，VIRTIO 设备寄存器分布在设备树指定的地址范围内，基地址为`VIRT_VIRTIO=0x10001000L`，每个设备占用 4KB 的空间。寄存器的布局遵循标准化结构，包括：

- **MagicValue**：固定为 `0x74726976`，用于识别 Virtio 设备
- **Version**：设备版本号，固定为 `0x1`
- **DeviceID**：标识设备类型（块设备 `0x2`，网卡设备 `0x1`）
- **QueueNum**：设置虚拟队列大小
- **QueuePFN**：存储虚拟队列的物理页帧号（PFN）
- **InterruptStatus**：记录中断状态，需要主动清零

寄存器空间之后是设备配置空间 `Config[]`，用于存储设备特定信息如块设备容量、网卡 MAC 地址等。

### 设备初始化

MMIO 设备初始化流程如下：

1. **重置设备**：清除设备状态寄存器
2. **特性协商**：检查设备支持的功能标志（如 `VIRTIO_BLK_F_SIZE_MAX`）并启用所需功能
3. **队列设置**：
   - 通过 `QueueSelect` 选择队列索引
   - 设置 `QueueNum` 指定队列大小
   - 分配物理内存存放 `virtqueue` 结构
   - 取得内存地址的物理页号，写入 `QueuePFN` 寄存器
4. **完成配置**：设置 `GuestPageSize` 为系统页大小 `PGSIZE`
5. **启动设备**：设置状态寄存器为 `VIRTIO_STATUS_DRIVER_OK`

### Virt Queue 初始化

不同种类的设备需要不同个数的 Virt Queue，其初始化流程如下：

1. 根据队列大小计算内存需求：
   - 描述符表 (`virtqueue_desc`)：`queue_size × 16字节`
   - 可用环 (`virtqueue_avail`)：`8 + queue_size × 2字节`
   - 已用环 (`virtqueue_used`)：`8 + queue_size × 8字节`
2. 严格对齐分配内存，满足`VIRTIO_DEFAULT_ALIGN`要求
3. 初始化链表：
   - 所有描述符按顺序链接，形成空闲链表
   - 可用环头尾索引清零
   - 设置已用环的 `flags` 与 `idx` 为0

### 中断通知

Virt Queue 在完成向描述符表填充缓冲区的工作后，向 QueueNotify 寄存器写入队列号，通知设备进行 IO 操作。

## Virtio / PCI

### PCI 设备寄存器

    （virtio寄存器在pci设备的哪个bar，设备寄存器的布局）

### 设备初始化

    （pci virtio 设备初始化流程）

### Virt Queue 初始化

    （virtio queue 初始化流程）

### 中断初始化

#### 传统中断

    （传统中断初始化流程和怎么获取中断号）

#### MSI-X 中断

    （msi-x初始化流程和怎么获取中断号）

## Virtio-blk



## Virtio-net

