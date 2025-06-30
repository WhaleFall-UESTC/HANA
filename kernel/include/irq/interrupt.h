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

/**
 * Register an interrupt handler
 * @param irq The interrupt number to register
 * @param handler The function to handle the interrupt
 * @param data User data to pass to the handler
 * @return 0 on success, -1 on error
 */
int irq_register(unsigned int irq, irq_handler_t handler, void* data);

/**
 * Unregister an interrupt handler
 * @param irq The interrupt number to unregister
 */
void irq_free(unsigned int irq);

/**
 * Handle an interrupt response.
 * This function is registered in trap.
 */
void irq_response(void);

/**
 * Initialize the interrupt subsystem.
 */
void irq_init(void);

/**
 * Push the current interrupt state off the stack.
 * This is used to disable interrupts temporarily.
 */
void irq_pushoff();

/**
 * Pop the last pushed interrupt state from the stack.
 * This is used to restore the previous interrupt state.
 */
void irq_popoff();

#endif // __INTERRUPT_H__