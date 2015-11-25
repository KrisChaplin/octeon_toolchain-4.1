/* Copyright (C) 1995,1996,1997,2000,2001,2002,2003 Free Software Foundation, Inc.
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

#ifndef _SYS_SHM_H
# error "Never include <bits/shm.h> directly; use <sys/shm.h> instead."
#endif

#include <bits/types.h>

/* Permission flag for shmget.  */
#define SHM_R		0400		/* or S_IRUGO from <linux/stat.h> */
#define SHM_W		0200		/* or S_IWUGO from <linux/stat.h> */

/* Flags for `shmat'.  */
#define SHM_RDONLY	010000		/* attach read-only else read-write */
#define SHM_RND		020000		/* round attach address to SHMLBA */
#define SHM_REMAP	040000		/* take-over region on attach */

/* Commands for `shmctl'.  */
#define SHM_LOCK	11		/* lock segment (root only) */
#define SHM_UNLOCK	12		/* unlock segment (root only) */

/* Segment low boundary address multiple.  */
#define SHMLBA		0x40000


#if !defined (__OCTEON__) || __WORDSIZE != 32
/* Type to count number of attaches.  */
typedef unsigned long int shmatt_t;

/* Data structure describing a set of semaphores.  */
struct shmid_ds
  {
    struct ipc_perm shm_perm;		/* operation permission struct */
    /* Change the type to match the kernel as in include/linux/shm.h. */
    int shm_segsz;
    __time_t shm_atime;			/* time of last shmat() */
    __time_t shm_dtime;			/* time of last shmdt() */
    __time_t shm_ctime;			/* time of last change by shmctl() */
    __pid_t shm_cpid;			/* pid of creator */
    __pid_t shm_lpid;			/* pid of last shmop */
    /* Change the type of the next two fields to match the kernel. */
    unsigned short shm_nattch;         /* number of current attaches */
    unsigned short shm_unused;
    unsigned long int __unused1;
    unsigned long int __unused2;
  };
#else /* defined (__OCTEON__) && __WORDSIZE == 32 */
  /*  For n32 glibc kernel is n64 on Octeon.  Turn long or pointer
      fields either into long long or pad them to match the layout of
      the kernel struct.  __time_t is different for n64 and n32.  */
struct shmid_ds
  {
    struct ipc_perm shm_perm;          /* operation permission struct */
    /* Change the type to match the kernel as in include/linux/shm.h. */
    int shm_segsz;                     /* size of segment in bytes */
    unsigned int __pad1;
    __time_t shm_atime;                        /* time of last shmat() */
    unsigned int __pad2;
    __time_t shm_dtime;                        /* time of last shmdt() */
    unsigned int __pad3;
    __time_t shm_ctime;                        /* time of last change by shmctl() */
    __pid_t shm_cpid;                  /* pid of creator */
    __pid_t shm_lpid;                  /* pid of last shmop */
    /* Change the type of the next two fields to match the kernel. */
    unsigned short shm_nattch;         /* number of current attaches */
    unsigned short shm_unused;
    unsigned long long __unused1;
    unsigned long long __unused2;
  };
#endif /* !defined (__OCTEON__) || __WORDSIZE != 32 */

#ifdef __USE_MISC

/* ipcs ctl commands */
# define SHM_STAT	13
# define SHM_INFO	14

/* shm_mode upper byte flags */
# define SHM_DEST	01000	/* segment will be destroyed on last detach */
# define SHM_LOCKED	02000   /* segment will not be swapped */
# define SHM_HUGETLB	04000	/* segment is mapped via hugetlb */

struct shminfo
  {
    unsigned long int shmmax;
    unsigned long int shmmin;
    unsigned long int shmmni;
    unsigned long int shmseg;
    unsigned long int shmall;
    unsigned long int __unused1;
    unsigned long int __unused2;
    unsigned long int __unused3;
    unsigned long int __unused4;
  };

struct shm_info
  {
    int used_ids;
    unsigned long int shm_tot;  /* total allocated shm */
    unsigned long int shm_rss;  /* total resident shm */
    unsigned long int shm_swp;  /* total swapped shm */
    unsigned long int swap_attempts;
    unsigned long int swap_successes;
  };

#endif /* __USE_MISC */
