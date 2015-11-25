/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2004 Cavium Networks
 */
#ifndef __ASM_MACH_CAVIUM_OCTEON_CPU_FEATURE_OVERRIDES_H
#define __ASM_MACH_CAVIUM_OCTEON_CPU_FEATURE_OVERRIDES_H

/*
 * Cavium Octeons are MIPS64v2 processors
 */
#define cpu_dcache_line_size()	128
#define cpu_icache_line_size()	128

#define cpu_has_llsc		1
#define cpu_has_prefetch  	1
#define cpu_has_dc_aliases  0

#endif
