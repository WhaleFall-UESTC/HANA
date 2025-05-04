#include <common.h>
#include <klib.h>
#include <mm/buddy.h>
#include <mm/slab.h>
#include <mm/mm.h>
#include <debug.h>

extern uint partial_len;
extern struct slab *current, *partial;

void test_slab() {
    log("slab test start");

    void* tmp[NR_OBJS] = {};
    void* tmp1[NR_OBJS] = {};
    void* tmp2[NR_OBJS] = {};
    void* tmp3[NR_OBJS] = {};
    for (int i = 1; i <= NR_OBJS; i++) {
        debug_dynamic("slab alloc, i = %d", i);
        tmp[i - 1] = kalloc(i * OBJECT_SIZE);
        tmp1[i - 1] = kalloc(OBJECT_SIZE);
        tmp2[i - 1] = kalloc(OBJECT_SIZE * 2);
    }

    log("slab alloc finish");

    for (int i = NR_OBJS - 1; i >= 0; i--) {
        if (i > 15) {
            debug_dynamic("slab free, i = %d, nr_objs = %d, free %p", i, 1, tmp1[i]);
            kfree(tmp1[i]);
        }

        if (i % 2 == 0) {
            debug_dynamic("slab free, i = %d, nr_objs = %d, free %p", i, 2, tmp2[i]);
            kfree(tmp2[i]);
        } else {
            debug_dynamic("slab free, i = %d, nr_objs = %d, free %p", i, i + 1, tmp[i]);
            kfree(tmp[i]);
        }
    }

    for (int i = 0; i < NR_OBJS; i++) {
        // i == 57 0x805af000
        debug_dynamic("slab alloc, i = %d, nr_objs = %d", i, 3);
        tmp3[i] = kalloc(OBJECT_SIZE * 3);
        if (i >= 5 && i <= 15) {
            debug_dynamic("slab free, i = %d, nr_objs = %d, free %p", i, 1, tmp1[i]);
            kfree(tmp1[i]);
        }
    }

    for (int i = 0; i < NR_OBJS; i++) {
        if (i % 2 != 0) {
            debug_dynamic("slab free, i = %d, nr_objs = %d, free %p", i, 2, tmp2[i]);
            kfree(tmp2[i]);
        } else {
            debug_dynamic("slab free, i = %d, nr_objs = %d, free %p", i, i + 1, tmp[i]);
            kfree(tmp[i]);
        }

        debug_dynamic("slab free, i = %d, nr_objs = %d, free %p", i, 3, tmp3[i]);
        // slab free, i = 59, nr_objs = 3, free 805b94c0; partial = NULL
        kfree(tmp3[i]);

        if (i < 5) {
            debug_dynamic("slab free, i = %d, nr_objs = %d, free %p", i, 1, tmp1[i]);
            kfree(tmp1[i]);
        }
    }

    PASS("PASS slab test!!!");
}

// void 
// test_slab() 
// {
//     assert((ROUNDUP(NR_OBJS * OBJECT_SIZE, OBJECT_SIZE) >> OBJECT_SHIFT)== NR_OBJS);

//     // slab_alloc and slab_free
//     void* tmp[MIN_PARTIAL + 1];
//     struct slab* origin[MIN_PARTIAL + 1];
//     origin[0] = current;
//     origin[1] = partial;
//     for (int i = 2; i <= MIN_PARTIAL; i++) {
//         origin[i] = get_slab_next(origin[i - 1]);
//     }
//     for (int i = 0; i <= MIN_PARTIAL; i++) {
//         // log("alloc slab %d", i);
//         tmp[i] = slab_alloc(NR_OBJS * OBJECT_SIZE);
//         assert(current == origin[i]);
//     }

//     uint8 size_div_3 = NR_OBJS / 3;
//     uint8 size_mod_3 = NR_OBJS - 3 * size_div_3;
//     void* fragment0 = slab_alloc(size_mod_3 * OBJECT_SIZE);
//     struct slab* s = SLAB(fragment0);
//     assert(s->sentinel.prev == 0);
//     assert(s->sentinel.next == 0);
//     assert(s->objs[0].prev == OBJECT_SENTINEL);
//     assert(s->objs[0].next == OBJECT_SENTINEL);
//     assert(s->objs[0].size = NR_OBJS - size_mod_3);
//     assert(s->objs[OBJECT_IDX(fragment0)].size == size_mod_3);
//     assert(s->objs[OBJECT_IDX(fragment0)].prev == A);
//     assert(s->objs[OBJECT_IDX(fragment0)].next == L);
//     // log("Alloc fragment0, this time sentinel: size: %d, prev: %d, next: %d", (int)s->sentinel.size, (int)s->sentinel.prev, (int)s->sentinel.next);
//     void* fragment1 = slab_alloc(size_div_3 * OBJECT_SIZE);
//     // log("Alloc fragment1, this time sentinel: size: %d, prev: %d, next: %d", (int)s->sentinel.size, (int)s->sentinel.prev, (int)s->sentinel.next);
//     // slab will call buddy_alloc
//     assert(partial_len == MIN_PARTIAL + 1);
//     assert(s->objs[OBJECT_IDX(fragment1)].size == size_div_3);
//     assert(s->objs[OBJECT_IDX(fragment1)].prev == A);
//     assert(s->objs[OBJECT_IDX(fragment1)].next == L);

//     void* fragment2 = slab_alloc(size_div_3 * OBJECT_SIZE);
//     assert(s->objs[OBJECT_IDX(fragment2)].size == size_div_3);
//     assert(s->objs[OBJECT_IDX(fragment2)].prev == A);
//     assert(s->objs[OBJECT_IDX(fragment2)].next == L);
//     // log("Alloc fragment2, this time sentinel: size: %d, prev: %d, next: %d", (int)s->sentinel.size, (int)s->sentinel.prev, (int)s->sentinel.next);
    
//     void* fragment3 = slab_alloc(size_div_3 * OBJECT_SIZE);
//     assert(s->objs[OBJECT_IDX(fragment3)].size == size_div_3);
//     assert(s->objs[OBJECT_IDX(fragment3)].prev == A);
//     assert(s->objs[OBJECT_IDX(fragment3)].next == L);
//     // log("Alloc all slab, this time sentinel: size: %d, prev: %d, next: %d", (int)s->sentinel.size, (int)s->sentinel.prev, (int)s->sentinel.next);
//     assert(s->sentinel.prev == OBJECT_SENTINEL);
//     assert(s->sentinel.next == OBJECT_SENTINEL);

//     for (int i = 0; i <= MIN_PARTIAL; i++)
//         if ((uint64)tmp[i] != (uint64)s) {
//             slab_free(tmp[1], NR_OBJS);
//             break;
//         }
//     assert(partial_len == MIN_PARTIAL);
//     PASS("pass partial len test");

//     assert(nr_free_objs(s) == 0);

//     // for (int i = 0; i < NR_OBJS; i++) {
//     //     assert(s->objs[i].size == 0);
//     //     assert(s->objs[i].prev == 0);
//     //     assert(s->objs[i].next == 0);
//     // }

//     PASS("pass slab alloc test");

//     // log("before free, this time sentinel: size: %d, prev: %d, next: %d", (int)s->sentinel.size, (int)s->sentinel.prev, (int)s->sentinel.next);
//     slab_free(fragment1, size_div_3);
//     // log("free fragment1, this time sentinel: size: %d, prev: %d, next: %d", (int)s->sentinel.size, (int)s->sentinel.prev, (int)s->sentinel.next);
//     assert(s->sentinel.prev == 2 * size_div_3);
//     assert(s->objs[2 * size_div_3].prev == OBJECT_SENTINEL);
//     assert(s->objs[2 * size_div_3].next == OBJECT_SENTINEL);
//     assert(s->objs[3 * size_div_3 - 1].size == size_div_3);
//     assert(s->objs[3 * size_div_3 - 1].prev == E);
//     assert(s->objs[3 * size_div_3 - 1].next == D);

//     slab_free(fragment3, size_div_3);
//     // log("free fragment3, this time sentinel: size: %d, prev: %d, next: %d", (int)s->sentinel.size, (int)s->sentinel.prev, (int)s->sentinel.next);
//     assert(s->sentinel.prev == 0);
//     assert(s->sentinel.next == 2 * size_div_3);
//     assert(nr_free_objs(s) == 2 * size_div_3);
//     assert(s->objs[0].prev == 2 * size_div_3);
//     assert(s->objs[0].next == OBJECT_SENTINEL);
//     assert(s->objs[size_div_3 - 1].size == size_div_3);
//     assert(s->objs[size_div_3 - 1].prev == E);
//     assert(s->objs[size_div_3 - 1].next == D);

//     slab_free(fragment2, size_div_3);
//     // log("free fragment2, this time sentinel: size: %d, prev: %d, next: %d", (int)s->sentinel.size, (int)s->sentinel.prev, (int)s->sentinel.next);
//     assert(s->sentinel.prev == 0);
//     assert(s->sentinel.next == 0);
//     assert(nr_free_objs(s) == NR_OBJS - size_mod_3);
//     assert(s->objs[0].size == NR_OBJS - size_mod_3);
//     assert(s->objs[0].prev == OBJECT_SENTINEL);
//     assert(s->objs[0].next == OBJECT_SENTINEL);
//     // // log("objs[0].size: %d", (int)s->objs[0].size);
//     // for (int i = 1; i < NR_OBJS - 2; i++) {
//     //     assert(s->objs[i].size == 0);
//     // }
//     assert(s->objs[NR_OBJS - size_mod_3 - 1].size == NR_OBJS - size_mod_3);
//     assert(s->objs[NR_OBJS - size_mod_3 - 1].prev == E);
//     assert(s->objs[NR_OBJS - size_mod_3 - 1].next == D);

//     slab_free(fragment0, size_mod_3);
//     assert(nr_free_objs(s) == NR_OBJS);
//     // log("free fragment0, this time sentinel: size: %d, prev: %d, next: %d", (int)s->sentinel.size, (int)s->sentinel.prev, (int)s->sentinel.next);

//     assert(nr_free_objs(s) == NR_OBJS);
//     assert(s->sentinel.prev == 0);
//     assert(s->sentinel.next == 0);
//     assert(s->objs[0].size == NR_OBJS);
//     assert(s->objs[0].prev == OBJECT_SENTINEL);
//     assert(s->objs[0].next == OBJECT_SENTINEL);
//     assert(s->objs[NR_OBJS - 1].size == NR_OBJS);
//     assert(s->objs[NR_OBJS - 1].prev == E);
//     PASS("pass slab free test");

//     void* fragment_4 = slab_alloc(4 * OBJECT_SIZE);
//     void* fragment_1 = slab_alloc(OBJECT_SIZE);
//     void* fragment_1_ = slab_alloc(OBJECT_SIZE);
//     void* fragment_2 = slab_alloc(2 * OBJECT_SIZE);

//     slab_free(fragment_1, 1);
//     int idx_1 = OBJECT_IDX(fragment_1);
//     struct slab* s1 = SLAB(fragment_1);
//     assert(s1->objs[idx_1].size == 1);
//     assert(s1->objs[idx_1].prev == 0);
//     assert(s1->objs[idx_1].next == OBJECT_SENTINEL);
//     assert(s1 == SLAB(fragment_4));

//     slab_free(fragment_4, 4);
//     assert(s1->objs[idx_1].size == 4 + 1);
//     assert(s1->objs[idx_1 + 4].size == 4 + 1);
//     assert(s1->objs[idx_1 + 4].prev == E && s1->objs[idx_1 - 4].next == D);

//     int idx_2 = OBJECT_IDX(fragment_2);
//     slab_free(fragment_2, 2);
//     assert(s1->objs[idx_2 + 1].prev == E && s1->objs[idx_2 + 1].next == D);
//     assert(s1->objs[idx_2 + 1].size == NR_OBJS - 1 - 1 - 4);

//     slab_free(fragment_1_, 1);
//     assert(s1->objs[0].prev == OBJECT_SENTINEL && s1->objs[0].next == OBJECT_SENTINEL);
//     assert(s1->objs[0].size == NR_OBJS);
//     // for (int i = 1; i < NR_OBJS - 1; i++)
//     //     assert(s1->objs[i].size == 0 && s1->objs[i].prev == 0 && s1->objs[i].next == 0);
//     assert(s1->objs[NR_OBJS - 1].size == NR_OBJS);

//     PASS("pass slab free one object test");

// }
