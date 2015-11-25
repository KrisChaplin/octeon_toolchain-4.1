/* Octeon startup code.
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

#include <machine/asm.h>
#include <machine/regdef.h>

#define __ASM_INCLUDE__

LEAF(__start)
	.globl	__cvmx_app_init .text
	.globl	__boot_desc_addr
	.globl	__octeon_argc
	.globl	__octeon_argv

	# Registers $16 - #20 are used by bootloader to pass values to 
	# application. Stack pointer set properly on entry, don't clear. 

	add $2,   $0, $0
	add $3,   $0, $0
	add $4,   $0, $0
	add $5,   $0, $0
	add $6,   $0, $0
	add $7,   $0, $0
	add $8,   $0, $0
	add $9,   $0, $0
	add $10,  $0, $0
	add $11,  $0, $0
	add $12,  $0, $0
	add $13,  $0, $0
	add $14,  $0, $0
	add $15,  $0, $0
	add $21,  $0, $0
	add $22,  $0, $0
	add $23,  $0, $0
	add $24,  $0, $0
	add $25,  $0, $0
	add $26,  $0, $0
	add $27,  $0, $0
	add $28,  $0, $0
	add $30,  $0, $0
	add $31,  $0, $0

	dla     gp,_gp  	# initialise $gp
	move    ra,zero		# ra == 0 to terminate stack trace
	move    fp, sp

	dadd    a0, $16, 0	# copy app descriptor address to arg for function
	dla     t9, __boot_desc_addr
	sd      a0, 0(t9)	# Save copy of boot descriptor address

	# Do basic app init - process argc/argv from bootloader */
	jal     __octeon_app_init
	nop

	dla     t9, _init	# run global constructors
	jal     t9
	nop

	# call __cvmx_app_init, only if symbol is non-zero
	dla     t9, __boot_desc_addr
	ld      a0, 0(t9)
	dla     t9, __cvmx_app_init  # expects boot descriptor address as arg
	beq     t9, zero, 1f
	nop     
	jal	t9
	nop
	b 2f
	nop

1:
	# Install some basic exception handlers in non-simple exec case

	# Get exception base
	mfc0 t9, $15, 1
	li   a0, ~0x3ff
	and  t9, a0  		    # Mask off core number
	dli a0, 0x1000ffff00000000  # branch followed by NOP
	sd a0, 0x80(t9)
	sd a0, 0x100(t9)
	sd a0, 0x180(t9)
	sd a0, 0x200(t9)
	sd a0, 0x280(t9)
	sd a0, 0x300(t9)
	sd a0, 0x380(t9)
	sd a0, 0x400(t9)
	sd a0, 0x480(t9)
	syncw

	# now clear BEV
	mfc0 t9, $12, 0
	li   a0, ~(1 << 22)
	and  t9, a0
	mtc0 t9, $12, 0
	b    2f
	nop

2:
	# load argc/arv as saved by __octeon_app_init
	dla     t9, __octeon_argc
	ld      a0, 0(t9)
	dla     t9, __octeon_argv
	ld      a1, 0(t9)
	dla     t9, main
	jal     t9
	nop
	dla     t9, exit
	jal     t9
	nop

	break
       
END(__start)

