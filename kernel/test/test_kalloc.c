#include <common.h>
#include <klib.h>
#include <mm/buddy.h>
#include <mm/slab.h>
#include <mm/mm.h>
#include <debug.h>

void test_kalloc_kfree()
{
    // 第一轮分配
    void *p1 = kalloc(4096); // 0x805dc000
    void *p2 = kalloc(144);  // 0x80643c80
    void *p3 = kalloc(4096); // 0x805db000
    void *p4 = kalloc(144);  // 0x80643bc0
    void *p5 = kalloc(128);  // 0x80643b40
    kfree(p5);               // 释放 128 字节块

    // 第二轮分配
    void *p6 = kalloc(4096); // 0x805da000
    void *p7 = kalloc(144);  // 0x80643b00
    void *p8 = kalloc(128);  // 0x80643a80
    kfree(p8);               // 释放 128 字节块

    // 第三轮分配
    void *p9 = kalloc(4096); // 0x805d9000
    void *p10 = kalloc(144); // 0x80643a40
    void *p11 = kalloc(128); // 0x806439c0
    kfree(p11);              // 释放 128 字节块

    // 复用已释放块
    void *p12 = kalloc(128);  // 0x806439c0（复用）
    void *p13 = kalloc(4096); // 0x805d8000
    void *p14 = kalloc(144);  // 0x80643900
    kfree(p12);               // 再次释放 128 字节块

    // 第四轮分配
    void *p15 = kalloc(4096); // 0x805d7000
    void *p16 = kalloc(144);  // 0x80643840
    void *p17 = kalloc(128);  // 0x806437c0
    kfree(p17);               // 释放 128 字节块

    // 第五轮分配（多次复用）
    void *p18 = kalloc(4096); // 0x805d6000
    void *p19 = kalloc(144);  // 0x80643780
    void *p20 = kalloc(128);  // 0x80643700
    kfree(p20);               // 首次释放
    void *p21 = kalloc(128);  // 0x80643700（复用）
    kfree(p21);               // 二次释放
    void *p22 = kalloc(128);  // 0x80643700（再复用）
    kfree(p22);               // 最终释放

    // 第六轮分配
    void *p23 = kalloc(4096); // 0x805d5000
    void *p24 = kalloc(144);  // 0x806436c0
    void *p25 = kalloc(128);  // 0x80643640
    kfree(p25);               // 释放 128 字节块

    // 清理大块内存
    kfree(p13); // 释放 0x805d8000（4096 字节）
    kfree(p14); // 释放 0x80643900（144 字节）

    // 第七轮分配
    void *p26 = kalloc(128); // 0x80643640（复用）
    kfree(p26);
    void *p27 = kalloc(128);  // 0x80643640（再复用）
    void *p28 = kalloc(4096); // 0x805d8000（复用大块）
    void *p29 = kalloc(144);  // 0x80643580
    kfree(p27);               // 释放 128 字节块

    // 最终清理
    kfree(p6); // 释放初始分配的 0x805da000

    kfree(p29);
    kfree(p28);
    kfree(p24);
    kfree(p23);
    kfree(p19);
    kfree(p18);
    kfree(p16);
    kfree(p15);
    kfree(p15);
    kfree(p10);
    kfree(p9);
    kfree(p7);
    kfree(p4);
    kfree(p3);
    kfree(p2);
    kfree(p1);
}

void test_kalloc()
{
    void *tmp_1pg = kalloc(PGSIZE);
    void *tmp_4pg = kalloc(PGSIZE << 2);
    void *tmp_8obj = kalloc(OBJECT_SIZE << 3);

    kfree(tmp_1pg);
    kfree(tmp_4pg);
    kfree(tmp_8obj);

    PASS("pass single alloc");

    for (int i = 0; i < MAX_ORDER - 1; i++)
    {
        debug("order: %d", i);
        void *tmp = kalloc(PGSIZE << i);
        void *tmp2 = kalloc((PGSIZE << i) + PGSIZE);
        memset(tmp, 0, PGSIZE << i);
        memset(tmp2, 0, PGSIZE << (i + 1));
        kfree(tmp);
        kfree(tmp2);
    }
    void *tmp = kalloc(PGSIZE << (MAX_ORDER - 1));
    kfree(tmp);

    PASS("pass buddy alloc");

    for (int i = 1; i <= NR_OBJS; i++)
    {
        // log("slab free %d objs", i);
        void *tmp = kalloc(i * OBJECT_SIZE);
        kfree(tmp);
    }
    PASS("pass slab alloc");

    test_kalloc_kfree();

    PASS("kalloc test passed");
}