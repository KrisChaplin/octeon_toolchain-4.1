/* strlen.c: Optimized for Octeon. Derived from the generic version in
 * newlib/libc/string/strlen.c.
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

/* File version info: $Id: strlen.c,v 1.2 2006/07/28 21:49:51 cchavva Exp $  */

/* 
FUNCTION
	<<strlen>>---character string length
	
INDEX
	strlen

ANSI_SYNOPSIS
	#include <string.h>
	size_t strlen(const char *<[str]>);

TRAD_SYNOPSIS
	#include <string.h>
	size_t strlen(<[str]>)
	char *<[src]>;

DESCRIPTION
	The <<strlen>> function works out the length of the string
	starting at <<*<[str]>>> by counting chararacters until it
	reaches a <<NULL>> character.

RETURNS
	<<strlen>> returns the character count.

PORTABILITY
<<strlen>> is ANSI C.

<<strlen>> requires no supporting OS subroutines.

QUICKREF
	strlen ansi pure
*/

#include <_ansi.h>
#include <string.h>
#include <limits.h>

#define LBLOCKSIZE   (sizeof (long))
#define UNALIGNED(X) ((long)X & (LBLOCKSIZE - 1))

#if LONG_MAX == 2147483647L
#define DETECTNULL(X) (((X) - 0x01010101) & ~(X) & 0x80808080)
#else
#if LONG_MAX == 9223372036854775807L
/* Nonzero if X (a long int) contains a NULL byte. */
#define DETECTNULL(X) (((X) - 0x0101010101010101) & ~(X) & 0x8080808080808080)
#else
#error long int is not a 32bit or 64bit type.
#endif
#endif

#ifndef DETECTNULL
#error long int is not a 32bit or 64bit byte
#endif

size_t
_DEFUN (strlen, (str),
	_CONST char *str)
{
#if defined(PREFER_SIZE_OVER_SPEED) || defined(__OPTIMIZE_SIZE__)
  _CONST char *start = str;

  while (*str)
    str++;

  return str - start;
#else
  _CONST char *start = str;
  unsigned long *aligned_addr;
  unsigned long temp;
  int align;
  int len;

  align = UNALIGNED(str);

  /* check first 8 bytes and align to 8 byte boundary */
  if (align) {
      if (*str == 0) return 0;
      if (*(str+1) == 0) return 1;
      if (*(str+2) == 0) return 2;
      if (*(str+3) == 0) return 3;
      if (*(str+4) == 0) return 4;
      if (*(str+5) == 0) return 5;
      if (*(str+6) == 0) return 6;
      if (*(str+7) == 0) return 7;
      str += (8 - align);
  }

  /* now word-aligned, we can check for the presence of 
     a null in each word-sized block.  */
#if 0
  aligned_addr = (unsigned long*)str;
  while (!DETECTNULL (*aligned_addr))
      aligned_addr++;
#else
  aligned_addr = (unsigned long*)str;
  do {
      temp = *aligned_addr;
      aligned_addr++;
  } while (!DETECTNULL(temp));
  aligned_addr--;
#endif

  /* Once a null is detected, we check each byte in that block for a
     precise position of the null.  */
  str = (char*)aligned_addr;

  len = str - start;
  if (*str == 0) goto done;
  len++;
  if (*(str+1) == 0) goto done;
  len++;
  if (*(str+2) == 0) goto done;
  len++;
  if (*(str+3) == 0) goto done;
  len++;
  if (*(str+4) == 0) goto done;
  len++;
  if (*(str+5) == 0) goto done;
  len++;
  len += (*(str+6) != 0);
  done:
  return len;

#endif /* not PREFER_SIZE_OVER_SPEED */
}
