/* Macros for mips*-octeon-elf target.
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

/* Add MASK_SOFT_FLOAT and MASK_OCTEON_UNALIGNED.  */

#undef TARGET_DEFAULT
#define TARGET_DEFAULT (MASK_SOFT_FLOAT | MASK_OCTEON_UNALIGNED)

/* Forward -m*octeon-useun.  */

#undef SUBTARGET_ASM_SPEC
#define SUBTARGET_ASM_SPEC "%{mocteon-useun} %{mno-octeon-useun}"

/* Link to libc library.  */

#undef LIB_SPEC
#define LIB_SPEC "-lc"

/* Link to startup file.  */

#undef STARTFILE_SPEC
#define STARTFILE_SPEC "crti%O%s crtbegin%O%s crt0%O%s"

#undef  SUBTARGET_CPP_SPEC
#define SUBTARGET_CPP_SPEC "-D__OCTEON__"

/* Add N32.  */

#define TARGET_OS_CPP_BUILTINS()		\
  do {						\
    if (mips_abi == ABI_N32)			\
      {						\
        builtin_define ("_ABIN32=2");		\
        builtin_define ("_MIPS_SIM=_ABIN32");	\
        builtin_define ("_MIPS_SZLONG=32");	\
        builtin_define ("_MIPS_SZPTR=32");	\
      }						\
    builtin_define ("_MIPS_SZINT=32");		\
  } while (0)

/* Pass linker emulation mode for N32.  */

#undef LINK_SPEC
#define LINK_SPEC "\
%(endian_spec) \
%{G*} %{mips1} %{mips2} %{mips3} %{mips4} %{mips32} %{mips32r2} %{mips64} \
%{mips64r2} %{bestGnum} %{shared} %{non_shared} \
%{mabi=n32:-melf32e%{!EL:b}%{EL:l}octeonn32}"

/* Override because of N32.  */

#undef LOCAL_LABEL_PREFIX
#define LOCAL_LABEL_PREFIX ((mips_abi == ABI_N32) ? "." : "$")

/* Newlib's implementation of strcmp is faster with word-aligned
   strings.  */

#define LOCAL_ALIGNMENT(TYPE, ALIGN)					\
  ((ALIGN) < BITS_PER_WORD && TREE_CODE (TYPE) == ARRAY_TYPE		\
   ? BITS_PER_WORD : (ALIGN))
