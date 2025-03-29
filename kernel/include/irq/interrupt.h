#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

#include <common.h>

typedef uint32 irqret_t;
typedef irqret_t (*irq_handler_t)(uint32, void*);

#define IRQ_HANDLED 0
#define IRQ_SKIP 1
#define IRQ_ERR 2

#define MAX_NR_IRQ 256
#define DEFAULT_HART 0
#define DEFAULT_PRI 2
#define DEFAULT_THRESHOLD 0

int register_irq(uint32 irq, irq_handler_t handler, void* dev);
void free_irq(uint32 irq);
void response_interrupt(void);
void interrupt_init(void);

#endif // __INTERRUPT_H__