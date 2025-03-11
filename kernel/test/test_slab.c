#include <defs.h>
#include <buddy.h>
#include <slab.h>
#include <debug.h>

extern uint partial_len;
extern struct slab *current, *partial;

void 
test_slab() 
{
    assert((ROUNDUP(NR_OBJS * OBJECT_SIZE, OBJECT_SIZE) >> OBJECT_SHIFT)== NR_OBJS);

    // slab_alloc and slab_free
    void* tmp[MIN_PARTIAL + 1];
    struct slab* origin[MIN_PARTIAL + 1];
    origin[0] = current;
    origin[1] = partial;
    for (int i = 2; i <= MIN_PARTIAL; i++) {
        origin[i] = get_slab_next(origin[i - 1]);
    }
    for (int i = 0; i <= MIN_PARTIAL; i++) {
        log("alloc slab %d", i);
        tmp[i] = slab_alloc(NR_OBJS * OBJECT_SIZE);
        assert(current == origin[i]);
    }

    uint8 size_div_3 = NR_OBJS / 3;
    uint8 size_mod_3 = NR_OBJS - 3 * size_div_3;
    void* fragment0 = slab_alloc(size_mod_3 * OBJECT_SIZE);
    struct slab* s = SLAB(fragment0);
    assert(s->sentinel.prev == 0);
    assert(s->sentinel.next == 0);
    assert(s->objs[0].prev == OBJECT_SENTINEL);
    assert(s->objs[0].next == OBJECT_SENTINEL);
    assert(s->objs[0].size = NR_OBJS - size_mod_3);
    log("Alloc fragment0, this time sentinel: size: %d, prev: %d, next: %d", s->sentinel.size, s->sentinel.prev, s->sentinel.next);
    void* fragment1 = slab_alloc(size_div_3 * OBJECT_SIZE);
    log("Alloc fragment1, this time sentinel: size: %d, prev: %d, next: %d", s->sentinel.size, s->sentinel.prev, s->sentinel.next);
    // slab will call buddy_alloc
    assert(partial_len == MIN_PARTIAL + 1);
    void* fragment2 = slab_alloc(size_div_3 * OBJECT_SIZE);
    log("Alloc fragment2, this time sentinel: size: %d, prev: %d, next: %d", s->sentinel.size, s->sentinel.prev, s->sentinel.next);
    void* fragment3 = slab_alloc(size_div_3 * OBJECT_SIZE);
    log("Alloc all slab, this time sentinel: size: %d, prev: %d, next: %d", s->sentinel.size, s->sentinel.prev, s->sentinel.next);
    assert(s->sentinel.prev == OBJECT_SENTINEL);
    assert(s->sentinel.next == OBJECT_SENTINEL);

    for (int i = 0; i <= MIN_PARTIAL; i++)
        if ((uint64)tmp[i] != (uint64)s) {
            slab_free(tmp[1], NR_OBJS);
            break;
        }
    assert(partial_len == MIN_PARTIAL);
    PASS("pass partial len test");

    assert(nr_free_objs(s) == 0);

    for (int i = 0; i < NR_OBJS; i++) {
        assert(s->objs[i].size == 0);
        assert(s->objs[i].prev == 0);
        assert(s->objs[i].next == 0);
    }

    PASS("pass slab alloc test");

    log("before free, this time sentinel: size: %d, prev: %d, next: %d", s->sentinel.size, s->sentinel.prev, s->sentinel.next);
    slab_free(fragment1, size_div_3);
    log("free fragment1, this time sentinel: size: %d, prev: %d, next: %d", s->sentinel.size, s->sentinel.prev, s->sentinel.next);
    assert(s->sentinel.prev == 2 * size_div_3);
    slab_free(fragment3, size_div_3);
    log("free fragment3, this time sentinel: size: %d, prev: %d, next: %d", s->sentinel.size, s->sentinel.prev, s->sentinel.next);
    assert(s->sentinel.prev == 0);
    assert(s->sentinel.next == 2 * size_div_3);
    assert(nr_free_objs(s) == 2 * size_div_3);

    slab_free(fragment2, size_div_3);
    log("free fragment2, this time sentinel: size: %d, prev: %d, next: %d", s->sentinel.size, s->sentinel.prev, s->sentinel.next);
    assert(s->sentinel.prev == 0);
    assert(s->sentinel.next == 0);
    assert(nr_free_objs(s) == NR_OBJS - size_mod_3);

    slab_free(fragment0, size_mod_3);
    assert(nr_free_objs(s) == NR_OBJS);
    log("free fragment1, this time sentinel: size: %d, prev: %d, next: %d", s->sentinel.size, s->sentinel.prev, s->sentinel.next);

    PASS("pass slab free test");
}
