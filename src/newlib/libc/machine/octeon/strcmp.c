/* strcmp.c: Optimized for Octeon. derived from both the generic version in 
 * newlib/libc/string/strcmp.c and the mips version in 
 * newlib/libc/machine/mips/strcmp.c.
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

/* File version info: $Id: strcmp.c,v 1.2 2006/07/28 21:49:51 cchavva Exp $  */

#include <string.h>
#include <limits.h>

/* Nonzero if either X or Y is not aligned on a "long" boundary.  */
#define UNALIGNED(X, Y) \
  (((long)X & (sizeof (long) - 1)) | ((long)Y & (sizeof (long) - 1)))

/* DETECTNULL returns nonzero if (long)X contains a NULL byte. */
#if LONG_MAX == 2147483647L
#define DETECTNULL(X) (((X) - 0x01010101) & ~(X) & 0x80808080)
#else
#if LONG_MAX == 9223372036854775807L
#define DETECTNULL(X) (((X) - 0x0101010101010101) & ~(X) & 0x8080808080808080)
#else
#error long int is not a 32bit or 64bit type.
#endif
#endif

#ifndef DETECTNULL
#error long int is not a 32bit or 64bit byte
#endif
int strcmp(const char *s1, const char *s2)
{ 
    /* If s1 or s2 are unaligned, then compare bytes. */
    if (!UNALIGNED (s1, s2)) {

        unsigned long *a1;
        unsigned long *a2;

        /* If s1 and s2 are word-aligned, compare them a word at a time. */
        a1 = (unsigned long*)s1;
        a2 = (unsigned long*)s2;
        while (*a1 == *a2)
        {
            /* To get here, *a1 == *a2, thus if we find a null in *a1,
               then the strings must be equal, so return zero.  */
            if (DETECTNULL (*a1))
                return 0;

            a1++;
            a2++;
        }

        /* A difference was detected in last few bytes of s1, so search bytewise */
        s1 = (char*)a1;
        s2 = (char*)a2;
    }

    {
        unsigned const char *us1 = (unsigned const char *)s1;
        unsigned const char *us2 = (unsigned const char *)s2;
        int c1a, c1b;
        int c2a, c2b;

        /* If the pointers aren't both aligned to a 16-byte boundary, do the
           comparison byte by byte, so that we don't get an invalid page fault if we
           are comparing a string whose null byte is at the last byte on the last
           valid page.  */
        if (((((long)us1) | ((long)us2)) & 1) == 0)
        {
            c1a = *us1;
#if 0
            { asm volatile("nop"); }  /* ### Yen: workaround for sim perf bug */
#endif
            for (;;)
            {
                c1b = *us2;
                us1 += 2;
                if (c1a == '\0')
                    goto ret1;

                c2a = us1[-1];
                if (c1a != c1b)
                    goto ret1;

                c2b = us2[1];
                us2 += 2;
                if (c2a == '\0')
                    break;

                c1a = *us1;
                if (c2a != c2b)
                    break;
            }

            return c2a - c2b;
        }
        else
        {
            do
            {
                c1a = *us1++;
                c1b = *us2++;
                if (c1a == '\0' || c1a != c1b) break;
                c1a = *us1++;
                c1b = *us2++;
            }
            while (c1a != '\0' && c1a == c1b);
        }

      ret1:
        return c1a - c1b;
    }
}
