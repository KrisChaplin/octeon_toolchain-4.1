/* Remote protocol for Octeon Simple Executive cross debugger. 

   Copyright (C) 2004, 2005, 2006 Cavium Networks.

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
//#include <sys/wait.h>
#include "gdb_assert.h"

extern pid_t wait(int);
/* Define the target subroutine names. */
struct target_ops octeon_ops;

/* Local function declarations */
static void close_connection ();
static void create_connection ();
static void simulator_fork (char **);
static int gets_octeondebug (char *);
static void octeon_close (int);
static void octeon_interrupt (int);
static void octeon_interrupt_twice (int);
static void process_core_command (char *, int, struct cmd_list_element *c);
static void process_mask_command (char *, int, struct cmd_list_element *c);
static void process_stepmode_command (char *, int, struct cmd_list_element *c);
static void octeon_stop (void);
static void update_core_mask (void);
static void update_focus (void);
static void update_step_all (void);
static int puts_octeondebug (char *);
static int readchar (int);
static int octeon_change_focus (int);
static int send_command_get_int_reply_generic (char *, char *, int);
static void set_performance_counter0_event (char *, int, 
					    struct cmd_list_element *c);
static void set_performance_counter1_event (char *, int, 
					    struct cmd_list_element *c);
static void show_performance_counter0_event_and_counter (struct ui_file *, int, 
					                 struct cmd_list_element *,
							 const char *);
static void show_performance_counter1_event_and_counter (struct ui_file *, int,
							 struct cmd_list_element *,
							 const char *);

/* The Octeon core number. */
static int octeon_coreid;
/* Controls stepping, single core or all the cores. */
static int octeon_stepmode = 0;
/* The string of cores separated by comma set for debugging. For ex.: 0,3,4.  */
static char *mask_cores = NULL;
/* The mask of the cores that are enabled for debugging.  */
static int octeon_activecores = 0;
/* Controls spawning the simulator. */
static int octeon_spawn_sim = 1;
/* Performance counters event.  */
static char *perf_event[2] = {0, 0};

static struct serial *octeon_desc;
/* Initially load 128 bytes of memory from the debug stub. For the subsequent
   memory read operation, if the memory address is within this 128 bytes then
   return the value from cache_mem_data instead of asking from debug stub.
   Sending packets to and fro from debug stub slows down a lot.  */
static CORE_ADDR cache_mem_addr = 0;
static char cache_mem_data[128];

/* Holds the mask of the coreid that has finished executing. Any request 
   after the core has finished executing will not be executed.  */
static int end_status = 0;

/* Simulator process id that gets spawned.  */
static int sim_pid = 0;
/* Options passed to spawn the simulator.  */
char **sim_options;
/* Set to 1 if debugging over tcp using simulator.  */
static int is_simulator = 0;
/* We cannot exit the simulator on kill because GDB assumes it can still 
   communicate with the target.  kill sets PENDING_KILL and we actually kill 
   the connection in create_inferior just before we would reestablish the 
   connection.  */
static int pending_kill = 0;
/* Serial port device, tcp or serial port number.  */
char *serial_port_name;

/* The address of the hardware watchpoint that caused the debugger to stop.
   Initialized in octeon_wait when the hardware watchpoint is hit and used
   later by octeon_stopped_data_address.  */
CORE_ADDR last_wp_addr;

/* Version number of the debug stub.  */
static int stub_version;

enum stub_feature { STUB_PERF_EVENT_TYPE };

/* The max size of buffer that is used for sending packet to the debug
   stub.  */
#define PBUFSIZ 512

#define DEFAULT_PROMPT "(Core#0-gdb) "

/* Total number of cores in Octeon. */
#define MAX_CORES 16
/* Maximium number of instruction hardware breakpoints Octeon has. */
#define MAX_OCTEON_INST_BREAKPOINTS 4

/* Record hardware instruction breakpoints for each core. Max 4 per core. */
static int hwbp[MAX_CORES][MAX_OCTEON_INST_BREAKPOINTS];

/* Record hardware data breakpoints (watchpoints). Max 4 per debugging 
   section.  */
static int hwwp[MAX_OCTEON_INST_BREAKPOINTS];

/* Record performance counters event per core. */
static int perf_status_t[MAX_CORES][2];

#define MAX_NO_PERF_EVENTS (sizeof (perf_events_t) / sizeof (perf_events_t[0]))
                                                                                
/* Enumeration of all supported performance counter types.  */
static struct
{
  const char *event;
  const char *help;
} perf_events_t [] = {
    {"none",            "Turn off the performance counter"},
    {"clk",             "Conditionally clocked cycles"},
    {"issue",           "Instructions issued but not retired"},
    {"ret",             "Instructions retired"},
    {"nissue",          "Cycles no issue"},
    {"sissue",          "Cycles single issue"},
    {"dissue",          "Cycles dual issue"},
    {"ifi",             "Cycle ifetch issued"},
    {"br",              "Branches retired"},
    {"brmis",           "Branch mispredicts"},
    {"j",              "Jumps retired"},
    {"jmis",           "Jumps mispredicted"},
    {"replay",         "Mem Replays"},
    {"iuna",           "Cycles idle due to unaligned_replays"},
    {"trap",           "trap_6a signal"},
    {NULL,             NULL},
    {"uuload",         "Unexpected unaligned loads (REPUN=1)"},
    {"uustore",        "Unexpected unaligned store (REPUN=1)"},
    {"uload",          "Unaligned loads (REPUN=1 or USEUN=1)"},
    {"ustore",         "Unaligned store (REPUN=1 or USEUN=1)"},
    {"ec",             "Exec clocks"},
    {"mc",             "Mul clocks"},
    {"cc",             "Crypto clocks"},
    {"csrc",           "Issue_csr clocks"},
    {"cfetch",         "Icache committed fetches (demand+prefetch)"},
    {"cpref",          "Icache committed prefetches"},
    {"ica",            "Icache aliases"},
    {"ii",             "Icache invalidates"},
    {"ip",             "Icache parity error"},
    {"cimiss",         "Cycles idle due to imiss"},
    {NULL,	       NULL},
    {NULL,	       NULL},
    {"wbuf",           "Number of write buffer entries created"},
    {"wdat",           "Number of write buffer data cycles used"},
    {"wbufld",         "Number of write buffer entries forced out by loads"}, 
    {"wbuffl",         "Number of cycles that there was no available write buffer entry"},
    {"wbuftr",         "Number of stores that found no available write buffer entries"},
    {"badd",           "Number of address bus cycles used"},
    {"baddl2",         "Number of address bus cycles not reflected (i.e. destined for L2)"},
    {"bfill",          "Number of fill bus cycles used"},
    {"ddids",          "Number of Dstream DIDs created"},
    {"idids",          "Number of Istream DIDs created"},
    {"didna",          "Number of cycles that no DIDs were available"},
    {"lds",            "Number of load issues"},
    {"lmlds",          "Number of local memory load"},
    {"iolds",          "Number of I/O load issues"},
    {"dmlds",          "Number of loads that were not prefetches and missed in the cache"},
    {NULL,	       NULL},
    {"sts",            "Number of store issues"},
    {"lmsts",          "Number of local memory store issues"},
    {"iosts",          "Number of I/O store issues"},
    {"iobdma",         "Number of IOBDMAs"},
    {NULL,	       NULL},
    {"dtlb",           "Number of dstream TLB refill, invalid, or modified exceptions"},
    {"dtlbad",         "Number of dstream TLB address errors"},
    {"itlb",           "Number of istream TLB refill, invalid, or address error exceptions"},
    {"sync",           "Number of SYNC stall cycles"},
    {"synciob",        "Number of SYNCIOBDMA stall cycles"},
    {"syncw",          "Number of SYNCWs"}
};

/* Convert number NIB to a hex digit.  */
static int
tohex (int nib)
{
  if (nib < 10)
    return '0' + nib;
  else
    return 'a' + nib - 10;
}

/* Convert hex digit A to a number.  */
static int
from_hex (int a)
{
  if (a == 0)
    return 0;

  if (a >= '0' && a <= '9')
    return a - '0';
  if (a >= 'a' && a <= 'f')
    return a - 'a' + 10;
  if (a >= 'A' && a <= 'F')
    return a - 'A' + 10;
  else if (remote_debug)
    error ("Reply contains invalid hex digit 0x%x", a);
  return 0;
}

/* Send a GDB packet to the target.  */
static int
puts_octeondebug (char *packet)
{
  if (!octeon_desc)
    error ("Use \"target octeon ...\" first.");

  if (remote_debug)
    printf_unfiltered ("Sending %s\n", packet);

  if (serial_write (octeon_desc, packet, strlen (packet)))
    fprintf_unfiltered (gdb_stderr, "Serial write failed: %s\n",
			safe_strerror (errno));

  return 1;
}

/* Make a GDB packet. The data is always ASCII.
   A debug packet whose contents are <data>
   is encapsulated for transmission in the form:
  
                $ <data> # CSUM1 CSUM2
  
   <data> must be ASCII alphanumeric and cannot include characters
   '$' or '#'.  If <data> starts with two characters followed by
   ':', then the existing stubs interpret this as a sequence number.
  
   CSUM1 and CSUM2 are ascii hex representation of an 8-bit
   checksum of <data>, the most significant nibble is sent first.
   the hex digits 0-9,a-f are used.  */
static void
make_gdb_packet (char *buf, char *data)
{
  int i;
  unsigned char csum = 0;
  int cnt;
  char *p;

  cnt = strlen (data);

  if (cnt > PBUFSIZ)
    error ("make_gdb_packet(): to much data\n");

  /* Start with the packet header */
  p = buf;
  *p++ = '$';

  /* Calculate the checksum */
  for (i = 0; i < cnt; i++)
    {
      csum += data[i];
      *p++ = data[i];
    }

  /* Terminate the data with a '#' */
  *p++ = '#';

  /* add the checksum as two ascii digits */
  *p++ = tohex ((csum >> 4) & 0xf);
  *p++ = tohex (csum & 0xf);
  *p = 0x0;			/* Null terminator on string */
}

/* Make a packet of the stream specified through FMT and other
   varargs and send it to the debug stub. */
static void
make_and_send_packet (char *fmt, ...)
{
  char buf[PBUFSIZ];
  char packet[PBUFSIZ];

  va_list args;
  va_start (args, fmt);
  (void) vsnprintf (buf, sizeof (buf), fmt, args);
  va_end (args);
  make_gdb_packet (packet, buf);
  if (puts_octeondebug (packet) == 0)
    error ("Couldn't transmit packet %s\n", packet);
}

/* Read data from target. */
static int
gets_octeondebug (char *packet)
{
  /* State of what we are expecting.  */
  enum
  { S_BEGIN, S_DATA, S_CSUM1, S_CSUM2 } state;
  /* Current input character.  */
  int c;
  /* Running chksum of the S_DATA section.  */
  unsigned char csum;
  /* Pointer to the current addr location in the packet.  */
  char *bp;
  /* True if the packet is invalid and needs to be displayed to the user.  */
  int do_display;

  state = S_BEGIN;
  csum = 0;
  do_display = 0;
  bp = packet;

  /* The only way out of this loop is for a valid packet to show up or a
     signal to occur (Control-C twice). Any input received that isn't a valid
     packet will be displayed to the user. This allows the debugger to use the
     same uart as the console. There is a small chance the program will output
     a valid debugger command, but this is unlikely. It has to also get the
     checksum correct. */
  while (1)
    {
      /* Read a character and add it to the packet buffer. A timeout will be
         cause a display since the case statement below doesn't accept it */
      c = readchar (remote_timeout);
      if (c != SERIAL_TIMEOUT)
	*bp++ = c;
      /* Based on the expected character state, determine what we will accept 
       */
      switch (state)
	{
	  /* In the beginning we will only accept a $. All other characters
	     will be displayed to the user */
	case S_BEGIN:
	  if (c == '$')
	    {
	      state = S_DATA;
	      /* Backup bp by one since the $ isn't suppose to be in the
	         final packet */
	      bp--;
	    }
	  else
	    do_display = 1;
	  break;
	  /* Once we've received the $, we expect data for the packet. Right
	     now data can only contain letters and numbers. A # signals the
	     end of the data section */
	case S_DATA:
	  if (c == '#')
	    state = S_CSUM1;
	  else if (c == '$')
	    {
	      /* Backup the pointer so the $ doesn't get displayed */
	      bp--;
	      do_display = 1;
	    }
	  else if ((c >= 32) && (c < 128))
	    csum += c;
	  else
	    do_display = 1;
	  break;
	  /* After we receive a #, we expect two hex digits. This checks for
	     the first */
	case S_CSUM1:
	  if (((c >= 'a') && (c <= 'f')) ||
	      ((c >= 'A') && (c <= 'F')) || ((c >= '0') && (c <= '9')))
	    state = S_CSUM2;
	  else
	    do_display = 1;
	  break;
	  /* We received the first hex digit, now check for the second. If it
	     is there, we have a complete packet */
	case S_CSUM2:
	  if (((c >= 'a') && (c <= 'f')) ||
	      ((c >= 'A') && (c <= 'F')) || ((c >= '0') && (c <= '9')))
	    {
	      unsigned char pktcsum;
	      /* Packet is complete, get the checksum from it */
	      pktcsum = from_hex (*(bp - 2)) << 4;
	      pktcsum |= from_hex (*(bp - 1));
	      /* If the checksum matches what we calculated then accept the
	         packet */
	      if (csum == pktcsum)
		{
		  /* Valid packet, return it to the caller. We strip off the
		     trailer # and two hex digits. The $ was never put on */
		  *(bp - 3) = '\0';
		  if (remote_debug)
		    printf_unfiltered ("Received %s\n", packet);
		  return 1;
		}
	      else
		do_display = 1;
	    }
	  else
	    do_display = 1;
	  break;
	}
      /* If we've exceeded our buffer or any of the above checks failed, the
         packet isn't valid, so display it to the user and start over */
      if (do_display || (bp >= packet + PBUFSIZ))
	{
	  const char *ptr = packet;
	  /* We need to specially handle the $ since it isn't stored in the
	     packet. Only the first state won't print a $ here */
	  if (state != S_BEGIN)
	    putchar_unfiltered ('$');
	  while (ptr < bp)
	    putchar_unfiltered (*ptr++);
	  fflush (stdout);
	  /* As a special case, we don't go back to the begin state if we
	     just received a $ */
	  if (c != '$')
	    state = S_BEGIN;
	  else
	    state = S_DATA;
	  csum = 0;
	  bp = packet;
	  do_display = 0;
	}
    }
}

/* Read a character from the remote system, doing all the fancy remote_timeout
   stuff.  Handles serial errors and EOF.  If TIMEOUT == 0, and no chars,
   returns -1, else returns next char.  Discards chars > 127.  */
static int
readchar (int remote_timeout)
{
  int c, i;

  immediate_quit++;

  for (i = 0; i < 3; i++)
    {
      c = serial_readchar (octeon_desc, remote_timeout);

      if (c >= 0)
	{
	  immediate_quit--;
	  return c & 0x7f;
	}

      if (c == SERIAL_TIMEOUT)
	{
	  if (remote_timeout == 0)
	    return -1;
	  printf_unfiltered ("Ignoring packet error, continuing...\n");
	  continue;
	}
    }

  immediate_quit--;

  if (c == SERIAL_TIMEOUT)
    error ("Timeout reading from remote system.");

  perror_with_name ("readchar");
}

/* Create octeon_desc. Spawn the simulator if debugging over tcp. */
static void
create_connection ()
{
  int j;

  if (is_simulator && octeon_spawn_sim)
    simulator_fork (sim_options);

  if (pending_kill)
    sleep (2);

  for (j = 0; j < 15; j++)
    {
      octeon_desc = serial_open (serial_port_name);
      if (!octeon_desc)
	{
	  sleep (1);
	  continue;
	}
      else
	break;
    }

  if (!octeon_desc)
    perror_with_name (serial_port_name);

  if (serial_setbaudrate (octeon_desc, baud_rate))
    {
      serial_close (octeon_desc);
      perror_with_name (serial_port_name);
    }

  serial_raw (octeon_desc);

  /* Sync up with debug stub on startup. */
  update_step_all ();
  update_core_mask ();
  update_focus ();

  stub_version = send_command_get_int_reply_generic ("?", "S", 0);
  if (remote_debug)
    printf_unfiltered ("Stub version: %x\n", stub_version);


  /* When target command is invoked the second time then octeon_kill() is
     called which sets pending_kill that makes the second run to kill the
     simulator and spawn back. We don't want to do that. Because we just
     spawned the simulator. Reset to the initial state. */
  pending_kill = 0;
}

/* Kill the simulator. Nullify octeon_desc after closing the connection. */
static void
close_connection ()
{
  if (sim_pid && is_simulator)
    {
      kill (sim_pid, SIGKILL);
      wait (sim_pid);
    }

  if (octeon_desc)
    serial_close (octeon_desc);
  octeon_desc = NULL;

  pending_kill = 1;
}

/* Fork the simulator by opening a new xterm.
   xterm -e oct-sim options.  */
static void
simulator_fork (char **sim_options)
{
  sim_pid = fork ();

  if (sim_pid < 0)
    perror_with_name ("fork");

  if (sim_pid == 0)
    {
      int i;
      char *errstring;
      sigset_t oldset;
      sigset_t blocked_mask;

      sigaddset (&blocked_mask, SIGINT);
      sigprocmask (SIG_BLOCK, &blocked_mask, &oldset);

      execvp (sim_options[0], sim_options);

      /* If we get here, it's an error */
      errstring = safe_strerror (errno);
      error ("Cannot execute \"%s\" because %s", sim_options[0], errstring);
    }
}

/* Parse the options that needs to be passed to the simulator for spawning.
   The port no is taken from target command. If no options are passed while 
   invoking target option, then default to "-noperf -quiet" options.  */
static int
simulator_setup_options (char **argv)
{
  int argc = 1;
  int sim_argc = 5;
  char *port_no;

  sim_options = (char **) malloc (PBUFSIZ * 2);
  memset (sim_options, '\0', PBUFSIZ * 2);

  while (argv[argc])
    {
      int len = strlen (argv[argc]);
      sim_options[sim_argc] = strdup (argv[argc]);
      argc++;
      sim_argc++;
    }

  /* Set the default simulator options if none is passed */
  if (sim_options[5] == NULL)
    {
      sim_options[sim_argc++] = strdup ("-noperf");
      sim_options[sim_argc++] = strdup ("-quiet");
    }

  /* Get the port number from target command */
  port_no = argv[0];

  if (strncmp (port_no, "tcp:", 4) == 0)
    {
      char *tcp_port_no;
      sim_options[sim_argc] = (char *) malloc (sizeof (char *) * 15);
      strcpy (sim_options[sim_argc], "-uart1=");
      tcp_port_no = strchr ((port_no + 4), ':');
      if (tcp_port_no)
	strcat (sim_options[sim_argc++], (tcp_port_no + 1));
      else
	{
	  freeargv (sim_options);
	  error ("No colon in host name!");
	  return -1;
	}
    }
  else
    {
      freeargv (sim_options);
      error
	("Incomplete \"target\" command, use \"help target octeon\" command for correct syntax.");
      return -1;
    }

  /* Invoke the simulator in a new terminal */
  sim_options[0] = strdup ("xterm");

  sim_options[1] = strdup ("-e");

  /* Copy the name of the executable. */
  sim_options[2] = strdup ("oct-sim");

  /* Copy the name of the executable as 1st parm to oct-sim. */

  if (exec_bfd == 0)
    {
      error ("No executable file specified.\n\
Use the \"file\" or \"exec-file\" command.");
      freeargv (sim_options);
      return 0;
    }
  else
    sim_options[3] = strdup (exec_bfd->filename);

  /* By default pass -debug option while invoking oct-sim. */
  sim_options[4] = strdup ("-debug");

  return 0;
}

/* Octeon specific commands, available after the target command is invoked.  */
static void
octeon_add_commands ()
{
  static struct cmd_list_element *remote_octeon_set_cmdlist;
  static struct cmd_list_element *remote_octeon_show_cmdlist;

  add_setshow_zinteger_cmd ("focus", class_obscure, &octeon_coreid, _("\
Set the core to be debugged."), _("\
Show the focus core of debugger operations."), 
			    NULL, process_core_command, NULL, 
			    &setlist, &showlist);

  add_setshow_boolean_cmd ("step-all", no_class, &octeon_stepmode, _("\
Set if \"step\"/\"next\"/\"continue\" commands should be applied to all the cores."), _("\
Show step commands affect all cores."), 
			   NULL, process_stepmode_command, NULL, 
			   &setlist, &showlist);

  add_setshow_string_cmd ("active-cores", no_class, &mask_cores, _("\
Set the cores stopped on execution of a breakpoint by another core."), _(" \
Show the cores stopped on execution of a breakpoint by another core."), 
			  NULL, process_mask_command, NULL, 
			  &setlist, &showlist);

  add_setshow_string_cmd ("perf-event0 <events>", no_class, &perf_event[0], 
			  _(" \
Set event for performance counter0 and the counter is reset to zero.\n"), _(" \
Show the performance counter0 event and counter\n"),
			  NULL, set_performance_counter0_event, 
			  show_performance_counter0_event_and_counter,
			  &setlist, &showlist);
                                                                                
  add_setshow_string_cmd ("perf-event1 <events>", no_class, &perf_event[1], 
			  _(" \
Set event for performance counter1 and the counter is reset to zero\n"), _(" \
Show the performance counter1 event and counter\n"),
                    	  NULL, set_performance_counter1_event, 
			  show_performance_counter1_event_and_counter,
			  &setlist, &showlist);
}

static void
octeon_remove_commands ()
{
  delete_cmd ("focus", &setlist);
  delete_cmd ("focus", &showlist);
  delete_cmd ("step-all", &setlist);
  delete_cmd ("step-all", &showlist);
  delete_cmd ("active-cores", &setlist);
  delete_cmd ("active-cores", &showlist);
  delete_cmd ("perf-event0", &setlist);
  delete_cmd ("perf-event0", &showlist);
  delete_cmd ("perf-event1", &setlist);
  delete_cmd ("perf-event1", &showlist);
}

static int 
check_if_simulator ()
{
  if (serial_port_name && strncmp (serial_port_name, "/dev", 4) != 0)
    is_simulator = 1;

  return is_simulator;
}

/* Open a connection to a remote debugger. It is called after target command.  */
static void
octeon_open (char *name, int from_tty)
{
  char **argv;

  if (name == NULL)
    error
      ("To open a MIPS remote debugging connection, you need to specify\n"
       "- what serial device is attached to the target board (eg./dev/ttyS0)\n"
       "- the port number of the tcp (eg. :[HOST]:port)\n");

  target_preopen (from_tty);

  push_target (&octeon_ops);
  setup_generic_remote_run (name, pid_to_ptid (100));

  if ((argv = buildargv (name)) == NULL)
    nomem (0);

  make_cleanup_freeargv (argv);

  serial_port_name = xstrdup (argv[0]);

  if (check_if_simulator () && octeon_spawn_sim)
    simulator_setup_options (argv);
  else if (strncmp (serial_port_name, "/dev", 4) == 0 && argv[1])
    {
      int baudrate;
      baudrate = atoi (argv[1]);
      if (baudrate == 0)
        printf
	  ("Invalid \"%s\" remote baudrate specified after serial port\n",
	    argv[1]);
      else
        baud_rate = baudrate;
    }

  /* Spawn the simulator while debugging over tcp. Establish the connection
     to octeon_desc by opening a serial port. */
  create_connection ();

  if (from_tty)
    printf ("Remote target %s connected to %s\n", octeon_ops.to_shortname,
	    serial_port_name);

  octeon_add_commands ();
}

/* Close all files and local state before this target loses control. */
static void
octeon_close (int quitting)
{
  end_status = 0;

  if (octeon_desc)
    {
      octeon_remove_commands ();
      /* Free the simulator options */
      if (is_simulator)
	freeargv (sim_options);
    }
  /* Kill the simulator and close the serial port. */
  close_connection ();
  sim_options = NULL;
}

/* Send ^C to target to halt it.  Target will respond, and send us a packet. */
static void (*ofunc) (int);

/* The command line interface's stop routine. This function is installed
   as a signal handler for SIGINT. The first time a user requests a
   stop, we call remote_stop to send a break or ^C. If there is no
   response from the target (it didn't stop when the user requested it),
   we ask the user if he'd like to detach from the target. */
static void
octeon_interrupt (int signo)
{
  signal (signo, octeon_interrupt_twice);

  octeon_stop ();
}

/* The user typed ^C twice.  */
static void
octeon_interrupt_twice (int signo)
{
  signal (signo, ofunc);
  target_terminal_ours ();

  if (query ("Interrupted while waiting for the program.\n\
Give up (and stop debugging it)? "))
    {
      octeon_close (0);
      target_mourn_inferior ();
      deprecated_throw_reason (RETURN_QUIT);
    }
  target_terminal_inferior ();

  signal (signo, octeon_interrupt);
}

/* Send COMMAND and expect a reply starting with REPLY.  Return the
   integer value interpreted as hexadecimal value just after REPLY in
   the reply-packet.  On error return ERRORVALUE.  */

static int
send_command_get_int_reply_generic (char *command, char *reply, int errorValue)
{
  char packet[PBUFSIZ];
  size_t reply_len = strlen (reply);

  make_gdb_packet (packet, command);

  if (puts_octeondebug (packet) == 0)
    {
      error ("Couldn't transmit command\n");
      return errorValue;
    }

  do
    {
      if (gets_octeondebug (packet) == 0)
	{
	  error ("Couldn't get reply\n");
	  return errorValue;
	}

      /* Treat packets beginning with "!" as messages to the user from the
         debug monitor */
      if (packet[0] == '!')
	printf_unfiltered ("%s\n", packet + 1);

    }
  while (packet[0] == '!');

  if (strncmp (&packet[0], reply, reply_len) != 0)
    {
      error ("Received incorrect reply (expected %s)\n", reply);
      return errorValue;
    }
  return strtol (packet + reply_len, NULL, 16);
}

/* Send a COMMAND to the remote system and get an integer reply. On
   error return ERRORVALUE.  Assumed that response is upper-case
   version of the COMMAND.  */
static int
send_command_get_int_reply (char *command, int error)
{
  char reply[2];

  reply[0] = toupper (command[0]);
  reply[1] = '\0';
  return send_command_get_int_reply_generic (command, reply, error);
}

/* This function is called to make sure the GDB concept of
   focus core matches the lower level debug monitor. It should
   be called anytime it's possible that the two may be out of
   sync.  */
static void
update_focus ()
{
  char prompt[32];
  cache_mem_addr = 0;
  octeon_coreid = send_command_get_int_reply ("f", octeon_coreid);

  sprintf (prompt, "(Core#%d-gdb) ", octeon_coreid);
  if (strcmp ((const char *) prompt, (const char *) get_prompt ()) != 0)
    set_prompt (prompt);
}

/* Wait until the remote machine stops, then return, storing the status in
   STATUS just as 'wait' would. */
static ptid_t
octeon_wait (ptid_t ptid, struct target_waitstatus *status)
{
  int sigval = 9;
  char packet[PBUFSIZ];
  int old_remote_timeout;
  
  last_wp_addr = 0;

  status->kind = TARGET_WAITKIND_STOPPED;
  status->value.sig = TARGET_SIGNAL_TRAP;

  ofunc = (void (*)()) signal (SIGINT, octeon_interrupt);

  /* The GDB is waiting for a response from debug stub after issuing a
     resume. To wait forever set remote_timeout = -1. */
  old_remote_timeout = remote_timeout;
  remote_timeout = -1;

  while (1)
    {
      char *reason;

      if (gets_octeondebug (packet) == 0)
	{
	  if (remote_debug)
	    printf ("Still waiting because %s\n", packet);
	}
      else if (packet[0] == '!')
	{
	  /* Treat packets beginning with "!" as messages to the user from
	     the debug monitor */
	  printf_unfiltered ("%s\n", packet + 1);
	}
      else if ((packet[0] == 'T') || (packet[0] == 'D'))
	break;
    }

  /* Revert to its original value.  */
  remote_timeout = old_remote_timeout;

  signal (SIGINT, ofunc);

  /* Return the debug execption type */
  if (*packet == 'T')
    {
      /* The debug stub sends "T9" for hardware breakpoints and software
	 breakpoints.  */
      if (packet[1] == '9')
        status->value.sig = TARGET_SIGNAL_TRAP;
      /* The debug stub sends "T8:HWWP_STATUS_BITS" if an hardware watchpoint
	 is hit. Hardware data breakpoint status register value is passed 
	 in HWWP_STATUS_BITS.  */
      else if (packet[1] == '8' && packet[2] == ':' && packet[3])
	{
	  int i, hwwp_hit;
	  /* Read which hardware watchpoint is hit.  */
	  hwwp_hit = strtoul (packet+3, NULL, 16) & 0xf;
          /* Find the address of load/store instruction that caused the 
	     watchpoint exception.  */
	  for (i = 0; i < MAX_OCTEON_INST_BREAKPOINTS; i++)
	    if ((hwwp_hit >> i) & 1)
	      {
		last_wp_addr = hwwp[i];
		break;
	      }
	} 
      /* The focus may have changed due to hitting a breakpoint. Synchronize
         with the low level debugger. Also syncronize step-all mode,
         active-cores. */
      update_step_all ();
      update_core_mask ();
      update_focus ();
    }
  /* When break 0x3ff insn is executed then stop the program as this is not a 
     normal breakpoint.  */
  else if (*packet == 'D')
    {
      status->kind = TARGET_WAITKIND_EXITED;
      status->value.sig = TARGET_SIGNAL_TRAP;
      status->value.integer = from_hex (packet[1]);
      end_status = (1 << octeon_coreid);
    }
  else
    {
      error ("Wrong signal from target \n");
      status->kind = TARGET_WAITKIND_STOPPED;
      status->value.sig = TARGET_SIGNAL_ILL;
    }
  return inferior_ptid;
}

/* This is the generic stop called via the target vector. When a target
   interrupt is requested, either by the command line or the GUI, we
   will eventually end up here. */
static void
octeon_stop (void)
{
  /* Send a ^C.  */
  make_and_send_packet ("\003");
}

/* Terminate the open connection to the remote debugger.  Use this
   when you want to detach and do something else with your gdb.  */
static void
octeon_detach (char *arg, int from_tty)
{
  pop_target ();		/* calls octeon_close to do the real work */
  if (from_tty)
    printf_unfiltered ("Ending remote %s debugging\n", target_shortname);
}

/* Tell the remote machine to resume.  */
static void
octeon_resume (ptid_t ptid, int step, enum target_signal sigal)
{
  cache_mem_addr = 0;

  if (step)
    make_and_send_packet ("s");
  else
    make_and_send_packet ("c");
}

/* Fetch the remote registers. */
static void
octeon_fetch_registers (int regno)
{
  char reg[MAX_REGISTER_SIZE];
  char *packet = alloca (PBUFSIZ);

  if (regno > NUM_REGS)
    return;
  make_and_send_packet ("g%04x", regno);

  gets_octeondebug (packet);
  /* We got the number the register holds, but gdb expects to see a value 
     in the target byte ordering.  */
  store_unsigned_integer (reg, register_size (current_gdbarch, regno),
			  strtoull (packet, NULL, 16));
  regcache_raw_supply (current_regcache, regno, reg);
}

/* Store all registers into remote target */
static void
octeon_store_registers (int regno)
{
  /* This fix is required for "break func" followed by "run" command to work
     if focus is set before. */
  if (regno == MIPS_EMBED_PC_REGNUM)
    {
      CORE_ADDR entry_pt;

      entry_pt = bfd_get_start_address (exec_bfd);

      if (entry_pt == read_register (MIPS_EMBED_PC_REGNUM))
	return;
    }

  make_and_send_packet ("G%04x,%016llx", regno,
			(unsigned long long) read_register (regno));
}

/* Get ready to modify the registers array.  On machines which store
   individual registers, this doesn't need to do anything.  On machines
   which store all the registers in one fell swoop, this makes sure
   that registers contains all the registers from the program being
   debugged.  */
static void
octeon_prepare_to_store ()
{
  /* Do nothing, since we can store individual regs */
}

/* Print info on this target.  */
static void
octeon_files_info (struct target_ops *ignore)
{
  if (is_simulator)
    printf_unfiltered ("Debugging Octeon on simulator over a serial line.\n");
  else
    printf_unfiltered ("Debugging Octeon on board over a serial line.\n");
}

/* Write memory data directly to the remote machine.
   This does not inform the data cache; the data cache uses this.
   MEMADDR is the address in the remote memory space.
   MYADDR is the address of the buffer in our space.
   LEN is the number of bytes.
                                                                                
   Returns number of bytes transferred, or 0 (setting errno) for
   error.  Only transfer a single packet. */
static unsigned int
octeon_write_inferior_memory (CORE_ADDR memaddr, unsigned char *myaddr,
			      int len)
{
  long i;
  int j;
  char packet[PBUFSIZ];
  char buf[PBUFSIZ];
  char *p;

  /* Do not send packet to debug_stub when the core finished executing. */

  if (end_status & (1 << octeon_coreid))
    return len;

  cache_mem_addr = 0;
  p = buf + snprintf (buf, sizeof (buf), "M%016llx,%04x:",
		      (unsigned long long) memaddr, len);
  for (j = 0; j < len; j++)
    {				/* copy the data in after converting it */
      *p++ = tohex ((myaddr[j] >> 4) & 0xf);
      *p++ = tohex (myaddr[j] & 0xf);
    }
  *p = 0;

  make_and_send_packet (buf);

  do
    {
      if (gets_octeondebug (packet) == 0)
	error ("couldn't receive packet \n");
      else if (packet[0] == '!')
	printf_filtered ("%s\n", packet + 1);
    }
  while (packet[0] == '!');

  if (remote_debug)
    {
      if (*packet == '-')
	error ("Problem writing into memory \n");
    }
  return len;
}

/* Helper function to octeon_read_inferior_memory(). If the memory
   address requested to read is already in cache_mem_addr return it
   instead of getting it from debug_stub.  */
static int
cache_mem_read (CORE_ADDR memaddr, char *myaddr, int len)
{
  if (len > sizeof (cache_mem_data))
    len = sizeof (cache_mem_data);

  if ((cache_mem_addr) && (memaddr >= cache_mem_addr)
      && (memaddr < cache_mem_addr + sizeof (cache_mem_data)))
    {
      if (memaddr + len > cache_mem_addr + sizeof (cache_mem_data))
	len = cache_mem_addr + sizeof (cache_mem_data) - memaddr;

      memcpy (myaddr, cache_mem_data + (memaddr - cache_mem_addr), len);
      return len;
    }
  else
    {
      char packet[PBUFSIZ];
      int j;
      char *ptr = cache_mem_data;
      char *hexptr = packet;
      make_and_send_packet ("m%016llx,%04x", (unsigned long long) memaddr,
			    (int) sizeof (cache_mem_data));

      do
	{
	  if (gets_octeondebug (packet) == 0)
	    error ("couldn't receive packet \n");
	  else if (packet[0] == '!')
	    printf_filtered ("%s\n", packet + 1);
	}
      while (packet[0] == '!');

      if (*packet == 0)
	error ("Got no data in the GDB packet \n");
      else if (*packet == '-')
	error ("Problem in reading the memory location \n");

      for (j = 0; j < sizeof (cache_mem_data); j++)
	{
	  *ptr++ = from_hex (*hexptr) * 16 + from_hex (*(hexptr + 1));
	  hexptr += 2;
	}
      cache_mem_addr = memaddr;
      memcpy (myaddr, cache_mem_data, len);
      return len;
    }
}

/* Read memory data directly from the remote machine.
   This does not use the data cache; the data cache uses this.
   MEMADDR is the address in the remote memory space.
   MYADDR is the address of the buffer in our space.
   LEN is the number of bytes.

   Returns number of bytes transferred, or 0 for error.  */
static int
octeon_read_inferior_memory (CORE_ADDR memaddr, char *myaddr, int len)
{
  int amount_left = len;
  while (amount_left)
    {
      int count = cache_mem_read (memaddr, myaddr, amount_left);
      memaddr += count;
      myaddr += count;
      amount_left -= count;
    }
  return len;
}

/* Read and write memory of and from target respectively. */
static int
octeon_xfer_inferior_memory (CORE_ADDR memaddr, gdb_byte * myaddr, int len,
			     int write, struct mem_attrib *attrib,
			     struct target_ops *target)
{
  if (write)
    return octeon_write_inferior_memory (memaddr, myaddr, len);
  else
    return octeon_read_inferior_memory (memaddr, myaddr, len);
}

static void
octeon_kill ()
{
  inferior_ptid = null_ptid;
  /* Set this to inform octeon_create_inferior to kill the simulator and
     spawn again. This is required for the user to set/delete breakpoints
     before the run command is issued. */
  pending_kill = 1;
}

/* At present the simulator is loading the program into memory and executing it
   so no need to do anything. */
static void
octeon_load (char *args, int from_tty)
{
  /* No need to call generic_load.  While debugging over serial port or over
     tcp, the program is loaded into memory by the bootloader.  */
  return;
}

/* Clean up when a program exits.
                                                                                
   The program actually lives on in the remote processor's RAM, and may be
   run again without a download.  Don't leave it full of breakpoint
   instructions.  */
static void
octeon_mourn_inferior ()
{
  remove_breakpoints ();
  unpush_target (&octeon_ops);
  generic_mourn_inferior ();	/* Do all the proper things now */
}

static int
octeon_multicore_hw_breakpoint ()
{
  return 1;
}

/* Update the address of the hardware watchpoint that hit. */
static int
octeon_stopped_data_address (struct target_ops *target, CORE_ADDR *addrp)
{
  if (last_wp_addr)
    {
      *addrp = last_wp_addr;
      return 1;
    }
  return 0;
}

/* Return 1 if hardware watchpoint is hit, otherwise return 0.  */
static int
octeon_stopped_by_watchpoint (void)
{
  CORE_ADDR addr;
  return octeon_stopped_data_address (&current_target, &addr);
}

/* CNT is the number of hardware breakpoint to be installed. Return non-zero
   value if the value does not cross the max limit (4 for Octeon).  */
static int
octeon_can_use_watchpoint (int type, int cnt, int othertype)
{
  int i; 

  if (cnt > MAX_OCTEON_INST_BREAKPOINTS)
    {
      printf_unfiltered ("Octeon supports only four hardware breakpoints/watchpoints\n");
      return -1;
    }
  return 1; 
}

static int 
octeon_get_core_number ()
{
  return octeon_coreid;
}

/* Octeon has 4 instruction hardware breakpoints per core. Insert hardware
   breakpoints that are applied to each core by changing the focus. */
static int
octeon_insert_mc_hw_breakpoint (struct bp_target_info *bp_tgt, int core)
{
  int i;
  int saved_core = octeon_coreid;
  
  if (!(octeon_activecores >> core) & 0x1)
    return -1;

  for (i = 0; i < MAX_OCTEON_INST_BREAKPOINTS; i++)
    {
      if (hwbp[core][i] == 0)
        {
	  if (core != saved_core)
	    octeon_change_focus (core);
	    
	  hwbp[core][i] = bp_tgt->placed_address;
	  make_and_send_packet("Zi%x,%016llx", i, bp_tgt->placed_address);
	  octeon_coreid = octeon_change_focus (saved_core);
	  return 0;
	}
     }
  return -1;
}

/* Remove hardware breakpoints. */
static int
octeon_remove_mc_hw_breakpoint (struct bp_target_info *bp_tgt, int core)
{
  int i;
  int saved_core = octeon_coreid;

  /* Do not send packet to debug_stub when the core finished executing. */
  if (end_status & (1<<octeon_coreid))
    return -1;
  
  if (!(octeon_activecores >> core) & 0x1)
    return -1;

  for (i = 0; i < MAX_OCTEON_INST_BREAKPOINTS; i++)
    {
      if (hwbp[core][i] == bp_tgt->placed_address)
        {
	  if (core != saved_core)
	    octeon_change_focus (core);
	    
	  hwbp[core][i] = 0;
	  make_and_send_packet("zi%x", i);
	  octeon_coreid = octeon_change_focus (saved_core);
	  return 0;
	}
     }
  return -1;
}

/* Octeon has 4 data hardware breakpoints (watchpoints) per core. At present
   watchpoints are implemented implemented globally instead of per core.
   Total there are only 4 hardware watchpoints, any request after that will
   be treated as software watchpoints.  */
static int
octeon_insert_hw_watchpoint (CORE_ADDR addr, int len, int type)
{
  int i;

  for (i = 0; i < MAX_OCTEON_INST_BREAKPOINTS; i++)
    {
      if (hwwp[i] == 0)
        {

          hwwp[i] = addr;
          make_and_send_packet ("Zd%x,%016llx,%x,%x", i, 
				(unsigned long long)addr, len, type);
          return 0;
        }
    }
  return -1;
}

/* Remove hardware watchpoints. */
static int
octeon_remove_hw_watchpoint (CORE_ADDR addr, int len, int type)
{
  int i;

  /* Do not send packet to debug_stub when the core finished
     executing. */
  if (end_status & (1 << octeon_coreid))
    return 0;
                                                                                
  for (i = 0; i < MAX_OCTEON_INST_BREAKPOINTS; i++)
    {
      if (hwwp[i] == addr)
        {
          hwwp[i] = 0;
          make_and_send_packet ("zd%x", i);
          return 0;
        }
    }
  return -1;
}

/* Insert a breakpoint on targets that don't have any better breakpoint
   support.  We read the contents of the target location and stash it,
   then overwrite it with a breakpoint instruction.  ADDR is the target
   location in the target machine.  CONTENTS_CACHE is a pointer to
   memory allocated for saving the target contents.  It is guaranteed
   by the caller to be long enough to save sizeof BREAKPOINT bytes (this
   is accomplished via BREAKPOINT_MAX).  */
static int
octeon_insert_breakpoint (struct bp_target_info *bp_tgt)
{
  return memory_insert_breakpoint (bp_tgt);
}

static int
octeon_remove_breakpoint (struct bp_target_info *bp_tgt)
{
  return memory_remove_breakpoint (bp_tgt);
}

static void
convert_active_cores_to_string ()
{
  char output[80];
  int i;
  int loc;

  /* Convert the bitmask into a comma seperated core list */
  loc = 0;
  for (i = 0; i < 16; i++)
    {
      if (octeon_activecores & (1 << i))
	loc += sprintf (output + loc, "%d,", i);
    }

  /* Remove the ending comma */
  if (loc)
    output[loc - 1] = 0;

  /* Only update GDB's variable if the value changed */
  if ((mask_cores == NULL) || (strcmp (mask_cores, output) != 0))
    {
      if (mask_cores)
	xfree (mask_cores);
      mask_cores = savestring (output, strlen (output));
    }
}

/* Update the focus of the core. Do not update the prompt. This is required
   to send hardware breakpoints to the core that is not in focus. When the
   cores resume the hardware breakpoints that are inserted to the other
   cores will also get executed. */
static int
octeon_change_focus (int coreid)
{
  char buf[64];
  snprintf (buf, sizeof (buf), "F%x", coreid);
  coreid = send_command_get_int_reply (buf, coreid);

  return coreid;
}

/* Get the number of the core to be debugged. */

static void
process_core_command (char *args, int from_tty,
			     struct cmd_list_element *c)
{
  cache_mem_addr = 0;
  octeon_coreid = octeon_change_focus (octeon_coreid);

  update_focus ();
  registers_changed ();
  normal_stop ();
}

/* If stepmode is set to 1, make all cores step/next. By default the step/next
   command gets applied to only the focused core. */
static void
process_stepmode_command (char *args, int from_tty,
				 struct cmd_list_element *c)
{
  cache_mem_addr = 0;
  update_step_all ();
}

/* Sync octeon_stepmode (step-all) with debug monitor. */
static void
update_step_all ()
{
  char buf[64];

  snprintf (buf, sizeof (buf), "A%x", octeon_stepmode);
  octeon_stepmode = send_command_get_int_reply (buf, octeon_stepmode);
}

/* Get the number of cores to stop on debug exception. */
static void
process_mask_command (char *args, int from_tty,
			     struct cmd_list_element *c)
{
  update_core_mask ();
}

/* Sync octeon_activecores (active-cores) with debug monitor. */
static void
update_core_mask ()
{
  char buf[64];
  int coreid;
  char *ptr;

  octeon_activecores = 0;
  if (mask_cores)
    {
      char mcores[strlen (mask_cores) + 1];
      strcpy (mcores, mask_cores);
      ptr = strtok (mcores, ",");
      while (ptr)
	{
	  coreid = atoi (ptr);
	  octeon_activecores |= (1 << coreid);
	  ptr = strtok (NULL, ",");
	}
    }

  snprintf (buf, sizeof (buf), "I%04x", octeon_activecores);
  octeon_activecores = send_command_get_int_reply (buf, octeon_activecores);
  convert_active_cores_to_string ();
}

/* Return true if the FEATURE is supported in a particular stub_version.  */
static int
check_stub_feature (enum stub_feature feature)
{
  if (feature == STUB_PERF_EVENT_TYPE)
    return stub_version >= 10;
  
  gdb_assert (1);
  return 0;
}

/* Set performance counter counters based on event.
   COUNTER - 1 = performance counter 1, 0 = performance counter 0 */

static void
set_performance_counter_event (int counter)
{
  int i;
  int pevent = -1;
                                                                                
  if (perf_event[counter])
    {
      for (i = 0; i < MAX_NO_PERF_EVENTS; i++)
        {
          if (perf_events_t[i].event 
	      && strcmp (perf_event[counter], perf_events_t[i].event) == 0)
	    {
              pevent = i;
	      perf_status_t[octeon_coreid][counter] = i;
	      make_and_send_packet ("e%d%x", counter + 1, i);
	      break;
	    }
        }
    }

  if (pevent == -1)
    {
      printf_unfiltered 
	("These are the performance counter events supported by Octeon:\n\n");
      printf_unfiltered ("Event            Description\n\n");
      for (i = 0; i < MAX_NO_PERF_EVENTS; i++)
	if (perf_events_t[i].event)
          printf_unfiltered ("%-16s %s\n", perf_events_t[i].event, perf_events_t[i].help);
    }
}
                                                                                
static void
set_performance_counter0_event (char *args, int from_tty, struct cmd_list_element *c)
{
  set_performance_counter_event (0);
}
                                                                                
static void
set_performance_counter1_event (char *args, int from_tty, struct cmd_list_element *c)
{
  set_performance_counter_event (1);
}

/* Show performance counter counters for the events set.
   COUNTER - 1 = performance counter 1, 0 = performance counter 0 */

static void
show_performance_counter_event_and_counter (int counter)
{
  char *packet = alloca (PBUFSIZ);
  unsigned long perf_counter, event;
  
  if (perf_event[counter])
    {
      if (counter)
        make_and_send_packet ("e4");
      else
        make_and_send_packet ("e3");

      gets_octeondebug(packet);

      perf_counter = strtoul (packet, &packet, 16);
      packet++;
      if (check_stub_feature (STUB_PERF_EVENT_TYPE))
        /* The debug stub returns "counter,event_type" packet.  */
	event = strtoul (packet, NULL, 16);
      else
	/* The debug stub returns only counter, get the event_type from
	   perf_status_t stored in set_performance_counter_event.  */ 
	event = perf_status_t[octeon_coreid][counter];

      printf_unfiltered ("Performance counter%d for \"%s\" event is %ld\n",
			 counter, perf_events_t[event].event, perf_counter);
    }
  else
    printf_unfiltered ("Performance counter%d event is not set.\n", counter); 
}

static void
show_performance_counter0_event_and_counter (struct ui_file *file, int from_tty, 
				  	     struct cmd_list_element *c, 
					     const char *value)
{
  show_performance_counter_event_and_counter (0);
}

static void
show_performance_counter1_event_and_counter (struct ui_file *file, int from_tty, 
					     struct cmd_list_element *c, 
					     const char *value)
{
  show_performance_counter_event_and_counter (1);
}

/* Forward it to remote-run.  */

static int 
octeon_can_run (void)
{
  return generic_remote_can_run_target (octeon_ops.to_shortname);
}

static void
init_octeon_ops (void)
{
  add_setshow_boolean_cmd ("spawn-sim", no_class, &octeon_spawn_sim, _("\
Set to zero to not spawn the simulator upon the target command."), _("\
Show whether the simulator would be spawned upon the target command."),
			  NULL, NULL, NULL,
			  &setlist, &showlist);

  octeon_ops.to_shortname = "octeon";
  octeon_ops.to_longname = "Remote Octeon target";
  octeon_ops.to_doc = "Use a remote Octeon ICE connected by a serial line;\n"
    "or a tcp connection.\n"
    "Arguments are the name of the device for the serial line,\n"
    "the speed to connect at in bits per second or the\n"
    "tcp port for tcp connection. eg\n"
    "target octeon /dev/ttyS0 9600\n" "target octeon tcp:[HOST]:65258";
  octeon_ops.to_open = octeon_open;
  octeon_ops.to_close = octeon_close;
  octeon_ops.to_detach = octeon_detach;
  octeon_ops.to_resume = octeon_resume;
  octeon_ops.to_wait = octeon_wait;
  octeon_ops.to_fetch_registers = octeon_fetch_registers;
  octeon_ops.to_store_registers = octeon_store_registers;
  octeon_ops.to_prepare_to_store = octeon_prepare_to_store;
  octeon_ops.deprecated_xfer_memory = octeon_xfer_inferior_memory;
  octeon_ops.to_files_info = octeon_files_info;
  octeon_ops.to_insert_breakpoint = octeon_insert_breakpoint;
  octeon_ops.to_remove_breakpoint = octeon_remove_breakpoint;
  octeon_ops.to_can_use_hw_breakpoint = octeon_can_use_watchpoint;
  octeon_ops.to_insert_mc_hw_breakpoint = octeon_insert_mc_hw_breakpoint;
  octeon_ops.to_remove_mc_hw_breakpoint = octeon_remove_mc_hw_breakpoint;
  octeon_ops.to_multicore_hw_breakpoint = octeon_multicore_hw_breakpoint;
  octeon_ops.to_get_core_number = octeon_get_core_number;
  octeon_ops.to_insert_watchpoint = octeon_insert_hw_watchpoint;
  octeon_ops.to_remove_watchpoint = octeon_remove_hw_watchpoint;
  octeon_ops.to_stopped_data_address = octeon_stopped_data_address;
  octeon_ops.to_stopped_by_watchpoint = octeon_stopped_by_watchpoint;
  octeon_ops.to_kill = octeon_kill;
  octeon_ops.to_load = octeon_load;
  octeon_ops.to_create_inferior = generic_remote_create_inferior;
  octeon_ops.to_can_run = octeon_can_run;
  octeon_ops.to_mourn_inferior = octeon_mourn_inferior;
  octeon_ops.to_stop = octeon_stop;
  octeon_ops.to_stratum = process_stratum;
  octeon_ops.to_has_all_memory = 1;
  octeon_ops.to_has_memory = 1;
  octeon_ops.to_has_stack = 1;
  octeon_ops.to_has_registers = 1;
  octeon_ops.to_has_execution = 1;
  octeon_ops.to_magic = OPS_MAGIC;
};

void
_initialize_octeon (void)
{
  struct cmd_list_element *c;
  init_octeon_ops ();
  add_target (&octeon_ops);
}
