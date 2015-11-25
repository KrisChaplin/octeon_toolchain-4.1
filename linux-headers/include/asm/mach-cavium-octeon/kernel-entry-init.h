/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2005 Cavium Networks, Inc
 */
#ifndef __ASM_MACH_GENERIC_KERNEL_ENTRY_H
#define __ASM_MACH_GENERIC_KERNEL_ENTRY_H


#define CP0_CVMMEMCTL_REG $11,7
#define CP0_CVMCTL_REG $9,7
#define COP0_MDEBUG_REG $22,0

.macro  kernel_entry_setup
    # Registers set by bootloader:
    # (only 32 bits set by bootloader, all addresses are physical addresses, and need
    #   to have the appropriate memory region set by the kernel
    # a0 = argc
    # a1 = argv (kseg0 compat addr )
    # a2 = 1 if init core, zero otherwise
    # a3 = address of boot descriptor block

    dmfc0   v0, CP0_CVMMEMCTL_REG       # Read the memory control register
    or  v0, v0, 0x1c0                   # Make sure access to CVMSEG is allowed in all modes
    or  v0, v0, 0x3f                    # Disable all local memory - cache is max size
    xor v0, v0, 0x3f
    dmtc0   v0, CP0_CVMMEMCTL_REG       # Write the memory control register
    dmfc0   v0, CP0_CVMCTL_REG          # Read the cavium control register
#ifdef CONFIG_CAVIUM_OCTEON_HW_FIX_UNALIGNED
    or  v0, v0, 0x1001                  # Disable unaligned load/store support but leave HW fixup enabled
    xor v0, v0, 0x1001
#else
    or  v0, v0, 0x5001                  # Disable unaligned load/store and HW fixup support
    xor v0, v0, 0x5001
#endif
    or  v0, v0, 0x2000                  # Disable instruction prefetching (Octeon Pass1 errata)
    dmtc0   v0, CP0_CVMCTL_REG          # Write the cavium control register
    sync
    cache   9, 0($0)                    # Flush dcache after config change

#ifdef CONFIG_SMP
    PTR_LA  t2, octeon_boot_desc_ptr;
    LONG_S  a3, (t2)

    rdhwr   v0, $0                      # Get my core id

    bne     a2, zero, octeon_main_processor # Jump the master to kernel_entry
    nop
    #
    # All cores other than the master need to wait here for SMP bootstrap
    # to begin
    #
    PTR_LA  t0, octeon_processor_boot   # This is the variable where the next core to boot os stored
octeon_spin_wait_boot:
    LONG_L  t1, (t0)                    # Get the core id of the next to be booted
    bne t1, v0, octeon_spin_wait_boot   # Keep looping if it isn't me
    nop
    PTR_LA  t0, octeon_processor_gp     # Get my GP from the global variable
    LONG_L  gp, (t0)
    PTR_LA  t0, octeon_processor_sp     # Get my SP from the global variable
    LONG_L  sp, (t0)
    LONG_L  zero, (t0)                  # Set the SP global variable to zero so the master knows we've started
    sync
    b   smp_bootstrap                   # Jump to the normal Linux SMP entry point
    nop
octeon_main_processor:
#endif
.endm

/*
 * Do SMP slave processor setup necessary before we can savely execute C code.
 */
    .macro  smp_slave_setup
    .endm


#endif /* __ASM_MACH_GENERIC_KERNEL_ENTRY_H */
