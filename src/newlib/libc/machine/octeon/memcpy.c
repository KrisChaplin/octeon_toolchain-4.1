/* memcpy.c: Optimized for Octeon.
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

/* File version info: $Id: memcpy.c,v 1.2 2006/07/28 21:49:51 cchavva Exp $  */

/*
FUNCTION
        <<memcpy>>---Copy memory regions, optimized for the mips processors

ANSI_SYNOPSIS
        #include <string.h>
        void* memcpy(void *<[out]>, const void *<[in]>, size_t <[n]>);

TRAD_SYNOPSIS
        void *memcpy(<[out]>, <[in]>, <[n]>
        void *<[out]>;
        void *<[in]>;
        size_t <[n]>;

DESCRIPTION
        This function copies <[n]> bytes from the memory region
        pointed to by <[in]> to the memory region pointed to by
        <[out]>.

        If the regions overlap, the behavior is undefined.

RETURNS
        <<memcpy>> returns a pointer to the first byte of the <[out]>
        region.

PORTABILITY
<<memcpy>> is ANSI C.

<<memcpy>> requires no supporting OS subroutines.

QUICKREF
        memcpy ansi pure
	*/

#include <_ansi.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>


    


#define CVMX_LOADUNA_INT16(result, address, offset) \
	asm ("ulh %[rdest], " CVMX_TMP_STR(offset) "(%[rbase])" : [rdest] "=d" (result) : [rbase] "d" (address))
#define CVMX_LOADUNA_UINT16(result, address, offset) \
	asm ("ulhu %[rdest], " CVMX_TMP_STR(offset) "(%[rbase])" : [rdest] "=d" (result) : [rbase] "d" (address))
#define CVMX_STOREUNA_INT16(data, address, offset) \
	asm volatile ("ush %[rsrc], " CVMX_TMP_STR(offset) "(%[rbase])" : : [rsrc] "d" (data), [rbase] "d" (address))

#define CVMX_LOADUNA_INT32(result, address, offset) \
	asm ("ulw %[rdest], " CVMX_TMP_STR(offset) "(%[rbase])" : [rdest] "=d" (result) : [rbase] "d" (address))
#define CVMX_STOREUNA_INT32(data, address, offset) \
	asm volatile ("usw %[rsrc], " CVMX_TMP_STR(offset) "(%[rbase])" : : [rsrc] "d" (data), [rbase] "d" (address))

#define CVMX_LOADUNA_INT64(result, address, offset) \
	asm ("uld %[rdest], " CVMX_TMP_STR(offset) "(%[rbase])" : [rdest] "=d" (result) : [rbase] "d" (address))
#define CVMX_STOREUNA_INT64(data, address, offset) \
	asm volatile ("usd %[rsrc], " CVMX_TMP_STR(offset) "(%[rbase])" : : [rsrc] "d" (data), [rbase] "d" (address))

#define CVMX_LOAD_INT8(result, address, offset) \
	asm ("lb %[rdest], " CVMX_TMP_STR(offset) "(%[rbase])" : [rdest] "=d" (result) : [rbase] "d" (address))
#define CVMX_STORE_INT8(data, address, offset) \
	asm volatile ("sb %[rsrc], " CVMX_TMP_STR(offset) "(%[rbase])" : : [rsrc] "d" (data), [rbase] "d" (address))
    

#define CVMX_TMP_STR(x) CVMX_TMP_STR2(x)
#define CVMX_TMP_STR2(x) #x
#define CVMX_PREFETCH(address, offset) CVMX_PREFETCH_PREF0(address, offset)
// normal prefetches that use the pref instruction
#define CVMX_PREFETCH_PREF0(address, offset) asm volatile ("pref 0, " CVMX_TMP_STR(offset) "(%[rbase])" : : [rbase] "d" (address) )


#ifdef __mips64
#define wordtype long long
#define WORDSIZE_EXP    3
#else
#define wordtype long
#define WORDSIZE_EXP    2
#endif
#define WORDSIZE    (1 << WORDSIZE_EXP)

/* Nonzero if either X or Y is not aligned on a "long" boundary.  */
#define UNALIGNED(X, Y) ((((long)X) | ((long)Y)) & (sizeof(wordtype) - 1))

  
/* How many bytes are copied each iteration of the 4X unrolled loop.  */
#define BIGBLOCKSIZE    (sizeof (wordtype) << 2)

/* How many bytes are copied each iteration of the word copy loop.  */
#define LITTLEBLOCKSIZE (sizeof (wordtype))


_PTR
_DEFUN (memcpy, (dst0, src0, len0),
	_PTR dst0 _AND
	_CONST _PTR src0 _AND
	size_t len0)
{
    char *dst = dst0;
    _CONST char *src = src0;
    wordtype *wordtype_dst;
    _CONST wordtype *wordtype_src;
    int   len =  len0;
    size_t iter;
    _CONST char *endcptr;
    _CONST wordtype *endptr;
    int tmp;

    wordtype tmp0, tmp1;

    CVMX_PREFETCH(src, 0);

    wordtype_src = (wordtype *)src;
    wordtype_dst = (wordtype *)dst;

    
    if (UNALIGNED (src, dst))
        goto unaligned;

    if (len < BIGBLOCKSIZE)
        goto aligned_small_blocks;


    /* Handle aligned copies here.  */



aligned_block:
        endptr = wordtype_src + ((len & ~(BIGBLOCKSIZE - 1)) >> WORDSIZE_EXP);
        len &= BIGBLOCKSIZE - 1;
        while (wordtype_src != endptr)
        {
            CVMX_PREFETCH(wordtype_src, 128);
            tmp0 = wordtype_src[0];
            tmp1 = wordtype_src[1];
            wordtype_dst[0] = tmp0;
            wordtype_dst[1] = tmp1;

            tmp0 = wordtype_src[2];
            tmp1 = wordtype_src[3];
            wordtype_dst[2] = tmp0;
            wordtype_dst[3] = tmp1;

            wordtype_src += 4;
            wordtype_dst += 4;
        }

        if (!len)
            goto done;

aligned_small_blocks:
        /* Copy one long or long long word at a time if possible.  */


        while (len >= LITTLEBLOCKSIZE)
        {
            len -= LITTLEBLOCKSIZE;
            tmp0 = *wordtype_src++;
            *wordtype_dst++ = tmp0;
        }


        dst = (char*)wordtype_dst;
        src = (char*)wordtype_src;


        // faster than 7 way case statement
        if (len < 4)
            goto aligned_321;

        len -= 4;
        *((uint32_t *)dst) = *((uint32_t *)src);
        src += 4;
        dst += 4;

aligned_321:


        switch (len)
        {
            case 3:
                *(dst + 2) = *(src + 2);
            case 2:
                *((uint16_t *)dst) = *((uint16_t *)src);
                break;
            case 1:
                *(dst) = *(src);
            default:
                break;
        }


        
done:
        return dst0;


        /* Case were both src/dst are unaligned,
        ** make dst aligned, then copy
        */
unaligned:

        wordtype_dst = (wordtype *)dst;
        wordtype_src = (wordtype *)src;

        if (len < BIGBLOCKSIZE)
            goto unaligned_small_blocks;


        tmp1 = sizeof(wordtype) - ((long)wordtype_dst & (sizeof(wordtype) - 1));
        /* Align dst ptr with one oversize load/store pair.  Since this is the beginning of the copy
        ** and we checked to make sure that the block is large enought, this should be safe.
        */
        CVMX_LOADUNA_INT64(tmp0, src, 0);
        len -= tmp1;
        CVMX_STOREUNA_INT64(tmp0, dst, 0);
        wordtype_src = (char *)wordtype_src + tmp1;
        wordtype_dst = (char *)wordtype_dst + tmp1;

        /* dst is now aligned, check src */
        if (!((long)wordtype_src & (sizeof(wordtype) - 1)))
            goto aligned_block;


        endptr = wordtype_dst + ((len & ~(BIGBLOCKSIZE - 1)) >> WORDSIZE_EXP);
        len &= BIGBLOCKSIZE - 1;

        while (wordtype_dst != endptr)
        {

            CVMX_PREFETCH(wordtype_src, 128);
            CVMX_LOADUNA_INT64(tmp0, wordtype_src, 0);
            CVMX_LOADUNA_INT64(tmp1, wordtype_src, WORDSIZE);
            wordtype_dst[0] = tmp0;
            wordtype_dst[1] = tmp1;

            CVMX_LOADUNA_INT64(tmp0, wordtype_src, 2 * WORDSIZE);
            CVMX_LOADUNA_INT64(tmp1, wordtype_src, 3 * WORDSIZE);
            wordtype_dst[2] = tmp0;
            wordtype_dst[3] = tmp1;

            wordtype_src += 4;
            wordtype_dst += 4;
        }

        if (!len)
            goto done;


unaligned_small_blocks:
        /* Copy one long or long long word at a time if possible.  */
        /* One cycle slower worst case, faster all others */
        while (len >= LITTLEBLOCKSIZE)
        {
            CVMX_LOADUNA_INT64(tmp0, wordtype_src++, 0);
            len -= LITTLEBLOCKSIZE;
            CVMX_STOREUNA_INT64(tmp0, wordtype_dst++, 0);
        }


        /* Pick up any residual with a byte copier.  */
        dst = (char*)wordtype_dst;
        src = (char*)wordtype_src;

        
unaligned_last_bytes:
        if (len < 4)
            goto unaligned_end321;

        len -= 4;
        CVMX_LOADUNA_INT32(tmp0, src, 0);
        src += 4;
        CVMX_STOREUNA_INT32(tmp0, dst, 0);
        dst += 4;
        
unaligned_end321:
        switch (len)
        {
            case 3:
                CVMX_LOADUNA_INT16(tmp0, src, 0);
                CVMX_LOAD_INT8(tmp1, src, 2);
                CVMX_STOREUNA_INT16(tmp0, dst, 0);
                CVMX_STORE_INT8(tmp1, dst, 2);
                break;
            case 2:
                CVMX_LOADUNA_INT16(tmp0, src, 0);
                CVMX_STOREUNA_INT16(tmp0, dst, 0);
                break;
            case 1:
                *dst = *src;
                break;
        }


        return(dst0);

}
