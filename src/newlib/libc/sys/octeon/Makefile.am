## Process this file with automake to generate Makefile.in

AUTOMAKE_OPTIONS = cygnus

INCLUDES = $(NEWLIB_CFLAGS) $(CROSS_CFLAGS) $(TARGET_CFLAGS)

noinst_LIBRARIES = lib.a

extra_objs =

lib_a_SOURCES = octeon-app-init.c octeon-os.c octeon-uart.c crt0.S
lib_a_LIBADD = $(extra_objs)
lib_a_DEPENDENCIES = $(extra_objs)

ACLOCAL_AMFLAGS = -I ../../..
CONFIG_STATUS_DEPENDENCIES = $(newlib_basedir)/configure.host
