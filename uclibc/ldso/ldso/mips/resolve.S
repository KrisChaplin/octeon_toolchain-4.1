/*
 * Linux dynamic resolving code for MIPS. Fixes up the GOT entry as
 * indicated in register t8 and jumps to the resolved address. Shamelessly
 * ripped from 'sysdeps/mips/dl-machine.h' in glibc-2.2.5.
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License.  See the file "COPYING.LIB" in the main directory of this
 * archive for more details.
 *
 * Copyright (C) 1996-2001 Kazumoto Kojima <kkojima@info.kanagawa-u.ac.jp>
 * Copyright (C) 2002 Steven J. Hill <sjhill@realitydiluted.com>
 *
 */

.text
.align	2
.globl	_dl_runtime_resolve
.type	_dl_runtime_resolve,@function
.ent	_dl_runtime_resolve
_dl_runtime_resolve:
	.frame	$29, 40, $31
	.set noreorder

	# Save GP.
	move	$3, $28

#if (defined _ABIN32 && _MIPS_SIM == _ABIN32)
	# n32 has 8 8-byte arg regs.
	subu	$29, 88
#else
	# Save arguments and sp value on stack.
	subu	$29, 40
#endif
	# Modify t9 ($25) so as to point .cpload instruction.
	addiu	$25, 12

	# Compute GP.
	.set noreorder
	/* Use .cpsetup with n32.  But only use for 4 bytes of storage.  */
	.cpsetup $25, $2, _dl_runtime_resolve+12
#if (defined _ABIN32 && _MIPS_SIM == _ABIN32)
	sw	$2, 80($sp)
#endif
	.cpload $25
	.set reorder

	# Save slot call pc.
	move	$2, $31
	.cprestore 32

	# Store function arguments from registers to stack
#if (defined _ABIN32 && _MIPS_SIM == _ABIN32)
	sw	$15, 84($29)
	sd	$4, 16($29)
	sd	$5, 24($29)
	sd	$6, 32($29)
	sd	$7, 40($29)
	sd	$8, 48($29)
	sd	$9, 56($29)
	sd	$10, 64($29)
	sd	$11, 72($29)
#else
	sw	$15, 36($29)
	sw	$4, 16($29)
	sw	$5, 20($29)
	sw	$6, 24($29)
	sw	$7, 28($29)
#endif
	# Setup functions args and call __dl_runtime_resolve
	move	$4, $24
	move	$5, $3
	jal	__dl_runtime_resolve

	# Restore function arguments from stack to registers
#if (defined _ABIN32 && _MIPS_SIM == _ABIN32)
	lw	$31, 84($29)
	ld	$4, 16($29)
	ld	$5, 24($29)
	ld	$6, 32($29)
	ld	$7, 40($29)
	ld	$8, 48($29)
	ld	$9, 56($29)
	ld	$10, 64($29)
	ld	$11, 72($29)
#else
	lw	$31, 36($29)
	lw	$4, 16($29)
	lw	$5, 20($29)
	lw	$6, 24($29)
	lw	$7, 28($29)
#endif
	# cpreturn by with 4 bytes of storage.
#if (defined _ABIN32 && _MIPS_SIM == _ABIN32)
	lw	$gp, 80($sp)
#endif

	# Do a tail call to the original function
#if (defined _ABIN32 && _MIPS_SIM == _ABIN32)
	addiu	$29, 88
#else
	addiu	$29, 40
#endif
	move	$25, $2
	jr	$25
.end	_dl_runtime_resolve
.previous

