/* Macros used by the octeon-os.c and octeon-uart.c.
 *
 * Copyright (c) 2004, 2005, 2006 Cavium Networks.
 *
 * The authors hereby grant permission to use, copy, modify, distribute,
 * and license this software and its documentation for any purpose, provided
 * that existing copyright notices are retained in all copies and that this
 * notice is included verbatim in any distributions. No written agreement,
 * license, or royalty fee is required for any of the authorized uses.
 * Modifications to this software may be copyrighted by their authors
 * and need not follow the licensing terms described here, provided that
 * the new terms are clearly indicated on the first page of each file where
 * they apply.
 */

#ifndef __OCTEON_CVMX_H__
#define __OCTEON_CVMX_H__

#include <stdint.h>

#define CVMX_MIO_BOOT_BIST_STAT    (0x80011800280000E8ull)

#define CVMX_CIU_DINT              (0x8001070000000720ull)

#define CVMX_MIO_UARTX_LSR(offset) (0x8001180000000828ull + ((offset) * 1024))
#define CVMX_MIO_UARTX_THR(offset) (0x8001180000000840ull + ((offset) * 1024))

#define CVMX_SPINLOCK_UNLOCKED_VAL  0

#define CVMX_SHARED  __attribute__ ((section (".cvmx_shared")))

#define CVMX_SYNCWS asm volatile ("syncws" : : :"memory")

/* CVMX_MIO_UARTX_LSR(0..1) = MIO UART Line Status Register.  */
typedef union {
  uint64_t u64;     struct
    {
#if __BYTE_ORDER == __BIG_ENDIAN
      uint64_t reserved	: 56;      /* Reserved */
      uint64_t ferr	: 1;       /* Error in Receiver FIFO bit */
      uint64_t temt	: 1;       /* Transmitter Empty bit */
      uint64_t thre	: 1;       /* Transmitter Holding Register Empty bit */
      uint64_t bi  	: 1;       /* Break Interrupt bit */
      uint64_t fe  	: 1;       /* Framing Error bit */
      uint64_t pe  	: 1;       /* Parity Error bit */
      uint64_t oe  	: 1;       /* Overrun Error bit */
      uint64_t dr  	: 1;       /* Data Ready bit */
#else
      uint64_t dr	: 1;
      uint64_t oe	: 1;
      uint64_t pe	: 1;
      uint64_t fe	: 1;
      uint64_t bi	: 1;
      uint64_t thre	: 1;
      uint64_t temt	: 1;
      uint64_t ferr	: 1;
      uint64_t reserved	: 56;
#endif
   } s;
} cvmx_uart_lsr_t;

/* Spinlocks for Octeon. */
typedef struct {
  volatile uint32_t value;
} cvmx_spinlock_t;

/* Gets lock, spins until lock is taken. */
static inline void cvmx_spinlock_lock (cvmx_spinlock_t *lock)
{
  unsigned int tmp;

  asm volatile (
	".set noreorder         \n"
	"1: ll   %[tmp], %[val]  \n"
	"   bnez %[tmp], 1b     \n"
	"   li   %[tmp], 1      \n"
	"   sc   %[tmp], %[val] \n"
	"   beqz %[tmp], 1b     \n"
	"   nop                \n"
	".set reorder           \n"
	: [val] "+m" (lock->value), [tmp] "=&r" (tmp)
	:
	: "memory");
}

/* Release lock. */
static inline void 
cvmx_spinlock_unlock (cvmx_spinlock_t *lock)
{
  CVMX_SYNCWS;
  lock->value = 0;
  CVMX_SYNCWS;
}

/* Return the core number. */
static uint64_t 
cvmx_get_core_num (void)
{
  uint64_t core_num;
  asm ("rdhwr %[rt],$0" : [rt] "=d" (core_num));
  return core_num;
}

extern uint64_t __cvmx_read_csr (uint64_t);
extern __cvmx_write_csr (uint64_t, uint64_t);

#endif /* __OCTEON_CVMX_H__  */
