#include <common.h>
#include <klib.h>
#include <mm/mm.h>
#include <mm/buddy.h>
#include <mm/memlayout.h>
#include <debug.h>

extern struct zone zone;
extern char end[];

void
test_buddy()
{
    // test for buddy_init
    for (int i = 0; i < MAX_ORDER; i++) {
        struct block* ptr = zone.free_area[i].free_list.next;
        int turn = zone.free_area[i].nr_free;
        while (turn-- > 0) {
            assert(ptr->next->prev = ptr);
            ptr = ptr->next;
        }
        assert(ptr = &zone.free_area[i].free_list);
    }
    PASS("pass budy_init test");
    
    // test for buddy_alloc
    int nr_free_0 = zone.free_area[0].nr_free;
    buddy_alloc(PGSIZE);
    void* block_1 = buddy_alloc(PGSIZE);
    void* block_2 = buddy_alloc(PGSIZE);
    void* block_3 = buddy_alloc(PGSIZE);
    assert(zone.free_area[0].nr_free == nr_free_0 - 4);
    for (int i = 4; i < nr_free_0; i++)
        buddy_alloc(PGSIZE);
    assert(zone.free_area[0].nr_free == 0);

    int nr_free_1 = zone.free_area[1].nr_free;
    buddy_alloc(PGSIZE);
    assert(zone.free_area[0].nr_free == 1);
    assert(zone.free_area[1].nr_free == nr_free_1 - 1);


    // test for buddy_free
    nr_free_0 = zone.free_area[0].nr_free;
    nr_free_1 = zone.free_area[1].nr_free;
    buddy_free(block_1, 0);
    buddy_free(block_2, 0);
    buddy_free(block_3, 0);
    assert(zone.free_area[0].nr_free == nr_free_0 + 1);
    assert(zone.free_area[1].nr_free == nr_free_1 + 1);

    PASS("pass alloc & free test");


    // test for recursive merge
    for (int i = 6; i < MAX_ORDER - 1; i++) {
        int nr_free_n = zone.free_area[i].nr_free;
        while (nr_free_n-- > 0)
            buddy_alloc(BLOCK_SIZE(i));
    }
    buddy_alloc(BLOCK_SIZE(6));

    for (int i = MAX_ORDER - 2; i >= 6; i--)
        assert(zone.free_area[i].nr_free == 1);

    PASS("pass buddy test");
}
