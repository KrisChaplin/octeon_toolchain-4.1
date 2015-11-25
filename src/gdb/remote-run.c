/* Provide run command from remote targets.

   Copyright (C) 2006 Cavium Networks.

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

/* When run is invoked on remote targets issue the equivalent of the
   last target command and a continue (implicit via run_command_1).
   In a target;run sequence, run should not reopen the target but
   simply issue a continue.  Even if the target closes we should be
   able to find the last target used (if it was registered with us)
   and invoke it with the same target arguments.  */

#include "defs.h"
#include "target.h"
#include "gdb_assert.h"
#include "inferior.h"
#include <string.h>

/* Original to_remote handler.  */
static void (*remote_resume) (ptid_t, int, enum target_signal);

/* Count the number of resumes since the last open.  If we just hit
   the debug exception handler after the target command there is no
   need to reopen the target.  */
static int nr_resumes;

/* Remember the open function of the last target opened.  This
   setup_generic_remote_run does need to pass its target_ops.  (Note
   that current_target is not equivalent of the target_ops of the
   current target but it is a union of all the pused targets.) */
static void (*last_target_to_open) (char *, int);
/* Arguments to the last target command.  */
static char *last_args;
/* Save this for debugging purposes.  */
static char * last_to_shortname;

/* Delay setting inferior_ptid so that we don't prompt the user at a
   run command just after openning the target.  */
static ptid_t current_inferior_ptid;

/* Don't reopen target if all we did so far was to connect to the
   target.  */

static int
reopen_p ()
{
  if (remote_debug)
    fprintf_unfiltered (gdb_stdlog, "  nr_resumes: %d\n", nr_resumes);
  return nr_resumes == 0;
}

/* Handle run by issuing a target command.  Try to do this by printing
   as less as possible.  The subsequent continue will issued by our
   caller.  */

void
generic_remote_create_inferior (char *execfile, char *args, char **env,
				int from_tty)
{
  if (remote_debug)
    fprintf_unfiltered (gdb_stdlog, "generic_remote_create_inferior:\n");

  if (reopen_p ())
    {
      if (remote_debug)
	fprintf_unfiltered (gdb_stdlog, "  no need to reopen target\n");

      /* Reset stop_soon to print breakpoint information after
	 disabling it in start_remote().  */
      init_wait_for_inferior ();
      
      return;
    }

  if (remote_debug)
    fprintf_unfiltered (gdb_stdlog, "  reopening target %s with %s\n",
			last_to_shortname, last_args);

  /* We are already killed at this point so don't prompt the user
     again in target_preopen() if the target is still alive.  */
  if (last_to_shortname
      && strcmp (current_target.to_shortname, last_to_shortname) == 0)
    pop_target ();

  /* Don't print the frame where we stop after connecting only the one
     after continue.  */
  never_print_frame = 1;
  /* No need to reset nr_resumes here as this will call back to us as
     setup_generic_remote_run below.  */
  last_target_to_open (last_args, 0);
  never_print_frame = 0;

  /* Reset stop_soon to print breakpoint information after disabling
     it in start_remote().  */
  init_wait_for_inferior ();
}

/* Capture resume call to know whether we have started effectively
   debugging the program.  */

static void
generic_remote_resume (ptid_t ptid, int step, enum target_signal signal)
{
  nr_resumes++;
  inferior_ptid = current_inferior_ptid;

  if (remote_debug && nr_resumes == 1)
    fprintf_unfiltered (gdb_stdlog, "generic_remote_resume: resumed once\n");
  
  remote_resume (ptid, step, signal);
}

/* This can be called from the targets's to_can_run handler.  If we've
   seen it how to run a target we can run it again.  */

int
generic_remote_can_run_target (char *shortname)
{
  return last_to_shortname && strcmp (shortname, last_to_shortname) == 0;
}

/* Call this from the target open function.  Also set
   to_create_inferior to generic_remote_create_inferior and call
   generic_remote_can_run_target from the to_can_run handler.  */

void
setup_generic_remote_run (char *args, ptid_t ptid)
{
  if (remote_debug)
    fprintf_unfiltered (gdb_stdlog, "Setting up generic_remote target\n");

  remote_resume = current_target.to_resume;
  current_target.to_resume = generic_remote_resume;

  last_target_to_open = current_target.to_open;
  last_args = args ? xstrdup (args) : NULL;
  last_to_shortname = current_target.to_shortname;

  /* Until the first resume pretend we haven't started the program
     yet.  This way in a target;run sequence, run is performed without
     prompting the user.  */
  current_inferior_ptid = ptid;

  nr_resumes = 0;
}
