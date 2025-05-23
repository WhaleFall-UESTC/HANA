#ifndef __EXTIOI_H__
#define __EXTIOI_H__

void    extioi_init();
uint64  extioi_claim(int hart); 
void    extioi_complete(int irq, int hart);

#endif