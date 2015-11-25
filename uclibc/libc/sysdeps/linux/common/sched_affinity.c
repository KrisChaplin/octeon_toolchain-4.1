/* Ported from glibc. */

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <sys/types.h>
#include <alloca.h>
#include <sys/syscall.h>

#if (defined _ABIN32 && _MIPS_SIM == _ABIN32)

#define __NR___syscall_sched_setaffinity __NR_sched_setaffinity
#define __NR___syscall_sched_getaffinity __NR_sched_getaffinity

static inline _syscall3(int, __syscall_sched_setaffinity, __pid_t, pid, size_t, cpusetsize, const cpu_set_t *, cpuset);
static inline _syscall3(int, __syscall_sched_getaffinity, __pid_t, pid, size_t, cpusetsize, const cpu_set_t *, cpuset);

static size_t __kernel_cpumask_size;

int
sched_setaffinity (pid_t pid, size_t cpusetsize, const cpu_set_t *cpuset)
{
  size_t cnt;
  if (__builtin_expect (__kernel_cpumask_size == 0, 0))
    {
      int res;

      size_t psize = 128;
      void *p = alloca (psize);

      while (!(res = __syscall_sched_getaffinity (getpid (), psize, p)))
	p = alloca (2 * psize);

      if (res == 0)
	{
	  __set_errno (EFAULT);
	  return -1;
	}
      __kernel_cpumask_size = res;
    }

  /* We now know the size of the kernel cpumask_t.  Make sure the user
     does not request to set a bit beyond that.  */
  for (cnt = __kernel_cpumask_size; cnt < cpusetsize; ++cnt)
    if (((char *) cpuset)[cnt] != '\0')
      {
        /* Found a nonzero byte.  This means the user request cannot be
	   fulfilled.  */
	__set_errno (EINVAL);
	return -1;
      }

  return __syscall_sched_setaffinity (pid, cpusetsize, cpuset);
}

int
sched_getaffinity (pid_t pid, size_t cpusetsize, cpu_set_t *cpuset)
{
  int res = __syscall_sched_getaffinity (pid, sizeof (cpu_set_t), cpuset);
  if (res != -1)
    {
      /* Clean the rest of the memory the kernel didn't do.  */
      memset ((char *) cpuset + res, '\0', cpusetsize - res);

      res = 0;
    }
  return res;
}
#endif
