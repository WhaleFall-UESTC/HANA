# slab 分配器

与伙伴系统相对，slab 分配器负责分配小块的连续内存分配。slab 从伙伴系统中获取大小为 PGSIZE（4096 字节） 的区域，称之为 slab，并将 slab 分成数个小块，称之为 object。在 HANA 的设计中，slab 可以连续分配 0 - 61 个 object 的区域，一个 object 的大小为 64 字节

<br/>

## struct slab

大小为 4096 字节，分成两个区域，其中元数据大小为 3*64 字节，是 slab 分配器管理的核心；另外一部分则为分配出去的内存

```c_cpp
struct slab {
    /* meta data */
    uint16 next_low;
    uint8  next_high;
    uint16 prev_low;
    uint8  prev_high;
    struct object_entry sentinel;
    struct object_entry objs[NR_OBJS];
    
    struct object objects[NR_OBJS];
};
```

下面解析各个参数的含义：

### next & prev

`next_low` 与 `next_high` 存储的是经过压缩之后的指针，指向 slab 链表中下一跳 slab 的地址。因为 slab 是由伙伴系统所分配的页对齐的内存，其地址的低 12 位必定为 0；所有 slab 地址的高 32 位全部一致；而 next_low 与 next_high 的组合表示其中间的 24 位（其中 next_high 只有 4 位是有效的）

还原指针可以参考下面的代码：

```c_cpp
static inline struct slab*
get_slab_next(struct slab* s) 
{
    uint32 next_low = (((s->next_high << 16) | s->next_low) << 12);
    return (struct slab *) ((((uint64) s) & 0xffffffff00000000) | next_low);
}
```

|位|值|
|--|--|
|0 - 11|0x000|
|12 - 27|next_low|
|28 - 31|next_high|
|32 - 63|与当前的 slab 相同|

`prev_low` `prev_high` 同理

这里的指针与两个全局变量有关 current 与 partial。current 是指向当前正在使用的 slab 的指针，partial 是一个有长度限制的 slab 链表（不得少于 MIN_PARTIAL = 3）

<br/>

### sentinel

首先先解释下其数据类型

```c_cpp
struct object_entry {
    uint8 size;
    int8 prev;
    int8 next;
};
```

这是一个环形双向链表的节点的定义，其中 size 表示着一个节点拥有多少个 objects，prev, next  表示下一个节点的开始的位置（即存储下一个节点信息在 objs[prev]，而分配的开始在 &objects[next]）。sentinel 就是这个环形双向链表的“哨兵节点”，其中 size 表示这个 slab 拥有的还未被分配的 object 的数量，prev, size 指向这个链表中第一个节点和最后一个节点的位置（最开始指向自己，即 -1）

这是最开始的设计的残余。实际上，为了方便合并，在节点所占有的块的最后还做了标记。所以这里完全可以使用隐式链表来实现

<br/>

### objs

参照上面的 sentinel 的描述

<br/>

### objects

```c_cpp
struct object {
    char buf[OBJECT_SIZE];
};
```

实质上是占用 64 字节的区域，在 slab 中实际用于分配

<br/>

下面举一个例子来解释用法：

slab 接收到了一个分配任务，要分配 128 字节大小的区域，也就是两个 object。此时查看 current 指向的 slab 所持有的 object 数量（current->sentinel.size），发现可以分配。接着，遍历节点链表，看到 sentinel.next 的值是 5。接着查看 objs[5].size，大于 2，于是将其值 -2，收缩这个节点的大小，然后返回 &current->objects[5]

<br/>

## slab 的分配流程

上面只是举了一个简单的例子，实际上会遇到的情况会比较复杂，下面实际说明：

首先先要寻找一个能够分配的 slab。首先先找 current，如果 current 不能分配的话，就在 partial 里面寻找，找到后，设定其为 current（以前的实现是将其移出 partial，成为新的 current，并将旧 current 放入 partial）

找到可以分配的 slab，使用这个 slab 分配

通过上文介绍的双向链表，匹配第一个合适的节点进行分配。在分配完成之后，你不仅需要修改 size 的值，你还需要修改这个节点的大小：每一个节点的开头会写上这个节点的元数据：两个指针一个 size，并且在这个节点的末尾还会有标记，末尾的 prev 设置位 E，next 设置为 D，size 仍然表示这个节点所持有的 object 数量，你需要改变节点末尾的位置。如果刚好分配完，你需要将其从链表中移除

其中被分配出去的块也有标记 A, L。~~虽然我已经忘记具体的实现了~~

不过这种设计有一个问题，那就是只分配一个 slab 需要特殊处理

<br/>

### slab 释放流程

当得到一个要释放的地址时，对其 PGROUNDDOWN 就可以得到它所对应的 struct slab* 指针

然后就可以对其操作了

首先，将这个块回收，重新加入链表。如果有节点的块与之相邻，则进行合并，可能向前或向后合并，或着两边都合（具体怎么合看你的实现）

如果这一个 free 直接将整个 slab 都 free 完了，在保证 slab 数量足够的前提下（也就是 partial 的长度大于 MIN_PARTIAL），就调用 buddy_free(struct slab*) 将其还给伙伴系统

<br/>

当然，最终的实现取决于你，你只需要实现如下的接口：

```c_cpp
/*****************************************************
 * 返回一个指针，指向一块大小大于等于 sz 的连续的地址
 * @Param sz 需要的内存大小
 * @Return 指向一块大小大于等于 sz 的连续的地址的指针
 */
void* slab_alloc(uint64 sz);


/*****************************************************
 * 回收由 slab 分配出去的内存
 * @Param addr 指向曾由 slab 分配出去的内存的指针
 * @Param nr_free 需要释放的 object 数量
 */
void slab_free(void* addr, uint8 nr_free);
```