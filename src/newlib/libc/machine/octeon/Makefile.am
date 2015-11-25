## Process this file with automake to generate Makefile.in

AUTOMAKE_OPTIONS = cygnus

INCLUDES = $(NEWLIB_CFLAGS) $(CROSS_CFLAGS) $(TARGET_CFLAGS)

noinst_LIBRARIES = lib.a

lib_a_SOURCES = setjmp.S strcmp.c strlen.c strncpy.c memset.c memcpy.c

ACLOCAL_AMFLAGS = -I ../../..
CONFIG_STATUS_DEPENDENCIES = $(newlib_basedir)/configure.host

setjmp.S: $(srcdir)/../mips/setjmp.S
	cp -f $^ $@

strncpy.c: $(srcdir)/../mips/strncpy.c
	cp -f $^ $@
