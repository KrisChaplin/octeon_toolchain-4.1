/* Functions to write to uart on a print command.
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

#include "octeon-uart.h"
#include "octeon-cvmx.h"

CVMX_SHARED static cvmx_spinlock_t octeon_uart_lock = {CVMX_SPINLOCK_UNLOCKED_VAL};

/* Put a single byte to uart port. UART_INDEX is the uart to write to (0/1).
   CH contains the byte to write.  */
static inline void 
octeon_uart_write_byte (int uart_index, uint8_t ch)
{
  cvmx_uart_lsr_t lsrval;

  /* Spin until there is room */
  do
    {
      lsrval.u64 = __cvmx_read_csr (CVMX_MIO_UARTX_LSR (uart_index));
    } while (lsrval.s.thre == 0);

  /* Write the byte */
  __cvmx_write_csr (CVMX_MIO_UARTX_THR (uart_index), ch);
}

/* Write out the PP banner without using any C library functions to uart
   specified by UART_INDEX.  */
static void 
octeon_uart_write_banner (int uart_index)
{
  const uint64_t coreid = cvmx_get_core_num ();

  octeon_uart_write_byte (uart_index, 'P');
  octeon_uart_write_byte (uart_index, 'P');
  if (coreid < 10)
    octeon_uart_write_byte (uart_index, coreid + '0');
  else
    {
      octeon_uart_write_byte (uart_index, '1');
      octeon_uart_write_byte (uart_index, coreid - 10 + '0');
    }
  octeon_uart_write_byte (uart_index, ':');
  octeon_uart_write_byte (uart_index, '~');
  octeon_uart_write_byte (uart_index, 'C');
  octeon_uart_write_byte (uart_index, 'O');
  octeon_uart_write_byte (uart_index, 'N');
  octeon_uart_write_byte (uart_index, 'S');
  octeon_uart_write_byte (uart_index, 'O');
  octeon_uart_write_byte (uart_index, 'L');
  octeon_uart_write_byte (uart_index, 'E');
  octeon_uart_write_byte (uart_index, '-');
  octeon_uart_write_byte (uart_index, '>');
  octeon_uart_write_byte (uart_index, ' ');
}

/* Write bytes to the uart specified by UART_INDEX into BUFFER of bytes 
   in BUFFER_LEN.  */
void 
octeon_uart_write (int uart_index, const char *buffer, int buffer_len)
{
  cvmx_spinlock_lock (&octeon_uart_lock);

  octeon_uart_write_banner (uart_index);

  /* Just loop writing one byte at a time */
  while (buffer_len)
    {
      if (*buffer == '\n')
        octeon_uart_write_byte (uart_index, '\r');
      octeon_uart_write_byte (uart_index, *buffer);
      buffer++;
      buffer_len--;
    }

  cvmx_spinlock_unlock (&octeon_uart_lock);
}

/* Write a string STR to the uart UART_INDEX.  */
void 
octeon_uart_write_string (int uart_index, const char *str)
{
  cvmx_spinlock_lock (&octeon_uart_lock);

  octeon_uart_write_banner (uart_index);

  /* Just loop writing one byte at a time */
  while (*str)
    {
      if (*str == '\n')
	octeon_uart_write_byte (uart_index, '\r');
      octeon_uart_write_byte (uart_index, *str);
      str++;
    }

  cvmx_spinlock_unlock (&octeon_uart_lock);
}

/* Read from CSR register that is at address CSR_ADDR. */
uint64_t 
__cvmx_read_csr (uint64_t csr_addr)
{
  uint32_t csr_addrh = csr_addr >> 32;
  uint32_t csr_addrl = csr_addr;
  uint32_t valh;
  uint32_t vall;
  asm volatile (
	"dsll   %[valh], %[csrh], 32\n"
	"dsll   %[vall], %[csrl], 32\n"
	"dsrl   %[vall], %[vall], 32\n"
	"or     %[valh], %[valh], %[vall]\n"
	"ld     %[vall], 0(%[valh])\n"
	"dsrl   %[valh], %[vall], 32\n"
	"dsll   %[vall], %[vall], 32\n"
	"dsrl   %[vall], %[vall], 32\n"
	: [valh] "=&r" (valh), [vall] "=&r" (vall)
	: [csrh] "r" (csr_addrh), [csrl] "r" (csr_addrl)
    );
  return (((uint64_t)(valh << 32)) | vall);
}

/* Write value (VAL) into CSR register that is at address CSR_ADDR. */
__cvmx_write_csr (uint64_t csr_addr, uint64_t val)
{
  uint32_t csr_addrh = csr_addr >> 32;
  uint32_t csr_addrl = csr_addr;
  uint32_t valh = (uint64_t)(val >> 32);
  uint32_t vall = val;
  uint32_t tmp1;
  uint32_t tmp2;
  uint32_t tmp3;
  asm volatile (
	"dsll   %[tmp1], %[valh], 32\n"
	"dsll   %[tmp2], %[csrh], 32\n"
	"dsll   %[tmp3], %[vall], 32\n"
	"dsrl   %[tmp3], %[tmp3], 32\n"
	"or     %[tmp1], %[tmp1], %[tmp3]\n"
	"dsll   %[tmp3], %[csrl], 32\n"
	"dsrl   %[tmp3], %[tmp3], 32\n"
	"or     %[tmp2], %[tmp2], %[tmp3]\n"
	"sd     %[tmp1], 0(%[tmp2])\n"
	: [tmp1] "=&r" (tmp1), [tmp2] "=&r" (tmp2), [tmp3] "=&r" (tmp3)
	: [valh] "r" (valh), [vall] "r" (vall),
	[csrh] "r" (csr_addrh), [csrl] "r" (csr_addrl));
}
