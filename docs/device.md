# 设备管理子系统

HANAOS 为各种各样的设备设计了统一的设备抽象：`struct device`，作为设备管理子系统的核心对象。围绕这个类，设备管理子系统将各个种类的设备保存在一个链表中，提供统一的注册和初始化操作，为驱动提供设备的注册接口，管理中断操作。

## device 类

device 类是通用的设备抽象，其定义如下：

```c
struct device {
    devid_t devid;                  // device number
    uint32 intr;                    // irq vec number

    char name[DEV_NAME_MAX_LEN];    // device name

    struct list_head dev_entry;     // entry in device list
    spinlock_t dev_lock;            // lock for accessing the device

    enum devtype {
        DEVICE_TYPE_BLOCK,
        DEVICE_TYPE_CHAR,
        DEVICE_TYPE_NET,
        DEVICE_TYPE_OTHER,
        DEVICE_TYPE_ANY,
    } type;                         // device type
};
```

`struct device`记录了设备 id、中断号、设备名称等关键信息，包含了对设备进行原子访问的锁字段，`type`字段用枚举记录了设备的类型，方便将`struct device`作为其他类型的设备的数据成员后判断其所在的结构体。`device_init`方法可以初始化传入的`struct device`指针对象。

通过`struct device`的`dev_entry`字段，每一个设备结构通过一个链表串联在一起。device 类提供了三种在设备链表中寻找指定设备的方法，指定设备类型后分别根据设备号、设备名称寻找以及返回链表的第一个设备。除此之外，还提供了宏`device_list_for_each_entry_locked`，原子地遍历设备链表。

`device_register`方法将设备添加到设备链表中并注册它的中断处理函数。

## blkdev 类

块设备类是随机读写设备的抽象，也就是磁盘设备驱动的抽象。块设备类在 device 类的基础上，记录了块设备的基础信息，如设备大小、扇区大小等。其定义如下：

```c
struct blkdev
{
    struct device dev; // base device struct
    unsigned long size; // blkdev capacity in bytes
    uint64 sector_size;
    const struct blkdev_ops *ops;
    struct list_head rq_list;  // list head for requests
    spinlock_t rq_list_lock;
};
```

块设备进行 I/O 操作的方式是提交 I/O 请求，每个请求记录了请求的参数、请求执行的状态等，定义如下：

```c
struct blkreq
{
    enum blkreq_type
    {
        BLKREQ_TYPE_READ,
        BLKREQ_TYPE_WRITE
    } type;

    sector_t sector_sta;
    uint64 size;
    void *buffer;

    // request result
    enum blkreq_status
    {
        BLKREQ_STATUS_INIT,
        BLKREQ_STATUS_OK,
        BLKREQ_STATUS_ERR
    } status;

    struct list_head rq_head;
    struct blkdev *rq_dev;

    // 请求创建者自己注册的，请求完成后调用的回调函数
    void (*endio)(struct blkreq *);
};
```

块设备维护了自己的请求链表，即记录了所有在自己上正在执行的`struct blkreq`。

块设备对底层的驱动程序提供了几个注册接口，驱动通过提供接口注册的函数来实现自己的功能，其定义如下：

```c
struct blkdev_ops
{
    struct blkreq *(*alloc)(struct blkdev *);
    void (*free)(struct blkdev *, struct blkreq *);
    void (*submit)(struct blkdev *, struct blkreq *);
    void (*status)(struct blkdev *);
    irqret_t (*irq_handle)(struct blkdev *);
};
```

其中包含对 I/O 请求的创建、释放和提交，打印块设备状态和块设备的中断处理函数。

块设备提供了`struct blkdev`的创建、初始化和处理等函数，也提供了块设备的通用中断处理函数供`device_init`函数注册。对于中断请求，通过`blkreq_alloc`进行创建之后通过`blkdev_submit_req`提交给设备驱动（这是一个异步函数），最终通过`blkdev_wait_all`等待一个块设备的所有关联请求结束，用`blkdev_free_all`将他们全部释放。除此之外，也可以调用同步函数`blkdev_submit_req_wait`在提交请求之后等待请求完成。

## chrdev 类

字符设备是单字符读写设备的抽象，如串口、鼠标、键盘等。字符设备相较于 device 类只多记录了设备操作，两个结构体定义如下：

```c
struct chrdev
{
    struct device dev; // base device struct
    const struct chrdev_ops *ops;
};

struct chrdev_ops
{
    void (*putchar)(struct chrdev *, char);
    char (*getchar)(struct chrdev *);
    irqret_t (*irq_handle)(struct chrdev *);
};
```

这里定义了字符设备的中断处理接口和两个核心操作：读写一个字符。除此之外，功能和去除了 I/O 请求的块设备完全相同。

## netdev 类

网络设备是物理层网卡设备的抽象。其关键结构如下：

```c
struct netdev {
    struct device dev; // base device struct
    struct netif netif; // network interface this device is associated with
    const struct netdev_ops *ops;
};

struct netdev_ops {
    void (*send)(struct netdev *, struct packet *);
    void (*status)(struct netdev *);
    irqret_t (*irq_handle)(struct netdev *);
};
```

网络设备记录了绑定的网络接口，其操作除了和上述提到过的类似的之外，send 接口提供了发送一个包的功能。