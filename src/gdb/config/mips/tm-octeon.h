/* Specify the default prompt */
#define DEFAULT_PROMPT  "(Core#0-gdb) "
 
/* Select the default mips disassembler */
#undef TM_PRINT_INSN_MACH
#define TM_PRINT_INSN_MACH bfd_mach_mips_octeon
