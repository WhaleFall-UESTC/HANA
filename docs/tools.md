# WhaleOS 工具说明

## tools/kfifo.h

这是从 linux 远古源码中取得的**环形缓冲区**，可以用于各种内核缓冲，目前用在了 `pipe` 上面。

具体的 api 说明见`tools/kfifo.h`里的注释，注意`get/put`的接口有带锁和不带锁两个版本，无锁的版本在消费者和生产者都不超过 1 个时适用。

## tools/list.h

这是 linux-style 的**链表头**。`struct list_head`应该作为结构体的数据成员被声明，使用`INIT_LIST_HEAD`进行初始化，`list_insert`进行插入元素和`list_for_each_entry`进行遍历，其他 api 见注释。

值得注意的是，`struct list_head`既可以代表链表头，也可以是链表的元素，当使用`list_for_each_entry`时默认传入的参数是链表头，不当作元素处理。

## locking/atomic.h

这是**原子数据类型**相关操作。使用时，`atomic_define`定义一种新的原子类型。

比如当你使用`atomic_define(int, a)`的时候，相当于

```c
struct { int val; spinlock_t lock; } a;
```

此时生成了一个匿名结构体，并且一个原子变量`a`被定义了。然后你需要使用`atomic_init`对他进行初始化设置初值才能使用。

之后可以使用`atomic_inc`、`atomic_set`等操作使用这个原子变量。

此外还可以使用`atomic_declear`先定义一个特定内置数据类型的原子类型，如当你使用`atomic_declear(int)`时，相当于

```c
typedef struct { int val; spinlock_t lock; } atomic_int_t;
```

之后你就可以使用`atomic_int_t`来声明新的`int`类型的原子变量了。