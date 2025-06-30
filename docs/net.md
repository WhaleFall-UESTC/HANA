# 网络栈

## 1. 概述

HANAOS 对 SOS (Stephen's OS) 的网络栈进行了移植，网络栈底层通过 netdev 注册的 Virtio-net 设备发送和接收数据包，持以太网帧处理、IP数据包路由和UDP通信功能。整体架构分为协议处理层和套接字接口层，提供基本的网络通信能力。

HANAOS 的网络栈实现了以下核心功能：

- **链路层**：以太网帧处理（MAC地址匹配/广播处理）
- **网络层**：IPv4协议支持（数据包路由、IP头校验和计算）
- **传输层**：UDP协议支持（端口映射、数据报传输）
- **接口层**：BSD风格套接字API（socket, bind, connect, send, recv）

## 2. 核心数据结构

### 2.1 网络接口(netif)
```c
struct netif {
    uint32 ip;          // IPv4地址
    uint8 mac[6];       // MAC地址
    uint32 gateway_ip;  // 网关IP
    uint32 subnet_mask; // 子网掩码
    uint32 dns;         // DNS服务器
};
```

### 2.2 数据包(packet)
```c
struct packet {
    // 各层协议头指针
    void *ll;   // 链路层(以太网)
    void *nl;   // 网络层(IP)
    void *tl;   // 传输层(UDP)
    void *al;   // 应用层
    
    void *end;  // 数据包结束位置
    struct list_head list; // 队列指针
    uint32 capacity;      // 容量
    uint8 data[0];        // 数据负载
};
```

### 2.3 套接字(socket)
```c
struct socket {
    // 状态标志
    struct {
        int sk_bound : 1;
        int sk_connected : 1;
        int sk_open : 1;
    } flags;
    
    // 地址信息
    struct sockaddr_in src;  // 本地地址
    struct sockaddr_in dst;  // 目标地址
    
    struct sockops *ops;     // 协议操作函数
    struct list_head recvq;  // 接收队列
    struct netdev *netdev;   // 关联的网络设备
};
```

## 3. 协议栈分层

### 3.1 以太网层(eth)

**核心功能**：处理 MAC 层通信

```c
void eth_recv(struct netif *netif, struct packet *pkt) {
    pkt->nl = pkt->ll + sizeof(struct etherframe); // 从以太网帧后开始IP头

    // 检查目标MAC是否匹配本机或广播
    if (memcmp(pkt->eth->dst_mac, broadcast_mac, MAC_SIZE) == 0 ||
        memcmp(pkt->eth->dst_mac, netif->mac, MAC_SIZE) == 0) {
        switch (ntohs(pkt->eth->ethertype)) {
        case ETHERTYPE_IP:
            pkt->tl = pkt->nl + ip_get_length(pkt->ip); // IP头后是传输层头
            ip_recv(netif, pkt); // 传递给IP层
            break;
        // 其他协议处理...
        }
    } else {
        packet_free(pkt); // 丢弃非本机数据包
    }
}
```

**设计说明**：
- 使用`struct packet`作为贯穿各层的统一数据结构
- 通过指针位移实现零拷贝：`pkt->ll`, `pkt->nl`, `pkt->tl`, `pkt->al`分别指向链路层、网络层、传输层和应用层数据
- 协议头使用压缩布局(`packed`属性)，减少内存占用

### 3.2 IP层
**核心功能**：IP数据包路由和转发

```c
void ip_recv(struct netif *netif, struct packet *pkt) {
    // 检查目标IP是否匹配本机
    if (pkt->ip->dst != netif->ip && pkt->ip->dst != 0xFFFFFFFF) {
        goto cleanup; // 丢弃非本机数据包
    }
    
    // 添加到ARP表
    upsert_mapping(pkt->ip->src, pkt->eth->src_mac);
    
    switch (pkt->ip->proto) {
    case IPPROTO_UDP:
        udp_recv(netif, pkt); // 传递给UDP层
        return;
    }
cleanup:
    packet_free(pkt);
}
```

### 3.3 UDP层

**核心功能**：提供无连接的数据报服务

```c
// UDP端口映射
#define UDP_HLIST_SIZE 128
struct hlist_head udp_hlist[UDP_HLIST_SIZE];

void udp_recv(struct netif *netif, struct packet *pkt) {
    // 根据目标端口哈希查找
    uint32 hash = udp_hash(ntohs(pkt->udp->dst_port));
    
    struct udp_wait_entry *entry;
    list_for_each_entry(entry, &udp_hlist[hash], list) {
        if (entry->sock) {
            // 数据包加入socket接收队列
            list_insert_end(&entry->sock->recvq, &pkt->list);
            wakeup(&entry->sock); // 唤醒等待进程
            return;
        }
    }
    packet_free(pkt); // 无匹配端口则释放
}
```

**处理流程**：
1. 通过`udp_hash()`计算目标端口哈希值
2. 遍历哈希桶查找监听该端口的socket
3. 数据包插入socket接收队列
4. 唤醒可能阻塞在`recv()`系统调用的进程

## 4. 套接字接口

### 4.1 套接字创建
```c
struct socket* socket_socket(int domain, int type, int protocol) {
    // 仅支持IPv4和UDP
    if (domain != AF_INET || type != SOCK_DGRAM) 
        return ERR_PTR(-EAFNOSUPPORT);
    
    struct socket *sock = kalloc(sizeof(struct socket));
    memset(sock, 0, sizeof(struct socket));
    sock->ops = &udp_ops; // 绑定UDP操作
    INIT_LIST_HEAD(sock->recvq);
    sock->netdev = netdev_get_default_dev(); // 关联默认网卡
    return sock;
}
```

### 4.2 数据发送流程
```c
int udp_sys_send(struct socket *sock, const void *data, size_t len, int flags) {
    // 分配数据包内存
    int space = udp_reserve();
    struct packet *pkt = packet_alloc();
    
    // 填充应用数据
    memcpy(pkt->app, data, len);
    pkt->end = pkt->app + len;
    
    // 通过协议栈发送
    udp_send(&sock->netdev->netif, pkt, 
             sock->src.sin_addr.s_addr, // 源IP
             sock->dst.sin_addr.s_addr, // 目标IP
             sock->src.sin_port,        // 源端口
             sock->dst.sin_port);       // 目标端口
    return len;
}

// 完整的协议头构建
void udp_send(struct netif *netif, struct packet *pkt, uint32 src_ip,
              uint32 dst_ip, uint16 src_port, uint16 dst_port) 
{
    pkt->tl = pkt->al - sizeof(struct udphdr); // 传输层位置
    
    // 填充UDP头
    pkt->udp->src_port = src_port;
    pkt->udp->dst_port = dst_port;
    pkt->udp->len = htons(pkt->end - pkt->tl);
    
    // 计算校验和
    csum_init(&csum);
    csum_add(&csum, (uint16 *)&src_ip, 2);
    // ... 省略其他校验和字段
    pkt->udp->csum = csum_finalize(&csum);
    
    // 传递给IP层
    ip_send(netif, pkt, IPPROTO_UDP, src_ip, dst_ip);
}
```

**优化策略**：
- 预计算并保留协议头空间，避免内存复制
- 分层处理协议，各层仅关注自身职责
- 自动绑定源IP源端口，简化应用开发

### 4.3 数据接收流程
```c
ssize_t socket_read(struct file *file, char *buffer, size_t size, off_t *offset) {
    struct socket *sock = (struct socket *)file->f_private;
    
    // 从接收队列获取数据包
    struct packet *pkt = socket_recvq_get(sock);
    while (!pkt) {
        sleep(sock); // 阻塞等待数据
        pkt = socket_recvq_get(sock);
    }
    
    // 复制数据到用户空间
    size_t pktlen = pkt->end - pkt->al;
    memcpy(buffer, pkt->al, pktlen);
    
    list_remove(&pkt->list);
    packet_free(pkt);
    return pktlen;
}
```

## 5. 系统调用接口

| 系统调用 | 功能描述 |
|---------|---------|
| socket() | 创建新套接字 |
| bind() | 绑定本地地址 |
| connect() | 连接远程地址 |
| send() | 发送数据 |
| recv() | 接收数据 |

```c
// connect系统调用实现
SYSCALL_DEFINE3(connect, int, fd, const struct sockaddr *, address) {
    struct file *file = fd_get(myproc()->fdt, fd);
    struct socket *sock = file->f_private;
    
    // 复制用户空间地址参数
    struct sockaddr_in addr;
    copy_from_user(&addr, address, address_len);
    
    // 设置目标地址
    sock->dst = addr;
    sock->flags.sk_connected = 1;
    return 0;
}
```

```c
// socket系统调用实现
SYSCALL_DEFINE3(socket, int, domain, int, type, int, protocol) {
    struct file *file = kcalloc(sizeof(struct file), 1);
    int ret = socket_init(file); // 创建socket结构
    
    if (ret < 0) {
        kfree(file);
        return ret;
    }
    
    // 分配文件描述符
    return fd_alloc(myproc()->fdt, file);
}

// 文件操作绑定
static struct file_operations socket_fops = {
    .read = socket_read,
    .write = socket_write,
};
```

**架构特点**：
- 通过文件描述符抽象网络套接字
- 为文件系统兼容层注册了文件接口，可以通过文件访问的方式访问套接字
- 进程表管理套接字引用计数

## 6. 工具函数

### 地址转换函数
```c
// 32位主机/网络字节序转换
uint32 htonl(uint32 orig) {
    return ((orig & 0xFF) << 24) | ((orig & 0xFF00) << 8) |
           ((orig & 0xFF0000) >> 8) | ((orig & 0xFF000000) >> 24);
}

// IPv4地址转字符串
char* ip_ntoa(uint32 ip) {
    static char buf[16];
    sprintf(buf, "%u.%u.%u.%u", 
        (ip >> 24) & 0xFF, (ip >> 16) & 0xFF,
        (ip >> 8) & 0xFF, ip & 0xFF);
    return buf;
}

// MAC地址转字符串
char* mac_ntoa(uint8 mac[6]) {
    static char buf[18];
    sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return buf;
}
```

**实用功能**：
- 提供完整的字节序转换工具
- 支持点分十进制IP地址格式化