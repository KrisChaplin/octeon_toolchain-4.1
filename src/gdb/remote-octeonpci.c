/* Remote protocol for Octeon over PCI. 

   Copyright (C) 2005, 2006 Cavium Networks.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

#include "defs.h"
#include "gdbcore.h"
#include "gdbarch.h"
#include "inferior.h"
#include "target.h"
#include "value.h"
#include "command.h"
#include "gdb_string.h"
#include "exceptions.h"
#include "gdbcmd.h"
#include <sys/types.h>
#include "serial.h"
#include "remote-utils.h"
#include "symfile.h"
#include "regcache.h"
#include <time.h>
#include <ctype.h>
#include <signal.h>
#include "top.h"
#include "mips-tdep.h"

/* This is really bad, but we can't seem to agree that we need a place to
    get common files that will be GPLed */
#include "../../sdk/host/pci/octeon-pci.c"
#include "../../sdk/host/pci/octeon-pci-debug.c"

static struct target_ops    octeon_ops;                 /**< Represents this target to GDB */
static int                  octeon_is_open      = 0;    /**< Non-zero if the PCI connection is open */
static int                  octeon_coreid;    		/**< The current active core (0-15) */
static octeon_pci_debug_t   octeon_core_state;          /**< The register state of the current core */
static int                  octeon_stepmode     = 0;    /**< Non zero if all active cores should step */
static char *               mask_cores          = NULL; /**< String representation of the active core mask */
static int                  octeon_activecores  = 0x1;  /**< The cores that should be debugged */
static int                  octeon_corestate    = 0;    /**< Unused, just the show command is used */
static sighandler_t         octeon_old_signal;          /**< Used to store the current signal handler */
static char *               octeon_pci_bootcmd = NULL;  /**< Give commands to bootloader running on Octeon in PCI slot. */

/* Whether we need to detach on exit.  */
static int detach_on_exit;

/* Maximium number of hardware breakpoints/watchpoints allowed */
#define MAX_OCTEON_HW_BREAKPOINTS 4

/* Total number of cores in Octeon. */
#define MAX_CORES 16

/* Record hardware instruction breakpoints for each core. Max 4 per core. */
static int hwbp[MAX_CORES][MAX_OCTEON_HW_BREAKPOINTS];

/* We need a prototype for octeon_interrupt since it and octeon_interrupt_twice
   reference each other */
static void octpci_interrupt (int);
static void octeonpci_open (char *, int);
static void octeonpci_prepare_to_store (void);

/* Stop all the cores in the active mask.  */
static void 
octeonpci_stop (void)
{
  octeon_pci_debug_stop_cores (octeon_activecores);
}

/* The user typed ^C twice.  */
static void 
octpci_interrupt_twice (int signo)
{
  signal (signo, octeon_old_signal);
  target_terminal_ours ();

  if (query ("Interrupted while waiting for the program.\n"
               "Give up (and stop debugging it)? "))
    {
      target_mourn_inferior ();
      deprecated_throw_reason (RETURN_QUIT);
    }

  target_terminal_inferior ();
  signal (signo, octpci_interrupt);
}

/* The command line interface's stop routine. This function is installed
   as a signal handler for SIGINT. The first time a user requests a
   stop, we call remote_stop to send a break or ^C. If there is no
   response from the target (it didn't stop when the user requested it),
   we ask the user if he'd like to detach from the target.  */
static void 
octpci_interrupt (int signo)
{
  signal (signo, octpci_interrupt_twice);
  octeonpci_stop ();
}

/* Convert the active cores bitmask into a human readable string.  */
static void 
convert_active_cores_to_string (void)
{
  char output[80];
  int i;
  int loc;

  /* Convert the bitmask into a comma seperated core list */
  loc = 0;
  for (i=0; i<16; i++)
    {
      if (octeon_activecores & (1<<i))
	loc += sprintf (output + loc, "%d,", i);
    }

  /* Remove the ending comma */
  if (loc)
    output[loc-1] = 0;

  /* Only update GDB's variable if the value changed */
  if ((mask_cores == NULL) || (strcmp (mask_cores, output) != 0))
    {
      if (mask_cores)
	xfree (mask_cores);
      mask_cores = savestring (output, strlen (output));
    }
}

/* This function should be called whenever the focus core changes.  */
static void 
update_focus (void)
{
  char prompt[32];

  /* Don't set focus to a core in reset */
  if (octeon_pci_read_csr (CVMX_CIU_PP_RST) & (1 << octeon_coreid))
    {
      octeon_coreid = 0;
      error("Core is in reset. It can't become the focus core");
    }

  /* Make sure the new core is in the active mask */
  if ((octeon_activecores & (1 << octeon_coreid)) == 0)
    {
      octeon_activecores |= 1 << octeon_coreid;
      convert_active_cores_to_string ();
    }

  /* Update the local register state to match the new core */
  if (octeon_pci_debug_read_core (octeon_coreid, &octeon_core_state))
    {
      octeon_pci_debug_stop_cores (1 << octeon_coreid);
      if (octeon_pci_debug_read_core (octeon_coreid, &octeon_core_state))
	error("Failed to read core state");
    }

  sprintf(prompt, "(Core#%d-gdb) ", octeon_coreid);
  if (strcmp (prompt, get_prompt ()) != 0)
    set_prompt (prompt);
}

/* Set/reset hardware instruction breakpoints. INDEX of hardware instruction
   breakpoint (max 4). ADDR is address to insert hardware breakpoints.
   ENA_BKT if set enable hardware breakpoint otherwise disable. CORE to
   insert hardware breakpoint.  */
static void 
initialize_hw_ibp (int index, CORE_ADDR addr, int ena_bkt, int core)
{
  if (core != octeon_coreid)
    {
      octeon_pci_debug_t state;
      octeon_pci_debug_read_core (core, &state);
      state.hw_ibp_address[index] = addr;
      state.hw_ibp_address_mask[index] = 0;
      state.hw_ibp_asid[index] = 0;
      state.hw_ibp_control[index] = (ena_bkt ? 5 : 0);
      octeon_pci_debug_write_core (core, &state);
    }	
  else
    {
      octeon_core_state.hw_ibp_address[index] = addr;
      octeon_core_state.hw_ibp_address_mask[index] = 0;
      octeon_core_state.hw_ibp_asid[index] = 0;
      octeon_core_state.hw_ibp_control[index] = (ena_bkt ? 5 : 0);
      octeon_pci_debug_write_core (octeon_coreid, &octeon_core_state);
    }
}

/* Wait until the remote machine stops, then return, storing the status in
   STATUS just as 'wait' would.  */
static ptid_t 
octeonpci_wait (ptid_t ptid, struct target_waitstatus *status)
{
  int core;
  status->kind = TARGET_WAITKIND_STOPPED;
  status->value.sig = TARGET_SIGNAL_TRAP;

  octeon_old_signal = signal (SIGINT, octpci_interrupt);

  while (1)
    {
      uint64_t core_control 
		= octeon_pci_debug_read_register (octeon_coreid, 
				   		  offsetof (octeon_pci_debug_t,
						            core_control));
      if (core_control != 0)
        break;
      usleep (100000);
    }
  signal (SIGINT, octeon_old_signal);

  for (core = 0; core < 16; core++)
    {
      octeon_pci_debug_read_core (core, &octeon_core_state);
      /* If dib bit in Debug register is set, then the core stopped
	 from one of the hardware instruction breakpoint */
      if (octeon_core_state.cop0_debug & 0x10)
	{
	  /* Clear the bit to continue.  */
	  octeon_core_state.cop0_debug &= ~0x10ull;
	  /* Also clear hardware status register.  */
	  octeon_core_state.hw_ibp_status &= ~0xf;
	  octeon_pci_debug_write_core (core, &octeon_core_state);
	  octeon_coreid = core;
	  update_focus ();
	  break;
	}
     }

  octeon_pci_debug_read_core (octeon_coreid, &octeon_core_state);

  /* FIXME: Determine difference between stop and program complete */
  if (1)
    {
      status->value.sig = TARGET_SIGNAL_TRAP;
    }
  else
    {
      /* Program terminated */
      status->kind = TARGET_WAITKIND_EXITED;
      status->value.sig = TARGET_SIGNAL_TRAP;
      status->value.integer = 0;
    }
  return inferior_ptid;
}

/* Tell the remote machine to resume.  */
static void 
octeonpci_resume (ptid_t ptid, int step, enum target_signal sigal)
{
  /* Stop the Count register when GSTOP is set */
  octeon_core_state.cop0_multicoredebug |= 0x1ull << 15;

  /* Set or clear the single step bit */
  if (step)
    octeon_core_state.cop0_debug |= 0x100ull;
  else
    octeon_core_state.cop0_debug &= ~0x100ull;

  if (octeon_pci_debug_write_core (octeon_coreid, &octeon_core_state))
    error("Failed to write core state");

  if (octeon_stepmode)
    {
      int core;
      octeon_pci_debug_t state;

      /* This is painful, We need to go through each core setting the step and
	 MCD0 bits */
      for (core=0; core < 16; core++)
	{
	  octeon_pci_debug_read_core (core, &state);
	  /* Stop the Count register when GSTOP is set */
	  state.cop0_multicoredebug |= 0x1ull << 15;

	  /* COP0_MDEBUG_REG |= GSDBBP | Enable MCD0 | Clear MCD0 */
	  if (octeon_activecores & (1 << core))
	    state.cop0_multicoredebug |= 0x1100ull;
	  else
	    state.cop0_multicoredebug &= ~0x1100ull;

	  /* Clear the single step bit */
	  if (core != octeon_coreid)
	    state.cop0_debug &= ~0x100ull;

	  octeon_pci_debug_write_core (core, &state);
        }
      octeon_pci_debug_start_cores (octeon_activecores);
    }
  else
    octeon_pci_debug_start_cores (1 << octeon_coreid);
}

/* Fetch the remote registers.  */
static void 
octeonpci_fetch_registers (int regno)
{
  if (regno < 38)
    write_register (regno, octeon_core_state.regs[regno]);
  else
    write_register (regno, 0);
}


/* Store all registers into remote target.  */
static void 
octeonpci_store_registers (int regno)
{
  /* This fix is required for "break func" followed by "run" command to work
     if focus is set before. */
  if ((regno == MIPS_EMBED_PC_REGNUM) && exec_bfd)
    if (bfd_get_start_address (exec_bfd) == read_register (MIPS_EMBED_PC_REGNUM))
      return;

  if (regno < 38)
    octeon_core_state.regs[regno] = read_register (regno);
}


/* Get ready to modify the registers array.  On machines which store
   individual registers, this doesn't need to do anything.  On machines
   which store all the registers in one fell swoop, this makes sure
   that registers contains all the registers from the program being
   debugged.  */
static void 
octeonpci_prepare_to_store (void)
{
  /* Do nothing, since we can store individual regs */
}


/* Read and write memory of and from target respectively.  */
static int 
octeonpci_xfer_inferior_memory (CORE_ADDR memaddr, gdb_byte *myaddr, 
			        int len, int write, struct mem_attrib *attrib,
                                struct target_ops *target)
{
  uint64_t paddr = octeon_pci_debug_get_paddr (&octeon_core_state, memaddr);
  if (paddr == 0xffffffffffffffffull)
    error ("Unable to convert virtual address to physical");

  if (write)
    octeon_pci_write_mem (paddr, myaddr, len, OCTEON_PCI_ENDIAN_64BIT_SWAP);
  else
    octeon_pci_read_mem (myaddr, paddr, len, OCTEON_PCI_ENDIAN_64BIT_SWAP);

  return len;
}


/* Terminate the open connection to the remote debugger.  Use this
   when you want to detach and do something else with your gdb.  */

static void 
octeonpci_detach (char *arg, int from_tty)
{
  octeon_pci_debug_start_cores (octeon_activecores);
  detach_on_exit = 0;
  pop_target ();
}

/* As long as target is in effect we are attached.  */

static void 
octeonpci_attach (char *arg, int from_tty)
{
  /* ??? We should not even install this hook.  */
}

/* Print info on this target.  */
static void 
octeonpci_files_info (struct target_ops *ignore)
{
  printf_unfiltered ("Debugging a MIPS64 OCTEON board over PCI.\n");
}


static void 
octeonpci_kill (void)
{
  inferior_ptid = null_ptid;
}


/* At present the bootloader is loading the program into memory and executing 
   it so no need to do anything.  */
static void 
octeonpci_load (char *args, int from_tty)
{
  error ("octeonpci_load not implemented");
}

/* Clean up when a program exits. The program actually lives on in the remote 
   processor's RAM, and may be run again without a download. Don't leave it 
   full of breakpoint instructions.  */
static void 
octeonpci_mourn_inferior (void)
{
  remove_breakpoints ();
  unpush_target (&octeon_ops);
  /* Do all the proper things now.  */ 
  generic_mourn_inferior ();
}

/* CNT is the number of hardware breakpoint to be installed. Return non-zero
   value if the value does not cross the max limit (4 for Octeon).  */
static int 
octeonpci_check_watch_resources (int type, int cnt, int ot)
{
  int i;

  if (type != bp_hardware_breakpoint)
    return -1;

  if (cnt > MAX_OCTEON_HW_BREAKPOINTS)
    {
      printf_unfiltered ("Octeon supports only four hardware breakpoints per core\n");
      return -1;
    }

  return 1;
}


/* Insert hardware breakpoints. ADDR is the address to insert hardware
   breakpoints.  */
static int 
octeonpci_insert_mc_hw_breakpoint (struct bp_target_info *bp_tgt, int core)
{
  int i;
   
  for (i = 0; i < MAX_OCTEON_HW_BREAKPOINTS; i++)
    {
      if (hwbp[core][i] == 0) 
	{
	  hwbp[core][i] = bp_tgt->placed_address;
	  initialize_hw_ibp (i, bp_tgt->placed_address, 1, core);
	  return 0;
	}
    }
  return -1;
}

/* Remove hardware breakpoints. ADDR is the address to delete hardware
   breakpoints from.  */
static int 
octeonpci_remove_mc_hw_breakpoint (struct bp_target_info *bp_tgt, int core)
{
  int i;
   
  for (i = 0; i < MAX_OCTEON_HW_BREAKPOINTS; i++)
    {
      if (hwbp[core][i] == bp_tgt->placed_address) 
	{
	  initialize_hw_ibp (i, bp_tgt->placed_address, 0, core);
	  hwbp[core][i] = 0;
	  return 0;
	}
    }

  return -1;
}

/* Return the current core number.  */
static int
octeonpci_get_core_number ()
{
  return octeon_coreid;
}

/* Return 1 if debugger supports hardware breakpoint in multicore debugging.  */
static int
octeonpci_multicore_hw_breakpoint ()
{
  return 1;
}

/* Get the number of the core to be debugged.  */
static void 
process_core_command (char *args, int from_tty, struct cmd_list_element *c)
{
  update_focus ();
  registers_changed ();
  normal_stop ();
}

/* Get the number of cores to stop on debug exception.  */
static void 
process_mask_command (char *args, int from_tty, struct cmd_list_element *c)
{
  char buf[64];
  int coreid;
  char *ptr;
  char mcores[strlen (mask_cores) + 1];

  octeon_activecores = 0;
  strcpy (mcores, mask_cores);
  if (mcores[0])
    {
      ptr = strtok (mcores, ",");
      while (ptr)
	{
	  coreid = atoi (ptr);
	  octeon_activecores |= (1 << coreid);
	  ptr = strtok (NULL, ",");
	}
    }
  else
    /* A blank active-cores string defaults to all cores */
    octeon_activecores = 0xffff;

  /* Rebuild the active-cores string to make sure it matches what we parsed */
  convert_active_cores_to_string ();
}

/* Display detailed core state.  */
static void 
process_corestate_command (struct ui_file *file, int from_tty,
			   struct cmd_list_element *c, const char *value)
{
  octeon_pci_debug_display_core (&octeon_core_state);
}

/* Load the executable EXECFILE and boot it using
   BOOTLOADER_COMMAND.  */

static void
pci_load_and_boot (char *execfile, char *bootloader_command)
{
  char **argv, **arg;
  unsigned long long address = 0x21000000ULL;
  int seen_debug = 1;

  /* Parse the args for address to load the program at.  */
  if ((argv = buildargv (bootloader_command)) == NULL)
    nomem (0);

  if (argv[1])
    address = strtoul (argv[1], NULL, 16);

  /* If debug was not supplied on the command-line add it.  */
  for (arg = argv; *arg; arg++)
    {
      seen_debug = strncmp (*arg, "debug", 5) == 0;
      if (seen_debug || strcmp (*arg, "endbootarg") == 0)
	break;
    }
  if (!seen_debug)
    {
      char *p = xmalloc (strlen (bootloader_command) + sizeof (" debug"));
      char *e = strstr (bootloader_command, " endbootarg");
      size_t s = e ? e - bootloader_command : strlen (bootloader_command);

      strncpy (p, bootloader_command, s);
      p[s] = '\0';
      bootloader_command = strcat (p, " debug");
      if (e)
	strcat (bootloader_command, e);
    }

  /* If an option other than oct-pci-reset is set through "set pci-bootcmd" 
     then invoke the command before starting the program. For more details 
     look Bug #372. */
  if (octeon_pci_bootcmd[0] != '\0' && system (octeon_pci_bootcmd) != 0)
    error ("Executing %s failed\n", octeon_pci_bootcmd);

  /* Install the debug handler. It was removed after the reset */
  if (octeon_pci_debug_setup (OCTEON_PCI_DEBUG_HANDLER_PCI))
    error ("Unable to install debug exception handler");

  /* Copy the executable into memory */
  int handle = open (execfile, O_RDONLY);
  if (handle < 0)
    error ("Unable to open file %s\n", execfile);

  uint64_t file_size = lseek (handle, 0, SEEK_END);
  char *ptr = (char*) mmap (NULL, file_size, PROT_READ, MAP_SHARED, handle, 0);
  close (handle);

  if (ptr == NULL)
    error ("Unable to memory map file %s\n", execfile);

  octeon_pci_write_mem (address, ptr, file_size, OCTEON_PCI_ENDIAN_64BIT_SWAP);
  munmap (ptr, file_size);

  printf_unfiltered ("\nSending bootloader command: %s\n\n",
		     bootloader_command);
  if (octeon_pci_send_bootcmd (bootloader_command, 100))
    error ("Sending bootcmd failed");
}

static void
add_commands ()
{
  add_setshow_zinteger_cmd ("focus", no_class, &octeon_coreid, _("\
Set the core to be debugged, should be (0-15).\n"), _("\
Show the focus core of debugger operations.\n"), 
                            NULL, process_core_command, NULL, 
			    &setlist, &showlist);

  add_setshow_boolean_cmd ("step-all", no_class, &octeon_stepmode, _("\
Set if \"step\"/\"next\"/\"continue\" commands should be applied to all the cores.\n"), _("\
Show step commands affect all cores.\n"), NULL,
			   NULL, NULL, 
			   &setlist, &showlist);

  add_setshow_string_cmd ("active-cores", no_class, &mask_cores, _("\
Set the cores stopped on execution of a breakpoint by another core.\n"), _("\
Show the cores stopped on execution of a breakpoint by another core.\n"),
			  NULL, process_mask_command, NULL,
			  &setlist, &showlist);

  add_setshow_boolean_cmd ("core-state", no_class, &octeon_corestate, _("\
Set currently does nothing.\n"), _("\
Show detailed focus core state.\n"),
			   NULL, NULL, process_corestate_command,
			   &setlist, &showlist);
}

static void
remove_commands ()
{
  delete_cmd ("focus", &setlist);
  delete_cmd ("focus", &showlist);

  delete_cmd ("step-all", &setlist);
  delete_cmd ("step-all", &showlist);

  delete_cmd ("active-cores", &setlist);
  delete_cmd ("active-cores", &showlist);

  delete_cmd ("core-state", &setlist);
  delete_cmd ("core-state", &showlist);
}

/* Open a connection over PCI to Octeon. Also adds new commands needed for 
   debugging Octeon. They are added here so the other Octeon target can have 
   a seperate set. ARGS is the arguments that were passed to the
   target command after our target name.  FROM_TTY is nonzero if GDB is
   interactive.  */

static void 
octeonpci_open (char *args, int from_tty)
{
  /* Close the current target early.  */
  target_preopen (from_tty);

  if (octeon_pci_open (0))
    error ("Unable to open PCI connection to Octeon");

  if (octeon_pci_debug_setup (OCTEON_PCI_DEBUG_HANDLER_PCI))
    error ("Unable to install debug exception handler");

  if (octeon_pci_bootcmd == NULL)
    octeon_pci_bootcmd = strdup ("oct-pci-reset");

  /* We have two modes.  If we are passed anything we treat it as a
     boot command and boot.  We need to boot here because after the
     target command we should be able to display memory.  Otherwise we
     attach to the existing program running on the target.  */
  if (args)
    {
      struct target_waitstatus status;
      char *execfile;

      execfile = get_exec_file (0);
      if (!execfile)
	error ("No executable file specified");

      pci_load_and_boot (execfile, args);

      detach_on_exit = 0;
    }
  else
    {
      octeon_pci_debug_stop_cores (octeon_activecores);
      if (octeon_pci_debug_read_core (octeon_coreid, &octeon_core_state) == -1)
	error ("Failed to stop cores");

      detach_on_exit = 1;
    }

  /* Only add ourselves if we can communicate with the target.  We
     need to do this before start_remote as it wants call back into
     the target.  */
  push_target (&octeon_ops);
  setup_generic_remote_run (args, pid_to_ptid (100));

  /* Now do the wait and then update internal state like
     current_frame.  Unlike remote.c in both modes we should be
     stopping in the debug exception handler so we should not fail
     here.  */
  start_remote ();

  add_commands ();
  /* Set the initial prompt.  */
  set_prompt ("(Core#0-gdb) ");
}

/* Close the connection to OcteonPCI. This also needs to unregister all 
   OcteonPCI specific commands. QUITTING is true if GDB is exiting.  */

static void 
octeonpci_close (int quitting)
{
  if (detach_on_exit)
    octeon_pci_debug_start_cores (octeon_activecores);
  detach_on_exit = 0;

  octeon_pci_close ();

  remove_commands ();
}

/* Forward it to remote-run.  */

static int 
octeonpci_can_run (void)
{
  return generic_remote_can_run_target (octeon_ops.to_shortname);
}

/* Initialize the OcteonPCI target. This is the main entry point for this file.
   The function name must be on a seperate line from the type so the init.c 
   script can find it.  */
void
_initialize_octeonpci (void)
{
  convert_active_cores_to_string ();

  /* Since we boot in the target command we need to be able to set
     pci-bootcmd before that so this has to be global.  */
  add_setshow_string_cmd ("pci-bootcmd", no_class, &octeon_pci_bootcmd, _("\
Set boot command (shell command) needed for PCI boot as debugger resets the \n\
board internally on a \"run\" command. Default is oct-pci-reset\n"), _("\
Show boot command for PCI boot\n"),
			  NULL, NULL, NULL,
			  &setlist, &showlist);

  octeon_ops.to_shortname = "octeonpci";
  octeon_ops.to_longname = "Remote connection to Octeon over PCI";
  octeon_ops.to_doc = "Connect to Octeon through PCI. No parameters are necessary.\n"
                      "Example: target octeonpci";
  octeon_ops.to_open = octeonpci_open;
  octeon_ops.to_close = octeonpci_close;
  octeon_ops.to_attach = octeonpci_attach;
  octeon_ops.to_detach = octeonpci_detach;
  octeon_ops.to_resume = octeonpci_resume;
  octeon_ops.to_wait = octeonpci_wait;
  octeon_ops.to_fetch_registers = octeonpci_fetch_registers;
  octeon_ops.to_store_registers = octeonpci_store_registers;
  octeon_ops.to_prepare_to_store = octeonpci_prepare_to_store;
  octeon_ops.deprecated_xfer_memory = octeonpci_xfer_inferior_memory;
  octeon_ops.to_files_info = octeonpci_files_info;
  octeon_ops.to_insert_breakpoint = memory_insert_breakpoint;
  octeon_ops.to_remove_breakpoint = memory_remove_breakpoint;
  octeon_ops.to_can_use_hw_breakpoint = octeonpci_check_watch_resources;
  octeon_ops.to_insert_mc_hw_breakpoint = octeonpci_insert_mc_hw_breakpoint;
  octeon_ops.to_remove_mc_hw_breakpoint = octeonpci_remove_mc_hw_breakpoint;
  octeon_ops.to_multicore_hw_breakpoint = octeonpci_multicore_hw_breakpoint;
  octeon_ops.to_get_core_number = octeonpci_get_core_number;
  octeon_ops.to_kill = octeonpci_kill;
  octeon_ops.to_load = octeonpci_load;
  octeon_ops.to_create_inferior = generic_remote_create_inferior;
  octeon_ops.to_can_run = octeonpci_can_run;
  octeon_ops.to_mourn_inferior = octeonpci_mourn_inferior;
  octeon_ops.to_stop = octeonpci_stop;
  octeon_ops.to_stratum = process_stratum;
  octeon_ops.to_has_all_memory = 1;
  octeon_ops.to_has_memory = 1;
  octeon_ops.to_has_stack = 1;
  octeon_ops.to_has_registers = 1;
  octeon_ops.to_has_execution = 1;
  octeon_ops.to_magic = OPS_MAGIC;
  add_target (&octeon_ops);
}
