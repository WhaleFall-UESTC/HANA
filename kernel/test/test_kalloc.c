#include <defs.h>
#include <buddy.h>
#include <slab.h>
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

    for (int i = 0; i < MAX_ORDER; i++) {
        void* tmp = kalloc(PGSIZE << i);
        // log("alloc big, tmp: %p", tmp);
        kfree(tmp);
    }

    for (int i = 1; i <= NR_OBJS; i++) {
        // log("slab free %d objs", i);
        void* tmp = kalloc(i * OBJECT_SIZE);
        kfree(tmp);
    }
}