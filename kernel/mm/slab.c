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
    while (j != 1) {
        now = (j & slab_node_cnt == 0 ? now->ls : now->rs);
        j >>= 1;
    }
    (1 & slab_node_cnt ? now->ls : now->rs) = x;
    x->fa = now;
    while (x->fa != NULLPTR) {
        if (x->mxlen > x->fa->mxlen)
            heap_up_node(x);
        else break;
    }
}

static inline struct slab*
heap_pop() {
    struct slab* ret = slab_rt;
    uint16 j = 1;
    while (j <= slab_node_cnt)
        j <<= 1;
    j >>= 1;
    struct slab* now = slab_rt;
    while (j) {
        now = (j & slab_node_cnt == 0 ? now->ls : now->rs);
        j >>= 1;
    }
    heap_set_son(now, ret->ls, 0);
    heap_set_son(now, ret->rs, 1);
    ret->ls = ret->rs = now->fa = NULLPTR;
    (now->fa->rs == now ? now->fa->rs : now->fa->ls) = NULLPTR;
    slab_node_cnt -= 1;
    while (1) {
        if (now->ls == NULLPTR)
            break;
        struct slab* nxt = now->ls;
        if (now->rs != NULLPTR && now->ls->mxlen < now->rs->mxlen)
            nxt = now->rs;
        if (nxt->mxlen > now->mxlen)
            heap_up_node(nxt);
        else break;
    }
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

static inline void
AVL_delete(struct slab* x, uint8 idx) {
    // TODO
}

static inline void*
AVL_slab_alloc(struct slab* x, uint8 idx, uint8 nr_objs) {
    void* ret;
    // TODO
    if (nr_objs == x->objs_info[idx].len) {
        ret = &x->objects[idx];
        AVL_delete(x, idx);
    } else {
        
    }
}

void*
slab_alloc(uint64 sz) {
    assert(0 < sz && sz <= OBJECT_SIZE * NR_OBJS);
    uint8 nr_objs = ROUNDUP(sz, OBJECT_SIZE) >> OBJECT_SHIFT;
    if (nr_objs <= slab_rt->mxlen) {
        struct slab* ret = heap_pop();
        void* ret_addr;
        static int idx_stack[NR_OBJS], tp;
        idx_stack[tp = 0] = ret->rt;
        struct object_entry* now = &ret->objs_info[ret->rt];
        tp += 1;
        // TODO
        while (1) {
            if (now->ls != NULLPTR && ret->objs_info[ret->ls].mxlen >= nr_objs) {
                idx_stack[tp++] = now->ls;
                now = &ret->objs_info[ret->ls];
            } else if (now->rs != NULLPTR && ret->objs_info[ret->rs].mxlen >= nr_objs) {
                idx_stack[tp++] = now->rs;
                now = &ret->objs_info[ret->rs];
            } else break;
        }
        // TODO
        // ret->mxlen = (ret->rt != NULLPTR) ? ret->objs_info[ret->rt].mxlen: 0;
        heap_push(ret);
        return ret_addr;
    } else {
        struct slab* ret = new_slab();
        AVL_slab_alloc(ret, ret->rt, nr_objs);
        heap_push(ret);
        return ret->objects;   
    }
}