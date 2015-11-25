/* Octeon file I/O and other OS functions.
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

#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>

#define CVMX_SYNC asm volatile ("sync" : : :"memory")
#define CVMX_SYNCIO asm volatile ("syncio" : : :"memory")
#define CVMX_SYNCIOBDMA asm volatile ("synciobdma" : : :"memory")
#define CVMX_SYNCIOALL asm volatile ("syncioall" : : :"memory")
#define CVMX_SYNCW asm volatile ("syncw" : : :"memory")

static int octeon_os_uart = 0;
uint64_t __boot_desc_addr;
static int enable_uart_write = 0;
uint32_t octeon_cpu_clock_hz;
static uint64_t rtc_cycles_from_epoch = 0;

/* The size of struct stat is different for N32 and EABI ABI's. Created a 
   struct st which is from EABI ABI to pass it to the simulator magic and 
   later copy the relavant elements of the struct that are used by the 
   application.  */
struct st
{
  dev_t         st_dev;
  ino_t         st_ino;
  mode_t        st_mode;
  nlink_t       st_nlink;
  uid_t         st_uid;
  gid_t         st_gid;
  dev_t         st_rdev;
  int64_t       st_size;
  int64_t       st_atime;
  int64_t       st_spare1;
  int64_t       st_mtime;
  int64_t       st_spare2;
  int64_t       st_ctime;
  int64_t       st_spare3;
  int64_t       st_blksize;
  int64_t       st_blocks;
  int64_t       st_spare4[2];
};

/* This controls if output, stdout and stderr go to the uart or simmagic. 
   By default they go to simmagic. It is recommended to call this function 
   as early as possible in startup. If ENABLE_UART is non-zero, then stdout 
   and stderr messages go to the uart.  */
void 
octeon_os_use_uart (int enable_uart)
{
  enable_uart_write = enable_uart;
}

void 
octeon_os_set_uart_num (int uart_num)
{
  if (uart_num == 0 || uart_num == 1)
    octeon_os_uart = uart_num;
}

/* simmagic printf() function.  */
int 
simprintf ()
{
  CVMX_SYNC;
  asm volatile (
	"add $25, $0, 6\n"
	"dli $15,0x8000000feffe0000\n"
	"dadd $24, $31, $0\n"
	"jalr $15\n"
	"dadd $31, $24, $0\n"
	: : );
  return 0;
}

/* This is the MIPS cache flush function call.  No defines are provided
   by libgloss for 'cache', and CFE doesn't let you flush ranges, so
   we just flush all I & D for every call.  */
void _flush_cache (void)
{
  CVMX_SYNC;
}

/* Move read/write pointer. Using simulator simmagic.  */
off_t 
lseek (int fd, off_t offset, int whence)
{
  register int arg1 asm ("$4") = fd;
  register int arg2 asm ("$5") = offset;
  register int arg3 asm ("$6") = whence;
  CVMX_SYNC;
  asm volatile (
	"add $25, $0, 0xd\n"
	"dli $15,0x8000000feffe0000\n"
	"dadd $24, $31, $0\n"
	"jalr $15\n"
	"dadd $31, $24, $0\n"
	:  	
	: "r" (arg1), "r" (arg2), "r" (arg3) 
	: "$2");
}

/* Open a file descriptor. Using simulator magic.  */
int 
open (char *buf, int flags, int mode)
{
  register char *arg1 asm ("$4") = buf;
  register int arg2 asm ("$5") = flags;
  register int arg3 asm ("$6") = mode;
  CVMX_SYNC;
  asm volatile (
	"add $25, $0, 0x9\n"
	"dli $15,0x8000000feffe0000\n"
	"dadd $24, $31, $0\n"
	"jalr $15\n"
	"dadd $31, $24, $0\n"
	:  	
	: "r" (arg1), "r" (arg2), "r" (arg3) 
	: "$2");
}

/* Used by newlib as mentioned in the libgloss porting documentation.  */
int 
isatty (int fd)
{
  return (1);
}

/* Close a file descriptor. Using simulator magic.  */
int 
close (int fd)
{
  register int arg1 asm ("$4") = fd;
  /* These file descriptors are not created by explicit open command. The 
     __sinit() is called internally by stdio functions that open file 
     descriptors for stdin, stdout and stderr. Uart is handled internally. 
     Don't use sim-magic to close.  */
  if (enable_uart_write && (fd >= 0 && fd <= 2))
    return 0;

  CVMX_SYNC;
  asm volatile (
	"add $25, $0, 0xA\n"
	"dli $15,0x8000000feffe0000\n"
	"dadd $24, $31, $0\n"
	"jalr $15\n"
	"dadd $31, $24, $0\n"
	:  : "r" (arg1));
}

/* Get status of a file. Since we have no file system, using simulator magic
   to initialize the struct stat.  */
int 
stat (const char *path, struct stat *sbuf)
{
  struct st local_st;
  register int status asm ("$2");
  register const char *arg1 asm ("$4") = path;
  CVMX_SYNC;
  asm volatile (
	"add $25, $0, 0xB\n"
	"dli $15,0x8000000feffe0000\n"
	"dadd $24, $31, $0\n"
	"jalr $15\n"
	"dadd $31, $24, $0\n"
	: "=r" (status)
	: "r" (arg1), "r" (&local_st)
	: "$5", "$6");
  sbuf->st_size = local_st.st_size;
  sbuf->st_ino = local_st.st_ino;
  sbuf->st_mode = local_st.st_mode;
  return status;
}

/* Get status of a file. Since we have no file system, using simulator magic
   to initialize the struct stat.  */
int 
fstat (int fd, struct stat *st)
{
  struct st local_st;
  register int status asm ("$2");
  register int arg1 asm ("$4") = fd;

  CVMX_SYNC;
    
  /* Fake out calls for standard streams */
  if (fd < 3)
    {
      st->st_mode = S_IFCHR;
      return (0);
    }
  asm volatile (
	"add $25, $0, 0xC\n"
	"dli $15,0x8000000feffe0000\n"
	"dadd $24, $31, $0\n"
	"dadd  $4, %[fd], $0\n"
	"dadd  $5, %[buf], $0\n"
	"jalr $15\n"
	"dadd $31, $24, $0\n"
	: "=r" (status)
	: [fd] "r" (arg1), [buf] "r" (&local_st)
	: "$5", "$6");

  st->st_size = local_st.st_size;
  st->st_ino = local_st.st_ino;
  st->st_mode = local_st.st_mode;
  return status;
}

/* Read bytes from file descriptor. Using simulator magic.  */
int 
read (int fd, char *buf, int len)
{
  register int arg1 asm ("$4") = fd;
  register char *arg2 asm ("$5") = buf;
  register int arg3 asm ("$6") = len;
  CVMX_SYNC;
  asm volatile (
	"add $25, $0, 8\n"
	"dli $15,0x8000000feffe0000\n"
	"dadd $24, $31, $0\n"
	"jalr $15\n"
	"dadd $31, $24, $0\n"
	: "=r" (arg3)
	: "r" (arg1), "r" (arg2)
	: "$2");
}

/* Using simulator magic to write NBYTES from BUF to stdout.  */
static int 
magic_write (int fd, char *buf, int nbytes)
{
  int retval;
  register int arg1 asm ("$4") = fd;
  register char *arg2 asm ("$5") = buf;
  register int arg3 asm ("$6") = nbytes;

  CVMX_SYNC;
  asm volatile (
	"add $25, $0, 7\n"
	"dli $15,0x8000000feffe0000\n"
	"dadd $24, $31, $0\n"
	"jalr $15\n"
	"dadd $31, $24, $0\n"
	: "=r" (retval) 
	: "r" (arg1), "r" (arg2), "r" (arg3)
	: "$2");
}

/* Used by printf, etc for stdout output.  */
int 
write (int fd, char *buf, int nbytes)
{
  /* Redirect the output to simulator magic or to uart.  */
  if (enable_uart_write && ((fd == 1) || (fd == 2)))
    {
      octeon_uart_write (octeon_os_uart, buf, nbytes);
      return nbytes;
    }
  else
    {
      magic_write (fd, buf, nbytes);
      return nbytes;
    }
}

/* Heap address and size are defined in boot_elf.h.  */
uint64_t  boot_heap_base;
uint64_t  boot_heap_end;

/* Changes heap size. Get NBYTES more from RAM.  */ 
char *
sbrk (int nbytes)
{
  CVMX_SYNC;
  char *tmp;
  if ((boot_heap_base) + nbytes > (boot_heap_end))
    return ((char *)-1);
  else
    {
      tmp = (char *)boot_heap_base;
      boot_heap_base += nbytes;
      return (tmp);
    }
}

/* Used by newlib. We don't have any file system, so creating a process
   is not allowed just return with an error message.  */
int 
fork ()
{
  errno=EAGAIN;
  return (-1);
}

/* Used by newlib. As per libgloss porting document, returning any value
   greater than 1 doesn't effect anything in newlib because there is
   no process control.  */
#define __MYPID 1
int 
getpid ()
{
  return __MYPID;
}

/* Used by newlib. There is no process created, waiting for a process to
   terminate does not apply.  */ 
int 
wait (int *status)
{
  errno = ECHILD;
  return (-1);
}

/* Used by newlib. As per libgloss porting document, kill() doesn't apply
   in an enviornment with no process control, it just exits.  */
int 
kill (int pid, int sig)
{
  if (pid == __MYPID)
    _exit (sig);

  return 0;
}

/* Used by newlib. File system is not supported, return an error 
   while making a hard link.  */
int 
link (char *old, char *new)
{
  errno=EMLINK;
  return (-1);
}

/* Used by newlib. File system is not supported, return an error 
   while removing a hard link.  */
int 
unlink (char *name)
{
  errno = ENOENT;
  return (-1);
}

#ifndef CVMX_RDHWR
#define CVMX_TMP_STR(x) CVMX_TMP_STR2(x)
#define CVMX_TMP_STR2(x) #x

#define CVMX_RDHWR(result, regstr) asm volatile ("rdhwr %[rt],$" CVMX_TMP_STR(regstr) : [rt] "=d" (result))
#endif

/* Return the elapsed time in struct timeval TV.  */
int 
gettimeofday (struct timeval *tv, struct timezone *tz)              
{
  uint64_t cycle;
  /* Get cycle count.  */
  CVMX_RDHWR (cycle, 31);
  cycle += rtc_cycles_from_epoch;
  if (tv)
    {
      tv->tv_sec = cycle / octeon_cpu_clock_hz;
      tv->tv_usec = (cycle % octeon_cpu_clock_hz) / (octeon_cpu_clock_hz / (1000 * 1000));
      return 0;
    }
  else
    return -1;
}

/* Return number of cycles taken at a fixed clock speed.  */
time_t 
time (time_t *t)
{
  uint64_t cycle;
  /* Get cycle count.  */
  CVMX_RDHWR (cycle, 31);
  cycle += rtc_cycles_from_epoch;
  if (t)
    *t = (time_t)cycle / octeon_cpu_clock_hz;

  return ((time_t)cycle / octeon_cpu_clock_hz);
}

/* Set the time as passed in struct timeval tv */
int 
settimeofday (const struct timeval *tv, const struct timezone *tz)
{
  uint64_t cycle, tv_cycles;
  /* Get cycle count.  */
  CVMX_RDHWR (cycle, 31);

  if (tv)
    {
      tv_cycles = (uint64_t)tv->tv_sec * octeon_cpu_clock_hz + ((uint64_t)tv->tv_usec * (octeon_cpu_clock_hz / (1000 * 1000))) % octeon_cpu_clock_hz;
    
      rtc_cycles_from_epoch = tv_cycles - cycle;
      return 0;
    }
  else
    {
      return -1;
    }
}

/* Get Process Times, P1003.1b-1993, p. 92.  */
struct tms {
  int tms_utime;              /* user time */
  int tms_stime;              /* system time */
  int tms_cutime;             /* user time, children */
  int tms_cstime;             /* system time, children */
};

/* Return an error for process consumption time as file I/O is not supported.  */  
int 
times (struct tms *buf)
{
  return -1;
}
