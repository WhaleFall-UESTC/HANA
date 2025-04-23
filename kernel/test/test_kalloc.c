#include <common.h>
#include <klib.h>
#include <mm/buddy.h>
#include <mm/slab.h>
#include <mm/mm.h>
#include <debug.h>

void 
test_kalloc()
{
    void* tmp_1pg = kalloc(PGSIZE);
    void* tmp_4pg = kalloc(PGSIZE << 2);
    void* tmp_8obj = kalloc(OBJECT_SIZE << 3);

    kfree(tmp_1pg);
    kfree(tmp_4pg);
    kfree(tmp_8obj);

    for (int i = 0; i < MAX_ORDER - 1; i++) {
        void* tmp = kalloc(PGSIZE << i);
        void* tmp2 = kalloc((PGSIZE << i) + PGSIZE);
        memset(tmp, 0, PGSIZE << i);
        memset(tmp2, 0, PGSIZE << (i + 1));
        kfree(tmp);
        kfree(tmp2);
    }
    void* tmp = kalloc(PGSIZE << (MAX_ORDER - 1));
    kfree(tmp);


    for (int i = 1; i <= NR_OBJS; i++) {
        // log("slab free %d objs", i);
        void* tmp = kalloc(i * OBJECT_SIZE);
        kfree(tmp);
    }

    PASS("kalloc test passed");
}