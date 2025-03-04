#include "common.h"

// CSR_MSTATUS
#define MSTATUS_SIE = (1L << 1)
#define MSTATUS_MIE = (1L << 3)
#define MSTATUS_SPIE = (1L << 5)
#define MSTATUS_MPIE = (1L << 7)
#define MSTATUS_SPP = (1L << 8)
#define MSTATUS_MPP = (3L << 11)
#define MSTATUS_MPP_M = (3L << 11)
#define MSTATUS_MPP_S = (1L << 11)
#define MSTATUS_MPP_U = (0L << 11)
#define MSTATUS_FS = (3L << 13)
#define MSTATUS_XS = (3L << 15)
#define MSTATUS_MPRV = (1L << 17)
#define MSTATUS_SUM = (1L << 18)
#define MSTATUS_MXR = (1L << 19)
#define MSTATUS_TVM = (1L << 20)
#define MSTATUS_TW = (1L << 21)
#define MSTATUS_TSR = (1L << 22)
#define MSTATUS_SD (1L << (XLEN - 1))

static inline uint64
r_mstatus()
{
  uint64 x;
  asm volatile("csrr %0, mstatus" : "=r" (x) );
  return x;
}

static inline void
w_mstatus(uint64 x)
{
  asm volatile("csrw mstatus, %0" : : "r" (x));
}


// CSR_MIP



// CSR_MIE



// CSR_MCAUSE



// CSR_MTVEC



// CSR_MTVAL



// CSR_MEPC



// CSR_MSCRATCH

