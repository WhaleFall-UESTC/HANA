#ifndef __ATOMIC_H__
#define __ATOMIC_H__

/**
 * Use to declear type atomic_##valtype
 * for future defination
 * ONLY support intergers
 */
#define atomic_declear(valtype) \
    typedef struct              \
    {                           \
        valtype val;            \
        spinlock_t lock;        \
    } atomic_##valtype##_t

/**
 * Use to define atomic type in-time
 */
#define atomic_define(valtype, ...) \
    struct                          \
    {                               \
        valtype val;                \
        spinlock_t lock;            \
    } __VA_ARGS__

#define atomic_init(atomic_var, init_val)                                   \
    do                                                                      \
    {                                                                       \
        (atomic_var)->val = init_val;                                       \
        spinlock_init(&(atomic_var)->lock, macro_param_to_str(atomic_var)); \
    } while (0)

#define atomic_inc(atomic_var)                 \
    ({                                         \
        typeof((atomic_var)->val) __ret;       \
        spinlock_acquire(&(atomic_var)->lock); \
        __ret = ++(atomic_var)->val;           \
        spinlock_release(&(atomic_var)->lock); \
        __ret;                                 \
    })

#define atomic_dec(atomic_var)                 \
    ({                                         \
        typeof((atomic_var)->val) __ret;       \
        spinlock_acquire(&(atomic_var)->lock); \
        __ret = --(atomic_var)->val;           \
        spinlock_release(&(atomic_var)->lock); \
        __ret;                                 \
    })

#define atomic_get(atomic_var)                 \
    ({                                         \
        typeof((atomic_var)->val) __ret;       \
        spinlock_acquire(&(atomic_var)->lock); \
        __ret = (atomic_var)->val;             \
        spinlock_release(&(atomic_var)->lock); \
        __ret;                                 \
    })

#define atomic_set(atomic_var, val)            \
    do                                         \
    {                                          \
        spinlock_acquire(&(atomic_var)->lock); \
        (atomic_var)->val = val;               \
        spinlock_release(&(atomic_var)->lock); \
    } while (0)

#endif // __ATOMIC_H__