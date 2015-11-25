/* Macros for mips*-octeon-linux target.
   Copyright (C) 2004, 2005, 2006 Cavium Networks.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING.  If not, write to
the Free Software Foundation, 51 Franklin Street, Fifth Floor,
Boston, MA 02110-1301, USA.  */

/* Add MASK_SOFT_FLOAT.  */

#undef TARGET_DEFAULT
#define TARGET_DEFAULT (MASK_ABICALLS | MASK_SOFT_FLOAT)

/* Forward -m*octeon-useun.  */

#undef SUBTARGET_ASM_SPEC
#define SUBTARGET_ASM_SPEC "\
%{!fno-PIC:%{!fno-pic:-KPIC}} \
%{fno-PIC:-non_shared} %{fno-pic:-non_shared} \
%{mocteon-useun} %{mno-octeon-useun}"

/* Add __OCTEON__.  */

#undef  SUBTARGET_CPP_SPEC
#define SUBTARGET_CPP_SPEC "%{pthread:-D_REENTRANT} -D__OCTEON__"

/* __cxa_atexit is not supported by uClibc. As __cxa_atexit is enabled at 
   configure time, as the same toolchain supports both glibc and uclibc 
   libraries, disable __cxa_atexit while compile C++ programs for uClibc.  */

#undef CC1PLUS_SPEC
#define CC1PLUS_SPEC "%{muclibc: -fno-use-cxa-atexit}"

/* uClibc is only supported with n32.  Default to n64 ABI instead of n32.  */

#undef DRIVER_SELF_SPECS
#define DRIVER_SELF_SPECS \
"%{!EB:%{!EL:%(endian_spec)}}", \
"%{muclibc: -mabi=n32}", \
"%{!mabi=*: -mabi=64}"

/* Append /uclibc to the sysroot when looking for headers and
   libraries.  */

#undef SYSROOT_HEADERS_SUFFIX_SPEC
#define SYSROOT_HEADERS_SUFFIX_SPEC\
  "%{muclibc:/uclibc}" 

#undef SYSROOT_SUFFIX_SPEC
#define SYSROOT_SUFFIX_SPEC\
  "%{muclibc:/uclibc}"

/* Diagnose unsupported combinations with uClibc.  */

#ifdef CROSS_COMPILE
# define OCTEON_LINK_SPEC_DIAGNOSE_UCLIBC \
"%{muclibc: %{mabi=64: %eno n64 support with uClibc}}"
#else
# define OCTEON_LINK_SPEC_DIAGNOSE_UCLIBC \
"%{muclibc: %eno native support for uClibc}"
#endif

/* Add uClibc's dynamic linker.  */

#undef LINK_SPEC
#define LINK_SPEC "\
%{G*} %{EB} %{EL} %{mips1} %{mips2} %{mips3} %{mips4} \
%{bestGnum} %{shared} %{non_shared} \
%{call_shared} %{no_archive} %{exact_version} \
 %(endian_spec) \
  %{!shared: \
    %{!ibcs: \
      %{!static: \
        %{rdynamic:-export-dynamic} \
        %{!dynamic-linker: \
	  %{mabi=n32: \
	    %{muclibc: -dynamic-linker /uclibc/lib/ld-uClibc.so.0 ; \
	      : -dynamic-linker /lib32/ld.so.1}} \
	  %{mabi=64: -dynamic-linker /lib64/ld.so.1} \
	  %{mabi=32: -dynamic-linker /lib/ld.so.1}}} \
      %{static:-static}}} \
%{mabi=n32:-melf32%{EB:b}%{EL:l}tsmipn32} \
%{mabi=64:-melf64%{EB:b}%{EL:l}tsmip} \
%{mabi=32:-melf32%{EB:b}%{EL:l}tsmip} " \
OCTEON_LINK_SPEC_DIAGNOSE_UCLIBC
