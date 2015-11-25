/* Helper functions for Octeon startup and exit calls.
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

#include "octeon-cvmx.h"
#include "octeon-app-init.h"
#include <stdio.h>
#include <stddef.h>

void octeon_os_use_uart (int enable_uart);
void octeon_os_set_uart_num (int uart_num);

/* Defined in octeon-os.c.  */
extern uint64_t boot_heap_base;
extern uint64_t boot_heap_end;
extern uint32_t octeon_cpu_clock_hz;

static int debug_active = 0;
static uint64_t debug_flags_base_addr = 0;

uint64_t __octeon_argc;
uint64_t __octeon_argv;

static char *argv_array[OCTEON_ARGV_MAX_ARGS];

/* Main entry point for all simple executive based programs. 
   This is the first C function called. It completes 
   initialization, calls main, and performs C level cleanup.
   APP_DESC_ADDR is the address of the application description structure 
   passed from the bootloader.  */
void __octeon_app_init (uint64_t app_desc_addr)
{
  /* App descriptor used by bootloader.  */
  octeon_boot_descriptor_t *app_desc_ptr = (void *)app_desc_addr;
  const uint32_t bootloader_flags = app_desc_ptr->flags;

  /* The following call controls if output to stdout and stderr go to
     the uart or simprintf. It needs to be early so no printfs are 
     missed. It only sets a static variable, so it is safe to call
     before everything is setup.  */
  octeon_os_use_uart (!!(bootloader_flags & OCTEON_BL_FLAG_NO_MAGIC));

  /* Select which uart to use 0 or 1 */
  octeon_os_set_uart_num (!!(bootloader_flags & OCTEON_BL_FLAG_CONSOLE_UART1));


  /* Heap is per core.  */
  boot_heap_base = app_desc_ptr->heap_base;
  boot_heap_end = app_desc_ptr->heap_end;

  /* This code only cares about heap, argc/argv, flags, and 
     debugger_flags_base_addr.  */
  if (app_desc_ptr->desc_version < 3 
      || app_desc_ptr->desc_version > OCTEON_CURRENT_DESC_VERSION)
    {
      printf("WARNING: boot descriptor version mismatch between bootloader and Octeon executive.\n");
      printf("WARNING: unpredictable behaviour may result.\n");
    }

  octeon_cpu_clock_hz = app_desc_ptr->eclock_hz;
  debug_flags_base_addr = app_desc_ptr->debugger_flags_base_addr;

  /* Copy argv pointer array from app desc. block to local memory.  */
  int argc = 0;
  char **argv = NULL;
    {
      int i;
      if (app_desc_ptr->argc > 0)
        {
	  argc = app_desc_ptr->argc;
	  argv = (void *)argv_array;
	  for (i = 0; i < argc; i++)
	    {
#if (_MIPS_SZPTR == 32)
	      argv_array[i] = (void *)(uint64_t)(((int64_t)(1<<31)) 
						 | (app_desc_ptr->argv[i] 
						    & 0x7fffffff));
#else
	      argv_array[i] = (void *)(uint64_t)((1ULL << 63) 
						 | (app_desc_ptr->argv[i] 
						    & 0x7fffffff));
#endif
	    }
	}
    }

  __octeon_argc = argc;
  __octeon_argv = (uint64_t)argv;

  asm volatile ("sync");

  if (bootloader_flags & OCTEON_BL_FLAG_DEBUG)
    {
      debug_active = 1;
      /* Set CVMX_CIU_DINT to enter debug exception handler.  */
      __cvmx_write_csr (CVMX_CIU_DINT, 1 << cvmx_get_core_num ());
    }
}

/* Called from exit(). Informs debug stub about the program reached the end
   while debugging. Also calls __cvmx_app_exit if using simple exec 
   applications to do the cleanup before exiting.  */
void _exit (int _status)
{
  uint64_t core_num = cvmx_get_core_num ();
    
  if (debug_active)
    {
      /* Signal to debug stub that this core is done with the application */
      uint32_t *debug_flags_ptr = (uint32_t *)((1 << 31) | debug_flags_base_addr);
      printf ("Debug _exit reached!, core %lld\n", core_num);

      /* Set flag indicating that the core is exiting the program */
      debug_flags_ptr[core_num] |= DEBUG_FLAG_CORE_DONE;
      asm volatile ("sync":::"memory");

      /* Set CVMX_CIU_DINT to enter debug exception handler. No need to
	 disable pulsing MCD0 signal as CVMX_CIU_DINT applies to core_num.  */
      __cvmx_write_csr (CVMX_CIU_DINT, 1 << core_num);
    }

  asm volatile (
	"dla $25, __cvmx_app_exit \n" 
	"beq $25, $0, 2f \n"
	"nop \n"
	"jal $25 \n"
	"nop \n"
	"2: \n"
	"break");
}
