#ifndef __CONTEXT_H__
#define __CONTEXT_H__

struct context {
    uint64 ra;
    uint64 sp;
    uint64 s[12];

    uint64 sstatus;
    uint64 epc;
};

#endif // __CONTEXT_H__
