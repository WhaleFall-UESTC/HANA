#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

#include <common.h>
#include <arch.h>

typedef uint32 irqret_t;
typedef irqret_t (*irq_handler_t)(uint32, void*);

#define IRQ_HANDLED 0
#define IRQ_SKIP 1
#define IRQ_ERR 2

#define MAX_NR_IRQ 256
#define DEFAULT_HART 0
#define DEFAULT_PRI 2
#define DEFAULT_THRESHOLD 0

int irq_register(uint32, irq_handler_t, void *);
void irq_free(uint32);
void irq_response(void);
void irq_init(void);

void irq_pushoff();
void irq_popoff();

#endif // __INTERRUPT_H__