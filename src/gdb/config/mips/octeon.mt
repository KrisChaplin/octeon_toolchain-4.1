# Target: Cavium Octeon processor
# No simulator objects or libraries are needed -- target uses oct-sim.

TDEPFILES = mips-tdep.o mips-mdebug-tdep.o remote-octeon.o remote-octeonpci.o \
  remote-run.o

DEPRECATED_TM_FILE = tm-octeon.h
