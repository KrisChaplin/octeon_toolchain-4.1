/* memset.c: Optimized for Octeon.
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
 * they apply.  */

/* File version info: $Id: memset.c,v 1.2 2006/07/28 21:49:51 cchavva Exp $  */

/*
FUNCTION
	<<memset>>---Set an area of memory, optimized for the MIPS processors

INDEX
	memset

ANSI_SYNOPSIS
	#include <string.h>
	void *memset(const void *<[dst]>, int <[c]>, size_t <[length]>);

TRAD_SYNOPSIS
	#include <string.h>
	void *memset(<[dst]>, <[c]>, <[length]>)
	void *<[dst]>;
	int <[c]>;
	size_t <[length]>;

DESCRIPTION
	This function converts the argument <[c]> into an unsigned
	char and fills the first <[length]> characters of the array
	pointed to by <[dst]> to the value.

RETURNS
	<<memset>> returns the value of <[m]>.

PORTABILITY
<<memset>> is ANSI C.

    <<memset>> requires no supporting OS subroutines.

QUICKREF
	memset ansi pure
*/

#include <string.h>

#ifdef __mips64
#define wordtype long long
#else
#define wordtype long
#endif

#define LBLOCKSIZE     (sizeof(wordtype))
#define UNALIGNED(X)   ((long)(X) & (LBLOCKSIZE - 1))
#define TOO_SMALL(LEN) ((LEN) < LBLOCKSIZE * 4)
#define LARGE_BLOCK(LEN)  ((LEN) > 256)

#if !defined(PREFER_SIZE_OVER_SPEED) && !defined(__OPTIMIZE_SIZE__) && !defined(__mips16)
/*
 * optimized Cavium Networks Octeon cacheline memset routine.
 * preconditions: s is cache line aligned, n is integral multiple of 128.
 *
 */
static inline void cache_memset(void * s, long long pat, size_t n)
{
    long long * p = s;

    while (n > 0) {
        *p = pat;
        *(p+1) = pat;
        *(p+2) = pat;
        *(p+3) = pat;
        *(p+4) = pat;
        *(p+5) = pat;
        *(p+6) = pat;
        *(p+7) = pat;
        *(p+8) = pat;
        *(p+9) = pat;
        *(p+10) = pat;
        *(p+11) = pat;
        *(p+12) = pat;
        *(p+13) = pat;
        *(p+14) = pat;
        *(p+15) = pat;
        p += 16;
        n -= 128;
    }
}
#endif

_PTR 
_DEFUN (memset, (m, c, n),
	_PTR m _AND
	int c _AND
	size_t n)
{
#if defined(PREFER_SIZE_OVER_SPEED) || defined(__OPTIMIZE_SIZE__) || defined(__mips16)
  char *s = (char *) m;

  while (n-- != 0)
    {
      *s++ = (char) c;
    }

  return m;
#else
  char *s = (char *) m;
  int i;
  unsigned wordtype buffer;
  unsigned wordtype *aligned_addr;
  unsigned short *short_addr;
  size_t iter;

  if (!TOO_SMALL (n))
    {
      int unaligned = UNALIGNED (s);

      /* We know that N is >= LBLOCKSIZE so we can just word
         align the S without having to check the length. */

      if (unaligned)
	{
	  while (unaligned++ < LBLOCKSIZE)
	    *s++ = (char)c, n--;
	}

      /* S is now word-aligned so we can process the remainder
         in word sized chunks except for a few (< LBLOCKSIZE)
         bytes which might be left over at the end. */

      aligned_addr = (unsigned wordtype *)s;

      /* Store C into each char sized location in BUFFER so that
         we can set large blocks quickly.  */
      c &= 0xff;
      buffer = c;
      if (buffer != 0)
	{
	  if (LBLOCKSIZE == 4)
	    {
	       buffer |= (buffer << 8);
	       buffer |= (buffer << 16);
	    }
	  else if (LBLOCKSIZE == 8)
	    {
	      buffer |= (buffer << 8);
	      buffer |= (buffer << 16);
	      buffer |= ((buffer << 31) << 1);
	    }
	  else
	    {
	      for (i = 1; i < LBLOCKSIZE; i++)
		buffer = (buffer << 8) | c;
	    }
        }

      if (LARGE_BLOCK(n)) goto memset_large_block;
      return_large_block:

      iter = n / (2*LBLOCKSIZE);
      n = n % (2*LBLOCKSIZE);
      while (iter > 0)
	{
	  aligned_addr[0] = buffer;
	  aligned_addr[1] = buffer;
	  aligned_addr += 2;
	  iter--;
	}

      if (n >= LBLOCKSIZE)
	{
	  *aligned_addr++ = buffer;
	  n -= LBLOCKSIZE;
	}

      /* Pick up the remainder with a bytewise loop.  */
      s = (char*)aligned_addr;
    }

  while (n > 0)
    {
      *s++ = (char)c;
      n--;
    }

  return m;

  memset_large_block:
  {
      int align;
      align = 128 - (((unsigned long) aligned_addr) & 127);
      iter = align / (2*LBLOCKSIZE);
      n -= iter * 2*LBLOCKSIZE;
      while (iter > 0)
      {
          aligned_addr[0] = buffer;
          aligned_addr[1] = buffer;
          aligned_addr += 2;
          iter--;
      }
      while ((((unsigned long) aligned_addr) & 127)) {
          *aligned_addr++ = buffer;
          n -= LBLOCKSIZE;
      }
      align = n & ~127;
      cache_memset(aligned_addr, buffer, align);
      aligned_addr = aligned_addr + (align / LBLOCKSIZE);
      n -= align;
      goto return_large_block;
  }

#endif /* not PREFER_SIZE_OVER_SPEED */
}
