#include <common.h>
#include <klib.h>
#include <mm/slab.h>
#include <mm/buddy.h>
#include <debug.h>

struct slab* slab_rt = NULL;
uint16 slab_node_cnt = 0;

static inline void
heap_set_son(struct slab* x, struct slab* y, uint8 which) {
    (which ? x->rs : x->ls) = y;
    if (y != NULLPTR)
        y->fa = x;
}

// swap x and x->fa
static inline void
heap_up_node(struct slab* x) {
    assert(x->fa != NULLPTR);
    struct slab* fa = x->fa;
    uint8 x_which = fa->rs == x;
    (x_which ? x->rs : x->ls);
    heap_set_son(fa, x_which ? x->rs : x->ls, x_which);
    struct slab* x_op_s = !x_which ? x->rs : x->ls;
    heap_set_son(x, !x_which ? fa->rs : fa->ls, !x_which);
    heap_set_son(fa, x_op_s, !x_which);
    struct slab* gfa = fa->fa;
    uint8 fa_which = (gfa != NULLPTR && gfa->rs == fa);
    heap_set_son(x, fa, x_which);
    if (gfa != NULLPTR)
        heap_set_son(gfa, x, fa_which);
}

static inline void
heap_push(struct slab* x) {
    uint16 j = 1;
    slab_node_cnt += 1;
    while (j <= slab_node_cnt)
        j <<= 1;
    j >>= 1;
    struct slab* now = slab_rt;
    while (j != 1)
        now = (j & slab_node_cnt == 0 ? now->ls : now->rs);
    (1 & slab_node_cnt ? now->ls : now->rs) = x;
    x->fa = now;
    while (x->fa != NULLPTR) {
        if (x->mxlen > x->fa->mxlen)
            heap_up_node(x);
        else break;
    }
}

static inline struct slab*
head_pop() {
    struct slab* ret = slab_rt;
    
}

static inline struct slab*
new_slab() {
    struct slab* ret = buddy_alloc(PGSIZE);

    ret->ls = ret->rs = ret->fa = NULLPTR;
    ret->mxlen = NR_OBJS;

    ret->rt = ret->tp = NR_OBJS - 1;
    for (int i = 0; i < NR_OBJS - 1; i++)
        ret->nd_stack[i] = i;
    
    ret->objs_info[ret->rt] = (struct object_entry){
        NULLPTR, NULLPTR,
        0, NR_OBJS, NR_OBJS,
        1
    };
}

static inline void
delete_slab(struct slab* x) {
    buddy_free((void*)x, 0);
}

void
slab_init() {
    slab_rt = new_slab();
    slab_node_cnt = 1;
    for (int i = 1; i < MIN_NODE_CNT; i++)
        heap_push(new_slab());
}