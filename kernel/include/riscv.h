#include <common.h>

#define PTE_V  (1L << 0)
#define PTE_R  (1L << 1)
#define PTE_W  (1L << 2)
#define PTE_X  (1L << 3)
#define PTE_U  (1L << 4)
#define PTE_SHARED (1L << 5)
#define PTE_EXECUTE_ONLY (1L << 6)
#define PTE_DONT_ALLOCATE (1L << 7)
#define PTE_READ_ONLY (1L << 8)

#define PA2PTE(pa) ((((uint64)pa) >> 12) << 10)
#define PTE2PA(pte) (((pte) >> 10) << 12)
#define PTE_FLAGS(pte) ((pte) & 0x3FF)

// #define MAXVA (1L << (9 + 9 + 9 + 12 - 1))
#define MAXVA 0x4000000000L

typedef uint64 pte_t;
typedef uint64* pagetable_t;

static inline void 
sfence_vma()
{
  asm volatile ("sfence.vma");
}


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

