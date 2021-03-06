#include <stdbool.h>
#include <stdio.h>

/* Internal definitions used in the libc code.  */
#define __getservbyname_r getservbyname_r
#define __socket socket
#define __getsockname getsockname
#define __inet_aton inet_aton
#define __gethostbyaddr_r gethostbyaddr_r
#define __gethostbyname2_r gethostbyname2_r

void
attribute_hidden
__check_pf (bool *p1, bool *p2)
{
  *p1 = *p2 = true;
}
int
__idna_to_ascii_lz (const char *input, char **output, int flags)
{
  return 0;
}
int
__idna_to_unicode_lzlz (const char *input, char **output, int flags)
{
  return 0;
}

#include "../sysdeps/posix/getaddrinfo.c"

service_user *__nss_hosts_database attribute_hidden;


/* This is the beginning of the real test code.  The above defines
   (among other things) the function rfc3484_sort.  */


#if __BYTE_ORDER == __BIG_ENDIAN
# define h(n) n
#else
# define h(n) __bswap_constant_32 (n)
#endif

struct sockaddr_in addrs[] =
{
  { .sin_family = AF_INET, .sin_addr = { h (0xc0a86d1d) } },
  { .sin_family = AF_INET, .sin_addr = { h (0xc0a85d03) } },
  { .sin_family = AF_INET, .sin_addr = { h (0xc0a82c3d) } },
  { .sin_family = AF_INET, .sin_addr = { h (0xc0a86002) } },
  { .sin_family = AF_INET, .sin_addr = { h (0xc0a802f3) } },
  { .sin_family = AF_INET, .sin_addr = { h (0xc0a80810) } },
  { .sin_family = AF_INET, .sin_addr = { h (0xc0a85e02) } }
};
#define naddrs (sizeof (addrs) / sizeof (addrs[0]))
static struct addrinfo ais[naddrs];
static struct sort_result results[naddrs];

static int expected[naddrs] =
  {
    6, 1, 0, 3, 2, 4, 5
  };


static int
do_test (void)
{
  struct sockaddr_in so;
  so.sin_family = AF_INET;
  so.sin_addr.s_addr = h (0xc0a85f19);

  for (int i = 0; i < naddrs; ++i)
    {
      ais[i].ai_family = AF_INET;
      ais[i].ai_addr = (struct sockaddr *) &addrs[i];
      results[i].dest_addr = &ais[i];
      results[i].got_source_addr = true;
      memcpy(&results[i].source_addr, &so, sizeof (so));
      results[i].source_addr_len = sizeof (so);
    }

  qsort (results, naddrs, sizeof (results[0]), rfc3484_sort);

  int result = 0;
  for (int i = 0; i < naddrs; ++i)
    {
      struct in_addr addr = ((struct sockaddr_in *) (results[i].dest_addr->ai_addr))->sin_addr;

      int here = memcmp (&addr, &addrs[expected[i]].sin_addr,
			 sizeof (struct in_addr));
      printf ("[%d] = %s: %s\n", i, inet_ntoa (addr), here ? "FAIL" : "OK");
      result |= here;
    }

  return result;
}

#define TEST_FUNCTION do_test ()
#include "../test-skeleton.c"
