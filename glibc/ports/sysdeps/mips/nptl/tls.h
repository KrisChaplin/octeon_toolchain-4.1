/* Definition for thread-local data handling.  NPTL/MIPS version.
   Copyright (C) 2005 Free Software Foundation, Inc.
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
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#ifndef _TLS_H
#define _TLS_H	1

#include <dl-sysdep.h>

#ifndef __ASSEMBLER__
# include <stdbool.h>
# include <stddef.h>
# include <stdint.h>

/* Type for the dtv.  */
typedef union dtv
{
  size_t counter;
  struct
  {
    void *val;
    bool is_static;
  } pointer;
} dtv_t;

/* Note: rd must be $v1 to be ABI-conformant.  */
# ifndef __OCTEON__
# define READ_THREAD_POINTER() \
    ({ void *__result;							      \
       asm volatile (".set\tpush\n\t.set\tmips32r2\n\t"			      \
		     "rdhwr\t%0, $29\n\t.set\tpop" : "=v" (__result));	      \
       __result; })
# else /* OCTEON */
/* The Kernel stores the value of "rdhwr v1,$29" in k0 ($26) register. And
   it is the Kernel's responsibility to always have the correct value in
   k0.  Replacing rdhwr instruction with k0, as this instruction needs to
   be emulated by the Kernel.  */
# define READ_THREAD_POINTER() ( { register void *__result asm ("$26"); __result; } )
# endif /* OCTEON */

#else /* __ASSEMBLER__ */
# include <tcb-offsets.h>

# ifndef __OCTEON__
# define READ_THREAD_POINTER(rd) \
	.set	push;							      \
	.set	mips32r2;						      \
	rdhwr	rd, $29;						      \
	.set	pop
# else /* OCTEON */
# define READ_THREAD_POINTER(rd) move rd, $26
# endif /* OCTEON */
#endif /* __ASSEMBLER__ */


/* We require TLS support in the tools.  */
#ifndef HAVE_TLS_SUPPORT
# error "TLS support is required."
#endif

/* Signal that TLS support is available.  */
#define USE_TLS	1

#ifndef __ASSEMBLER__

/* Get system call information.  */
# include <sysdep.h>

/* The TP points to the start of the thread blocks.  */
# define TLS_DTV_AT_TP	1

/* Get the thread descriptor definition.  */
# include <nptl/descr.h>

typedef struct
{
  dtv_t *dtv;
  void *private;
} tcbhead_t;

/* This is the size of the initial TCB.  Because our TCB is before the thread
   pointer, we don't need this.  */
# define TLS_INIT_TCB_SIZE	0

/* Alignment requirements for the initial TCB.  */
# define TLS_INIT_TCB_ALIGN	__alignof__ (struct pthread)

/* This is the size of the TCB.  Because our TCB is before the thread
   pointer, we don't need this.  */
# define TLS_TCB_SIZE		0

/* Alignment requirements for the TCB.  */
# define TLS_TCB_ALIGN		__alignof__ (struct pthread)

/* This is the size we need before TCB - actually, it includes the TCB.  */
# define TLS_PRE_TCB_SIZE \
  (sizeof (struct pthread)						      \
   + ((sizeof (tcbhead_t) + TLS_TCB_ALIGN - 1) & ~(TLS_TCB_ALIGN - 1)))

/* The thread pointer (in hardware register $29) points to the end of
   the TCB + 0x7000, as for PowerPC.  The pthread_descr structure is
   immediately in front of the TCB.  */
# define TLS_TCB_OFFSET	0x7000

/* Install the dtv pointer.  The pointer passed is to the element with
   index -1 which contain the length.  */
# define INSTALL_DTV(tcbp, dtvp) \
  (((tcbhead_t *) (tcbp))[-1].dtv = (dtvp) + 1)

/* Install new dtv for current thread.  */
# define INSTALL_NEW_DTV(dtv) \
  (THREAD_DTV() = (dtv))

/* Return dtv of given thread descriptor.  */
# define GET_DTV(tcbp) \
  (((tcbhead_t *) (tcbp))[-1].dtv)

/* Code to initially initialize the thread pointer.  This might need
   special attention since 'errno' is not yet available and if the
   operation can cause a failure 'errno' must not be touched.  */
# define TLS_INIT_TP(tcbp, secondcall) \
  ({ INTERNAL_SYSCALL_DECL (err);					\
     long result_var;							\
     result_var = INTERNAL_SYSCALL (set_thread_area, err, 1,		\
				    (char *) (tcbp) + TLS_TCB_OFFSET);	\
     INTERNAL_SYSCALL_ERROR_P (result_var, err)				\
       ? "unknown error" : NULL; })

/* Return the address of the dtv for the current thread.  */
# define THREAD_DTV() \
  (((tcbhead_t *) (READ_THREAD_POINTER () - TLS_TCB_OFFSET))[-1].dtv)

/* Return the thread descriptor for the current thread.  */
# define THREAD_SELF \
 ((struct pthread *) (READ_THREAD_POINTER ()			     \
		      - TLS_TCB_OFFSET - TLS_PRE_TCB_SIZE))

/* Magic for libthread_db to know how to do THREAD_SELF.  */
# define DB_THREAD_SELF \
  CONST_THREAD_AREA (32, TLS_TCB_OFFSET + TLS_PRE_TCB_SIZE)

/* Access to data in the thread descriptor is easy.  */
# define THREAD_GETMEM(descr, member) \
  descr->member
# define THREAD_GETMEM_NC(descr, member, idx) \
  descr->member[idx]
# define THREAD_SETMEM(descr, member, value) \
  descr->member = (value)
# define THREAD_SETMEM_NC(descr, member, idx, value) \
  descr->member[idx] = (value)

/* l_tls_offset == 0 is perfectly valid on MIPS, so we have to use some
   different value to mean unset l_tls_offset.  */
# define NO_TLS_OFFSET		-1

#endif /* __ASSEMBLER__ */

#endif	/* tls.h */
