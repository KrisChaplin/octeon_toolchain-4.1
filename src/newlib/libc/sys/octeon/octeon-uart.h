/* The functions are copied from Simple Executive Library. 
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

#ifndef __OCTEON_UART_H__
#define __OCTEON_UART_H__

/* Setup a uart for use (0 or 1). Return 0 on success. */ 
int octeon_uart_setup (int uart_index);

/* Write bytes of BUFFER_LEN to the uart from BUFFER. */ 
void octeon_uart_write (int uart_index, const char *buffer, int buffer_len);

/* Write a string (STR) to the uart. */
void octeon_uart_write_string (int uart_index, const char *str);

#endif /* __OCTEON_UART_H__  */
