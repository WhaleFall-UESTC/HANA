# VIRTIO 驱动

VIRTIO 是 HANAOS 采用的底层 I/O 协议和网络栈物理层协议。VIRTIO 驱动参考了 SOS (Stephen's OS) 的基于 MMIO 的 VIRTIO 1.0 代码，实现了遵守 VIRTIO 0.95 版本，在 RISCV 架构下基于 MMIO 总线，在 Loongarch 架构下基于 PCI 总线并支持 MSI-X 中断的，Virtio-blk 设备和 Virtio-net 设备的驱动。

## Virtio / MMIO

### MMIO 设备寄存器

在 MMIO 模式中，VIRTIO 设备寄存器分布在设备树指定的地址范围内，基地址为`VIRT_VIRTIO=0x10001000L`，每个设备占用 4KB 的空间，寄存器只能通过 32 位对齐访问。寄存器的布局遵循标准化结构，主要包括：

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
2. **特性协商**：调用
`virtio_check_capabilities`函数，检查设备支持的功能标志（如 `VIRTIO_BLK_F_SIZE_MAX`）并启用所需功能
3. **队列设置**：
   - 通过 `QueueSelect` 选择队列索引
   - 设置 `QueueNum` 指定队列大小
   - 分配物理内存存放 `virtqueue` 结构
   - 取得内存地址的物理页号，写入 `QueuePFN` 寄存器
4. **完成配置**：设置 `GuestPageSize` 为系统页大小 `PGSIZE`
5. **启动设备**：设置状态寄存器为 `VIRTIO_STATUS_DRIVER_OK`

### Virt Queue 初始化

不同种类的设备需要不同个数的 Virt Queue，队列结构和信息由`struct virtq_info`和`struct virtqueue`两个结构体记录，其结构如下：

```c
struct virtqueue
{
    // The actual descriptors (16 bytes each)
    union {
        void* base;
        volatile struct virtqueue_desc* desc;
    };

    // A ring of available descriptor heads with free-running index.
    volatile struct virtqueue_avail* avail;

    // A ring of used descriptor heads with free-running index.
    volatile struct virtqueue_used* used;
};

struct virtq_info
{
    /* Physical page frame number of struct virtqueue. */
    uint64 pfn;

    uint32 seen_used;
    uint32 free_desc;

    struct virtqueue virtq;
    void **desc_virt;

    uint32 queue_num;
    uint32 queue_size;
};
```

对于新分配的队列，初始化流程为：

1. 根据队列大小计算内存需求：
   - 描述符表 (`virtqueue_desc`)：`queue_size × 16字节`
   - 可用环 (`virtqueue_avail`)：`8 + queue_size × 2字节`
   - 已用环 (`virtqueue_used`)：`8 + queue_size × 8字节`
2. 严格对齐分配内存，队列的基地址和`virtqueue_used`的起始地址都要按照 4KB 对齐
3. 初始化链表：
   - 所有描述符按顺序链接，形成空闲链表
   - 可用环头尾索引清零
   - 设置已用环的 `flags` 与 `idx` 为0

### 中断通知

Virt Queue 在完成向描述符表填充缓冲区的工作后，向 QueueNotify 寄存器写入队列号，通知设备进行 IO 操作。

## Virtio / PCI

### PCI 设备寄存器

PCI 模式下寄存器布局为紧凑结构，寄存器布局为：

```c
typedef volatile struct __attribute__((packed)) {
    uint32 DeviceFeature;
    uint32 GuestFeature;
    uint32 QueueAddress;
    uint16 QueueSize;
    uint16 QueueSelect;
    uint16 QueueNotify;
    uint8 DeviceStatus;
    uint8 ISRStatus;
#ifdef VIRTIO_PCI_ENABLE_MSI_X
#define VIRTIO_MSI_NO_VECTOR 0xffff
    uint16 ConfigurationVector;
    uint16 QueueVector;
#endif
    uint32 Config[];
} virtio_pci_header;
```

寄存器字段含义和 MMIO 对应字段类似，其中最后两字段的寄存器只会在设备启用了 MSI-X 中断后启用。寄存器通过 BAR0 映射到 IO 空间。

### 设备初始化

PCI 设备初始化流程：

1. **验证身份**：检查 `VendorID=0x1af4` 和 `SubsystemDeviceID`（块设备 `0x2`，网卡 `0x1`）

2. **启用总线主控**：调用 `pci_enable_bus_mastering()` 

3. **中断模式选择**：根据中断模式启用相应中断

    ```c
    #ifdef VIRTIO_PCI_ENABLE_MSI_X
    pci_enable_msix(dev); // MSI-X模式
    #else
    pci_device_set_irq_line(); // INTx模式
    #endif
    ```

4. **特性协商**：`virtio_check_capabilities`函数，通过 `DeviceFeature/GuestFeature` 交互

5. **提交队列地址**：将 `virtqueue` 物理地址写入 `QueueAddress`

6. **最终启动**：设置设备状态为 `DRIVER_OK`

### Virt Queue 初始化

PCI 模式队列初始化与 MMIO 基本一致，但是没有队列大小的沟通机制，驱动只能接受设备给出的队列大小。

### 中断初始化

#### 传统中断

驱动通过 PCI 配置空间获取中断引脚号，调用 `pci_device_set_irq_line()` 绑定到系统中断控制器。触发流程为：

1. 设备操作导致中断断言
2. PCI 控制器转发中断信号
3. CPU 查询中断状态寄存器确认事件来源

#### MSI-X 中断

#### MSI-X 中断

MSI-X 提供非共享高性能中断：

1. **分配向量**：系统分配专属中断号

2. **绑定队列**：

```c
pci_msix_add_vector(dev, 0, PCI_MSIX_MSG_ADDR, intid);
WRITE16(header->QueueVector, 0); // 关联队列0
```

3. **配置空间关联**：设置 `ConfigurationVector` 对应配置变更事件

4. **直接通知**：设备通过内存写操作触发中断，无需中断引脚

## Virtio-blk

块设备驱动实现以下核心功能：

### 请求管理

- **请求分配**：使用 `virtq_alloc_desc()` 从队列描述符池分配三个连续描述符
  - 头部描述符：存放请求结构（类型+扇区号）
  - 数据描述符：指向用户缓冲区（读写区分 `VIRTQ_DESC_F_WRITE` 标志）
  - 尾部描述符：状态反馈区
- **队列提交**：组装描述符链后写入可用环（Avail Ring），更新 `idx` 并通知设备

### 中断处理

1. 中断响应：中断服务程序扫描已用环（Used Ring）

2. 结果解析：检查状态描述符的完成状态：

   ```c
   switch (req->status) {
   case VIRTIO_BLK_S_OK:  // 成功
   case VIRTIO_BLK_S_IOERR: // I/O错误
   ```

3. 资源回收：释放描述符回空闲链表

4. 请求完成：回调上层文件系统结束 I/O 等待

### 读写优化

- **扇区对齐**：强制请求大小为 512 字节的倍数
- **批量提交**：支持多描述符链并行处理
- **写缓存控制**：利用 `VIRTIO_BLK_F_CONFIG_WCE` 动态调整写策略

## Virtio-net

网络驱动实现高效数据包传输：

### 数据结构

- **双队列架构**：
  - RX队列：用于接收数据包
  - TX队列：用于发送数据包
- **元数据头**：每个包附加 10 字节的 `virtio_net_hdr` 存储校验信息

### 接收流程

1. **缓冲预分配**：初始化时向RX队列注册64个空包缓冲区

   ```c
   for (i=0; i<64; i++) {
     pkt = packet_alloc();
     desc1 = alloc_desc(); // 头描述符
     desc2 = alloc_desc(); // 数据包描述符
   }
   ```

2. **包到达处理**：

   - 中断触发后从已用环获取完成描述符
   - 解析元数据头和包缓冲区
   - 将有效载荷传递给上层网络栈（`eth_recv()`）

3. **缓冲区重用**：立即分配新包缓冲区并加入可用环

### 发送流程

1. **封装请求**：
   - 创建元数据头并设置 `gso_type=VIRTIO_NET_HDR_GSO_NONE`
   - 关联待发送的数据包
2. **提交队列**：组装描述符链（头+数据）并更新可用环
3. **异步通知**：写 `QueueNotify` 寄存器唤醒设备
4. **资源回收**：发送完成后中断释放元数据头和包内存

### 设备配置

- **MAC地址获取**：原子读取配置空间中的MAC地址字段

  ```c
   do {
     macbyte = cfg->mac[i];
     mb();
   } while(vdev->mac[i] != macbyte); // 确保读取一致性
  ```

- **多队列支持**：通过 `VIRTIO_NET_F_MQ` 特性启用多队列并行

- **状态同步**：使用配置空间 `status` 字段同步链路状态