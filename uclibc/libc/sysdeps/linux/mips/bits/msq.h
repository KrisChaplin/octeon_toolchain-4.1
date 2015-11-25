/* Copyright (C) 2002 Free Software Foundation, Inc.
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

#ifndef _SYS_MSG_H
# error "Never use <bits/msq.h> directly; include <sys/msg.h> instead."
#endif

#include <bits/types.h>
#include <bits/wordsize.h>

/* Define options for message queue functions.  */
#define MSG_NOERROR	010000	/* no error if message is too big */
#ifdef __USE_GNU
# define MSG_EXCEPT	020000	/* recv any msg except of specified type */
#endif

/* Types used in the structure definition.  */
typedef unsigned long int msgqnum_t;
typedef unsigned long int msglen_t;


#ifdef __OCTEON__
/*  For n32 glibc kernel is n64 on Octeon.  Turn long or pointer
    fields either into long long or pad them to match the layout of
    the kernel struct.  __time_t is different for n64 and n32.  */
struct msqid_ds
{
  struct ipc_perm msg_perm;    /* structure describing operation permission */
  unsigned long long __unused1;
  unsigned long long __unused2;
  unsigned int __pad1;
  __time_t msg_stime;          /* time of last msgsnd command */
  unsigned int __pad2;
  __time_t msg_rtime;          /* time of last msgrcv command */
  unsigned int __pad3;
  __time_t msg_ctime;          /* time of last change */
  unsigned long long __unused3;
  unsigned long long __unused4;
  /* The next three field are defined short in the kernel.  */
  unsigned short __msg_cbytes;  /* current number of bytes on queue */
  unsigned short  msg_qnum;    /* number of messages currently on queue */
  unsigned short  msg_qbytes;  /* max number of bytes allowed on queue */
   __pid_t msg_lspid;           /* pid of last msgsnd() */
   __pid_t msg_lrpid;           /* pid of last msgrcv() */
};
#else /* !__OCTEON__ */
/* Structure of record for one message inside the kernel.
   The type `struct msg' is opaque.  */
struct msqid_ds
{
  struct ipc_perm msg_perm;	/* structure describing operation permission */
#if (__WORDSIZE == 32) && !defined(__MIPSEL__)
	unsigned long	__unused1;
#endif
  __time_t msg_stime;		/* time of last msgsnd command */
#if (__WORDSIZE == 32) && defined(__MIPSEL__)
	unsigned long	__unused1;
#endif
#if (__WORDSIZE == 32) && !defined(__MIPSEL__)
	unsigned long	__unused2;
#endif
  __time_t msg_rtime;		/* time of last msgrcv command */
#if (__WORDSIZE == 32) && defined(__MIPSEL__)
	unsigned long	__unused2;
#endif
#if (__WORDSIZE == 32) && !defined(__MIPSEL__)
	unsigned long	__unused3;
#endif
  __time_t msg_ctime;		/* time of last change */
#if (__WORDSIZE == 32) && defined(__MIPSEL__)
	unsigned long	__unused3;
#endif
  unsigned long int __msg_cbytes; /* current number of bytes on queue */
  msgqnum_t msg_qnum;		/* number of messages currently on queue */
  msglen_t msg_qbytes;		/* max number of bytes allowed on queue */
  __pid_t msg_lspid;		/* pid of last msgsnd() */
  __pid_t msg_lrpid;		/* pid of last msgrcv() */
  unsigned long int __unused4;
  unsigned long int __unused5;
};
#endif /* __OCTEON__ */

#ifdef __USE_MISC

# define msg_cbytes	__msg_cbytes

/* ipcs ctl commands */
# define MSG_STAT 11
# define MSG_INFO 12

/* buffer for msgctl calls IPC_INFO, MSG_INFO */
struct msginfo
  {
    int msgpool;
    int msgmap;
    int msgmax;
    int msgmnb;
    int msgmni;
    int msgssz;
    int msgtql;
    unsigned short int msgseg;
  };

#endif /* __USE_MISC */
