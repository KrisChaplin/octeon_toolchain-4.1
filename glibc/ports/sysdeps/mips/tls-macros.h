/* Macros to support TLS testing in times of missing compiler support.  */

#if _MIPS_SIM != _ABI64

/* These versions are for o32 and n32.  */

# define TLS_GD(x)					\
  ({ void *__result;					\
     extern void *__tls_get_addr (void *);		\
     asm ("addiu %0, $28, %%tlsgd(" #x ")"		\
	  : "=r" (__result));				\
     (int *)__tls_get_addr (__result); })
#else
# define TLS_GD(x)					\
  ({ void *__result;					\
     extern void *__tls_get_addr (void *);		\
     asm ("daddiu %0, $28, %%tlsgd(" #x ")"		\
	  : "=r" (__result));				\
     (int *)__tls_get_addr (__result); })
#endif

#if _MIPS_SIM != _ABI64
# define TLS_LD(x)					\
  ({ void *__result;					\
     extern void *__tls_get_addr (void *);		\
     asm ("addiu %0, $28, %%tlsldm(" #x ")"		\
	  : "=r" (__result));				\
     __result = __tls_get_addr (__result);		\
     asm ("lui $3,%%dtprel_hi(" #x ")\n\t"		\
	  "addiu $3,$3,%%dtprel_lo(" #x ")\n\t"		\
	  "addu %0,%0,$3"				\
	  : "+r" (__result) : : "$3");			\
     __result; })
# ifndef __OCTEON__ 
# define TLS_IE(x)					\
  ({ void *__result;					\
     asm (".set push\n\t.set mips32r2\n\t"		\
	  "rdhwr\t%0,$29\n\t.set pop"			\
	  : "=v" (__result));				\
     asm ("lw $3,%%gottprel(" #x ")($28)\n\t"		\
	  "addu %0,%0,$3"				\
	  : "+r" (__result) : : "$3");			\
     __result; })
# define TLS_LE(x)					\
  ({ void *__result;					\
     asm (".set push\n\t.set mips32r2\n\t"		\
	  "rdhwr\t%0,$29\n\t.set pop"			\
	  : "=v" (__result));				\
     asm ("lui $3,%%tprel_hi(" #x ")\n\t"		\
	  "addiu $3,$3,%%tprel_lo(" #x ")\n\t"		\
	  "addu %0,%0,$3"				\
	  : "+r" (__result) : : "$3");			\
     __result; })

# else /* OCTEON */
/* The Kernel stores the value of "rdhwr v1,$29" in k0 ($26) register. And
   it is the Kernel's responsibility to always have the correct value in 
   k0.  Replacing rdhwr instruction with k0, as this instruction needs to 
   be emulated by the Kernel.  */
# define TLS_IE(x)					\
  ({ void *__result;					\
     asm ("lw $3,%%gottprel(" #x ")($28)\n\t"		\
	  "addu %0,$26,$3"				\
	  : "=r" (__result) : : "$3");			\
     __result; })
# define TLS_LE(x)					\
  ({ void *__result;					\
     asm ("lui $3,%%tprel_hi(" #x ")\n\t"		\
	  "addiu $3,$3,%%tprel_lo(" #x ")\n\t"		\
	  "addu %0,$26,$3"				\
	  : "=r" (__result) : : "$3");			\
     __result; })
# endif /* OCTEON */
#else

/* These versions are for n64.  */

# define TLS_LD(x)					\
  ({ void *__result;					\
     extern void *__tls_get_addr (void *);		\
     asm ("daddiu %0, $28, %%tlsldm(" #x ")"		\
	  : "=r" (__result));				\
     __result = __tls_get_addr (__result);		\
     asm ("lui $3,%%dtprel_hi(" #x ")\n\t"		\
	  "daddiu $3,$3,%%dtprel_lo(" #x ")\n\t"	\
	  "daddu %0,%0,$3"				\
	  : "+r" (__result) : : "$3");			\
     __result; })
# ifndef __OCTEON__
# define TLS_IE(x)					\
  ({ void *__result;					\
     asm (".set push\n\t.set mips32r2\n\t"		\
	  "rdhwr\t%0,$29\n\t.set pop"			\
	  : "=v" (__result));				\
     asm ("ld $3,%%gottprel(" #x ")($28)\n\t"		\
	  "daddu %0,%0,$3"				\
	  : "+r" (__result) : : "$3");			\
     __result; })
# define TLS_LE(x)					\
  ({ void *__result;					\
     asm (".set push\n\t.set mips32r2\n\t"		\
	  "rdhwr\t%0,$29\n\t.set pop"			\
	  : "=v" (__result));				\
     asm ("lui $3,%%tprel_hi(" #x ")\n\t"		\
	  "daddiu $3,$3,%%tprel_lo(" #x ")\n\t"		\
	  "daddu %0,%0,$3"				\
	  : "+r" (__result) : : "$3");			\
     __result; })
# else /* OCTEON */
# define TLS_IE(x)					\
  ({ void *__result;					\
     asm ("ld $3,%%gottprel(" #x ")($28)\n\t"		\
	  "daddu %0,$26,$3"				\
	  : "=r" (__result) : : "$3");			\
     __result; })
# define TLS_LE(x)					\
  ({ void *__result;					\
     asm ("lui $3,%%tprel_hi(" #x ")\n\t"		\
	  "daddiu $3,$3,%%tprel_lo(" #x ")\n\t"		\
	  "daddu %0,$26,$3"				\
	  : "=r" (__result) : : "$3");			\
     __result; })
# endif /* OCTEON */
#endif
