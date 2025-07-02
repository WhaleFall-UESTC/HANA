#ifndef __EXTIOI_H__
#define __EXTIOI_H__

#include <common.h>

/**
 * External IO Interrupt Controller (EXTIOI) init
 * @param hart Hart ID
 */
void    extioi_init(int hart);

/**
 * Enable an external IO interrupt
 * @param hart Hart ID
 * @param irq Interrupt number
 */
void    extioi_enable_irq(int hart, int irq);

/**
 * Disable an external IO interrupt
 * @param hart Hart ID
 * @param irq Interrupt number
 */
void    extioi_disable_irq(int hart, int irq);

/**
 * Claim an external IO interrupt
 * @param hart Hart ID
 * @return The claimed interrupt number, or 0 if no interrupt is available
 */
uint64  extioi_claim(int hart); 

/**
 * Complete an external IO interrupt
 * @param hart Hart ID
 * @param irq Interrupt number
 */
void    extioi_complete(int hart, int irq);

#endif