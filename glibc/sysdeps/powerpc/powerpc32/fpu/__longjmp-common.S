/* longjmp for PowerPC.
   Copyright (C) 1995-99, 2000, 2003-2005, 2006 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 1 Franklin Street, Fifth Floor, Boston MA
   02110-1301 USA.  */

#include <sysdep.h>
#define _ASM
#ifdef __NO_VMX__
# include <novmxsetjmp.h>
#else
# include <jmpbuf-offsets.h>
#endif
#include <bp-sym.h>
#include <bp-asm.h>

	.machine	"altivec"
ENTRY (BP_SYM (__longjmp))
	CHECK_BOUNDS_BOTH_WIDE_LIT (r3, r8, r9, JB_SIZE)
#ifndef __NO_VMX__
# ifdef PIC
	mflr    r6
	cfi_register (lr,r6)
#  ifdef HAVE_ASM_PPC_REL16
	bcl	20,31,1f
1:	mflr	r5
	addis	r5,r5,_GLOBAL_OFFSET_TABLE_-1b@ha
	addi	r5,r5,_GLOBAL_OFFSET_TABLE_-1b@l
#  else
	bl      _GLOBAL_OFFSET_TABLE_@local-4
	mflr    r5
#  endif
#  ifdef SHARED
	lwz     r5,_rtld_global_ro@got(r5)
	mtlr    r6
	cfi_same_value (lr)
	lwz     r5,RTLD_GLOBAL_RO_DL_HWCAP_OFFSET(r5)
#  else
	lwz     r5,_dl_hwcap@got(r5)
	mtlr    r6
	cfi_same_value (lr)
	lwz     r5,0(r5)
#  endif
# else
	lis	r5,_dl_hwcap@ha
	lwz     r5,_dl_hwcap@l(r5)
# endif
	andis.	r5,r5,(PPC_FEATURE_HAS_ALTIVEC >> 16)
	beq	L(no_vmx)
	la	r5,((JB_VRS)*4)(3)
	andi.	r6,r5,0xf
	lwz	r0,((JB_VRSAVE)*4)(3)
	mtspr	VRSAVE,r0
	beq+	aligned_restore_vmx
	addi    r6,r5,16
	lvsl	v0,0,r5
	lvx	v1,0,r5
	addi    r5,r5,32
	lvx	v21,0,r6
	vperm   v20,v1,v21,v0
# define load_misaligned_vmx_lo_loaded(loadvr,lovr,shiftvr,loadgpr,addgpr) \
	addi    addgpr,addgpr,32; \
	lvx	lovr,0,loadgpr; \
	vperm   loadvr,loadvr,lovr,shiftvr;
	load_misaligned_vmx_lo_loaded(v21,v22,v0,r5,r6)
	load_misaligned_vmx_lo_loaded(v22,v23,v0,r6,r5)
	load_misaligned_vmx_lo_loaded(v23,v24,v0,r5,r6)
	load_misaligned_vmx_lo_loaded(v24,v25,v0,r6,r5)
	load_misaligned_vmx_lo_loaded(v25,v26,v0,r5,r6)
	load_misaligned_vmx_lo_loaded(v26,v27,v0,r6,r5)
	load_misaligned_vmx_lo_loaded(v27,v28,v0,r5,r6)
	load_misaligned_vmx_lo_loaded(v28,v29,v0,r6,r5)
	load_misaligned_vmx_lo_loaded(v29,v30,v0,r5,r6)
	load_misaligned_vmx_lo_loaded(v30,v31,v0,r6,r5)
	lvx	v1,0,r5
	vperm   v31,v31,v1,v0
	b       L(no_vmx)
aligned_restore_vmx:
	addi	r6,r5,16
	lvx	v20,0,r5
	addi	r5,r5,32
	lvx	v21,0,r6
	addi	r6,r6,32
	lvx	v22,0,r5
	addi	r5,r5,32
	lvx	v23,0,r6
	addi	r6,r6,32
	lvx	v24,0,r5
	addi	r5,r5,32
	lvx	v25,0,r6
	addi	r6,r6,32
	lvx	v26,0,r5
	addi	r5,r5,32
	lvx	v27,0,r6
	addi	r6,r6,32
	lvx	v28,0,r5
	addi	r5,r5,32
	lvx	v29,0,r6
	addi	r6,r6,32
	lvx	v30,0,r5
	lvx	v31,0,r6
L(no_vmx):
#endif
	lwz r1,(JB_GPR1*4)(r3)
	lwz r0,(JB_LR*4)(r3)
	lwz r14,((JB_GPRS+0)*4)(r3)
	lfd fp14,((JB_FPRS+0*2)*4)(r3)
	lwz r15,((JB_GPRS+1)*4)(r3)
	lfd fp15,((JB_FPRS+1*2)*4)(r3)
	lwz r16,((JB_GPRS+2)*4)(r3)
	lfd fp16,((JB_FPRS+2*2)*4)(r3)
	lwz r17,((JB_GPRS+3)*4)(r3)
	lfd fp17,((JB_FPRS+3*2)*4)(r3)
	lwz r18,((JB_GPRS+4)*4)(r3)
	lfd fp18,((JB_FPRS+4*2)*4)(r3)
	lwz r19,((JB_GPRS+5)*4)(r3)
	lfd fp19,((JB_FPRS+5*2)*4)(r3)
	lwz r20,((JB_GPRS+6)*4)(r3)
	lfd fp20,((JB_FPRS+6*2)*4)(r3)
#ifdef PTR_DEMANGLE
	PTR_DEMANGLE (r0, r25)
	PTR_DEMANGLE2 (r1, r25)
#endif
	mtlr r0
	lwz r21,((JB_GPRS+7)*4)(r3)
	lfd fp21,((JB_FPRS+7*2)*4)(r3)
	lwz r22,((JB_GPRS+8)*4)(r3)
	lfd fp22,((JB_FPRS+8*2)*4)(r3)
	lwz r0,(JB_CR*4)(r3)
	lwz r23,((JB_GPRS+9)*4)(r3)
	lfd fp23,((JB_FPRS+9*2)*4)(r3)
	lwz r24,((JB_GPRS+10)*4)(r3)
	lfd fp24,((JB_FPRS+10*2)*4)(r3)
	lwz r25,((JB_GPRS+11)*4)(r3)
	lfd fp25,((JB_FPRS+11*2)*4)(r3)
	mtcrf 0xFF,r0
	lwz r26,((JB_GPRS+12)*4)(r3)
	lfd fp26,((JB_FPRS+12*2)*4)(r3)
	lwz r27,((JB_GPRS+13)*4)(r3)
	lfd fp27,((JB_FPRS+13*2)*4)(r3)
	lwz r28,((JB_GPRS+14)*4)(r3)
	lfd fp28,((JB_FPRS+14*2)*4)(r3)
	lwz r29,((JB_GPRS+15)*4)(r3)
	lfd fp29,((JB_FPRS+15*2)*4)(r3)
	lwz r30,((JB_GPRS+16)*4)(r3)
	lfd fp30,((JB_FPRS+16*2)*4)(r3)
	lwz r31,((JB_GPRS+17)*4)(r3)
	lfd fp31,((JB_FPRS+17*2)*4)(r3)
	mr r3,r4
	blr
END (BP_SYM (__longjmp))
