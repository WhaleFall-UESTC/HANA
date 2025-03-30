struct spinlock {
    uint32 locked;

    // For debugging
    char* name;
    struct cpu *cpu;    // which cpu holding this lock
};

#define UNLOCKED    0
#define LOCKED      1



void    init_lock(struct spinlock* lk, char* name);
void    acquire(struct spinlock* lk);
void    release(struct spinlock* lk);
int     holding(struct spinlock* lk);
void    push_off();
void    pop_off();