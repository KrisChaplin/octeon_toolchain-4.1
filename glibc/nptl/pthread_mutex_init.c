/* Copyright (C) 2002, 2003, 2004, 2005, 2006 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@redhat.com>, 2002.

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

#include <assert.h>
#include <errno.h>
#include <string.h>
#include "pthreadP.h"


static const struct pthread_mutexattr default_attr =
  {
    /* Default is a normal mutex, not shared between processes.  */
    .mutexkind = PTHREAD_MUTEX_NORMAL
  };


int
__pthread_mutex_init (mutex, mutexattr)
     pthread_mutex_t *mutex;
     const pthread_mutexattr_t *mutexattr;
{
  const struct pthread_mutexattr *imutexattr;

  assert (sizeof (pthread_mutex_t) <= __SIZEOF_PTHREAD_MUTEX_T);

  imutexattr = (const struct pthread_mutexattr *) mutexattr ?: &default_attr;

  /* Sanity checks.  */
  // XXX For now we cannot implement robust mutexes if they are shared.
  if ((imutexattr->mutexkind & PTHREAD_MUTEXATTR_FLAG_ROBUST) != 0
      && (imutexattr->mutexkind & PTHREAD_MUTEXATTR_FLAG_PSHARED) != 0)
    return ENOTSUP;
  // XXX For now we don't support priority inherited or priority protected
  // XXX mutexes.
  if ((imutexattr->mutexkind & PTHREAD_MUTEXATTR_PROTOCOL_MASK)
      != (PTHREAD_PRIO_NONE << PTHREAD_MUTEXATTR_PROTOCOL_SHIFT))
    return ENOTSUP;

  /* Clear the whole variable.  */
  memset (mutex, '\0', __SIZEOF_PTHREAD_MUTEX_T);

  /* Copy the values from the attribute.  */
  mutex->__data.__kind = imutexattr->mutexkind & ~PTHREAD_MUTEXATTR_FLAG_BITS;
  if ((imutexattr->mutexkind & PTHREAD_MUTEXATTR_FLAG_ROBUST) != 0)
    mutex->__data.__kind |= PTHREAD_MUTEX_ROBUST_PRIVATE_NP;
  switch ((imutexattr->mutexkind & PTHREAD_MUTEXATTR_PROTOCOL_MASK)
	  >> PTHREAD_MUTEXATTR_PROTOCOL_SHIFT)
    {
    case PTHREAD_PRIO_INHERIT:
      mutex->__data.__kind |= PTHREAD_MUTEX_PRIO_INHERIT_PRIVATE_NP;
      break;
    case PTHREAD_PRIO_PROTECT:
      mutex->__data.__kind |= PTHREAD_MUTEX_PRIO_PROTECT_PRIVATE_NP;
      if (PTHREAD_MUTEX_PRIO_CEILING_MASK
	  == PTHREAD_MUTEXATTR_PRIO_CEILING_MASK)
	mutex->__data.__kind |= (imutexattr->mutexkind
				 & PTHREAD_MUTEXATTR_PRIO_CEILING_MASK);
      else
	mutex->__data.__kind |= ((imutexattr->mutexkind
				  & PTHREAD_MUTEXATTR_PRIO_CEILING_MASK)
				 >> PTHREAD_MUTEXATTR_PRIO_CEILING_SHIFT)
				<< PTHREAD_MUTEX_PRIO_CEILING_SHIFT;
      break;
    default:
      break;
    }

  /* Default values: mutex not used yet.  */
  // mutex->__count = 0;	already done by memset
  // mutex->__owner = 0;	already done by memset
  // mutex->__nusers = 0;	already done by memset
  // mutex->__spins = 0;	already done by memset
  // mutex->__next = NULL;	already done by memset

  return 0;
}
strong_alias (__pthread_mutex_init, pthread_mutex_init)
INTDEF(__pthread_mutex_init)
