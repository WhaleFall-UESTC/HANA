#ifndef __EXTIOI_H__
#define __EXTIOI_H__

#include <common.h>

void    extioi_init(int hart);
void extioi_enable_irq(int hart, int irq);
void extioi_disable_irq(int hart, int irq);
uint64  extioi_claim(int hart); 
void    extioi_complete(int hart, int irq);

#endif