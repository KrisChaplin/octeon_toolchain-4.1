/* Define the struct that is initialized by the bootloader used by the 
 * startup code.
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

/* Structures used to pass information from the bootloader to the application.
   This should not be used by the application directly.  */

#ifndef __OCTEON_APP_INIT_H__
#define __OCTEON_APP_INIT_H__

/* Define to allow conditional compilation when new items are added */
#define OCTEON_APP_INIT_H_VERSION   1

/* Macro indicates that bootmem related structures are now in
   cvmx-bootmem.h.  */
#define OCTEON_APP_INIT_BOOTMEM_STRUCTS_MOVED

typedef enum
{
  /* If set, core should do app-wide init, only one core per app will have 
     this flag set.  */ 
  BOOT_FLAG_INIT_CORE     = 1,  
  OCTEON_BL_FLAG_DEBUG    = 1 << 1,
  OCTEON_BL_FLAG_NO_MAGIC = 1 << 2,
  OCTEON_BL_FLAG_CONSOLE_UART1 = 1 << 3,  /* If set, use uart1 for console */
} octeon_boot_descriptor_flag_t;

#define OCTEON_CURRENT_DESC_VERSION     6
#define OCTEON_ARGV_MAX_ARGS            (64)

#define OCTOEN_SERIAL_LEN 20

/* Bootloader structure used to pass info to Octeon executive startup code.
   NOTE: all fields are deprecated except for:
   * desc_version
   * desc_size,
   * heap_base
   * heap_end
   * exception_base_addr
   * flags
   * argc
   * argv
   * desc_size
   * cvmx_desc_vaddr
   * debugger_flags_base_addr

   All other fields have been moved to the cvmx_descriptor, and the new 
   fields should be added there. They are left as placeholders in this 
   structure for binary compatibility.  */
typedef struct
{   
  /* Start of block referenced by assembly code - do not change! */
  uint32_t desc_version;
  uint32_t desc_size;
  uint64_t stack_top;
  uint64_t heap_base;
  uint64_t heap_end;
  /* Only used by bootloader */
  uint64_t entry_point;   
  uint64_t desc_vaddr;
  /* End of This block referenced by assembly code - do not change! */
  uint32_t exception_base_addr;
  uint32_t stack_size;
  uint32_t heap_size;
  /* Argc count for application. */
  uint32_t argc;  
  uint32_t argv[OCTEON_ARGV_MAX_ARGS];
  uint32_t flags;
  uint32_t core_mask;
  /* DRAM size in megabyes. */
  uint32_t dram_size;  
  /* physical address of free memory descriptor block. */
  uint32_t phy_mem_desc_addr;  
  /* used to pass flags from app to debugger. */
  uint32_t debugger_flags_base_addr;  
  /* CPU clock speed, in hz. */
  uint32_t eclock_hz;  
  /* DRAM clock speed, in hz. */
  uint32_t dclock_hz;  
  /* SPI4 clock in hz. */
  uint32_t spi_clock_hz;  
  uint16_t board_type;
  uint8_t board_rev_major;
  uint8_t board_rev_minor;
  uint16_t chip_type;
  uint8_t chip_rev_major;
  uint8_t chip_rev_minor;
  char board_serial_number[OCTOEN_SERIAL_LEN];
  uint8_t mac_addr_base[6];
  uint8_t mac_addr_count;
  uint64_t cvmx_desc_vaddr;
} octeon_boot_descriptor_t;

/* Debug flags bit definitions.  */
#define DEBUG_FLAG_CORE_DONE    0x1

#endif /* __OCTEON_APP_INIT_H__ */
