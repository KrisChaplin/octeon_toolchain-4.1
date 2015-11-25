PREFIX = $(shell pwd)/tools
TARGET = mips64-octeon-linux-gnu
SYSROOT = $(PREFIX)/$(TARGET)/sys-root

CORE_PREFIX = $(shell pwd)/core_tools
CORE_SYSROOT = $(CORE_PREFIX)/$(TARGET)/sys-root

STRIP = $(PREFIX)/bin/$(TARGET)-strip

HEADER_DIR = $(shell pwd)/linux-headers/include

STATIC_NSS = $(shell find /usr/lib* -name libnss_files.a)
ifneq (,$(findstring libnss,$(STATIC_NSS)))
  STATIC_NSS_LIBS = -lc -lnss_files -lnss_dns -lresolv
else
  STATIC_NSS_LIBS =
endif

CONFIGURE_OCTEON_HOST = maybe-configure-bfd maybe-configure-opcodes \
	maybe-configure-binutils maybe-configure-gas maybe-configure-gprof \
	maybe-configure-intl maybe-configure-ld

.PHONY: linux_release
linux_release: build_minimal_gcc build-uclibc build_glibc build_glibc-n32 \
  build_gcc

.PHONY: build_minimal_gcc
# Build core compiler, linker and assembler.
build_minimal_gcc:
	mkdir -p $(SYSROOT)/lib
	mkdir -p $(SYSROOT)/usr/lib
	mkdir -p $(SYSROOT)/usr/include
	#ln -sf asm-mips64 $(HEADER_DIR)/asm
	cp -r $(HEADER_DIR)/* $(SYSROOT)/usr/include
	cd $(SYSROOT)/usr/include; cp asm/mach-generic/*.h .
	cd $(SYSROOT)/usr/include; cp asm/mach-mips/*.h .
	cd $(SYSROOT)/usr/include; cp asm/mach-cavium-octeon/*.h .
	mkdir -p $(CORE_PREFIX)
	rm -rf $(SYSROOT)/usr/include/asm/mach-*
	mkdir -p $(CORE_SYSROOT)/usr
	cp -r $(SYSROOT)/usr/include $(CORE_SYSROOT)/usr 
	mkdir -p $(CORE_SYSROOT)/uclibc
	ln -sf ../usr $(CORE_SYSROOT)/uclibc
	mkdir -p build_core_linux
	cd build_core_linux; \
	../src/configure --target=$(TARGET) \
		--prefix=$(CORE_PREFIX) \
		--with-local-prefix=$(CORE_SYSROOT) \
		--with-newlib \
		--enable-threads=no \
		--enable-symvers=gnu \
		--enable-__cxa_atexit \
		--enable-languages=c \
		--disable-shared \
		--with-sysroot=$(CORE_SYSROOT)
	$(MAKE) -C build_core_linux $(CONFIGURE_OCTEON_HOST)
	$(MAKE) -C build_core_linux LDFLAGS=-all-static all-gas all-binutils
	$(MAKE) -C build_core_linux LDFLAGS="-all-static $(STATIC_NSS_LIBS)" all-ld
	$(MAKE) -C build_core_linux install-gas install-ld install-binutils
	$(MAKE) -C build_core_linux configure-host
	$(MAKE) -C build_core_linux LDFLAGS=-static all-gcc
	$(MAKE) -C build_core_linux LDFLAGS=-static install-gcc 

.PHONY: build_glibc
build_glibc:
	$(CORE_PREFIX)/bin/$(TARGET)-gcc -mabi=64 -print-libgcc-file-name \
	  | (read i; ln -sf $$i $${i%.a}_eh.a)
	mkdir -p build_glibc
	cd build_glibc; \
	echo "libc_cv_forced_unwind=yes" > config.cache; \
	echo "libc_cv_c_cleanup=yes" >> config.cache; \
	$(shell pwd)/glibc/configure --prefix=/usr \
		--build=i386-linux --host=$(TARGET) \
		--enable-kernel=2.6.13 \
		--without-cvs --disable-profile \
		--disable-debug --without-gd \
		--enable-shared \
		--enable-static-nss \
		--enable-add-ons=nptl,ports \
		--with-headers=$(CORE_SYSROOT)/usr/include \
		--nfp --with-fp=no --cache-file=config.cache \
		CC="$(CORE_PREFIX)/bin/$(TARGET)-gcc -mabi=64 -O2 -march=octeon"
	$(MAKE) -j${NUM_PARALLEL} -C build_glibc all
	$(MAKE) -C build_glibc install_root=$(SYSROOT) install

.PHONY: build_glibc-n32
build_glibc-n32:
	$(CORE_PREFIX)/bin/$(TARGET)-gcc -mabi=n32 -print-libgcc-file-name \
	  | (read i; ln -sf $$i $${i%.a}_eh.a)
	mkdir -p build_glibc-n32
	cd build_glibc-n32; \
	echo "libc_cv_forced_unwind=yes" > config.cache; \
	echo "libc_cv_c_cleanup=yes" >> config.cache; \
	$(shell pwd)/glibc/configure --prefix=/usr \
		--build=i386-linux --host=$(TARGET) \
		--without-cvs --disable-profile \
		--disable-debug --without-gd \
		--enable-kernel=2.6.13 \
		--enable-shared \
		--enable-static-nss \
		--enable-add-ons=nptl,ports \
		--with-headers=$(CORE_SYSROOT)/usr/include \
		--nfp --with-fp=no --cache-file=config.cache \
		CC="$(CORE_PREFIX)/bin/$(TARGET)-gcc -mabi=n32 -O2 -march=octeon"
	$(MAKE) -j${NUM_PARALLEL} -C build_glibc-n32 all
	$(MAKE) -C build_glibc-n32 install_root=$(SYSROOT) install

.PHONY: build-uclibc clean-uclibc
build-uclibc:
	test -f uclibc/.config || cp -f uclibc/octeon.config uclibc/.config
	sed -e "s,^KERNEL_SOURCE=.*,KERNEL_SOURCE=\"`pwd`/linux-headers\",g" \
	  -i uclibc/.config
	# Set DODEBUG to y to compile with -g.
	make -C uclibc DODEBUG=$(DODEBUG) \
	  CC="$(CORE_PREFIX)/bin/$(TARGET)-gcc -mabi=n32" \
	  CROSS="$(CORE_PREFIX)/bin/$(TARGET)-" \
	  LD="$(CORE_PREFIX)/bin/$(TARGET)-ld -melf32btsmipn32"
	make -C uclibc install \
	  PREFIX=$(SYSROOT) DEVEL_PREFIX=/uclibc/usr/ RUNTIME_PREFIX=/uclibc/

clean-uclibc:
	make -C uclibc clean

#Build final compiler
build_linux/Makefile:
	mkdir -p build_linux
	cd build_linux;  \
	  ../src/configure --target=$(TARGET) --prefix=$(PREFIX) \
		--with-local-prefix=$(SYSROOT) \
		--disable-sim \
		--enable-symvers=gnu \
		--enable-__cxa_atexit \
		--enable-languages=c,c++ \
		--with-sysroot

.PHONY: build_gcc
build_gcc: build_linux/Makefile
	$(MAKE) -C build_linux $(CONFIGURE_OCTEON_HOST)
	$(MAKE) -C build_linux LDFLAGS=-all-static all-gas all-binutils
	$(MAKE) -C build_linux LDFLAGS="-all-static $(STATIC_NSS_LIBS)" all-ld 
	$(MAKE) -C build_linux configure-host
	$(MAKE) -C build_linux LDFLAGS=-static all-gcc
	$(MAKE) -C build_linux LDFLAGS=-static NAT_CLIBS="$(STATIC_NSS_LIBS)" all-gdb
	$(MAKE) -C build_linux all-target-libstdc++-v3
	$(MAKE) -C build_linux all-target-libiberty
	$(MAKE) -C build_linux install-gas install-ld install-binutils
	$(MAKE) -C build_linux LDFLAGS=-static install-gcc install-gdb
	$(MAKE) -C build_linux all-target-libstdc++-v3 all-target-libiberty \
	   install-target-libstdc++-v3 install-target-libiberty
	cp $(shell pwd)/src/newlib/libc/sys/octeon/octeon-app-init.h $(PREFIX)/lib/gcc/$(TARGET)/*/include
	cd $(SYSROOT)/../uclibc; $(STRIP) *.so.*; rm *.la
	cd $(SYSROOT)/../uclibc; mv libstdc++*.so* libgcc_s*.so* $(SYSROOT)/uclibc/lib
	- strip `find $(PREFIX)/bin -type f -perm +111 | xargs file | grep -v Bourne | cut -d: -f1`
	- strip `find $(PREFIX)/$(TARGET)/bin -type f -perm +111 | xargs file | grep -v Bourne | cut -d: -f1`
	- $(STRIP) `find $(SYSROOT)/usr/lib*/gconv -type f -perm +111 | xargs file | grep -v Bourne | cut -d: -f1`
	- strip `find $(PREFIX)/libexec -type f -perm +111 | xargs file | grep -v Bourne | cut -d: -f1`
	- $(STRIP) `find $(SYSROOT)/usr/bin -type f -perm +111 | xargs file | grep -v script | cut -d: -f1`
	- $(STRIP) `find $(SYSROOT)/sbin -type f -perm +111 | xargs file | grep -v Bourne | cut -d: -f1`
	- $(STRIP) `find $(SYSROOT)/usr/sbin -type f -perm +111 | xargs file | grep -v Bourne | cut -d: -f1`

build-linux-native/Makefile:
	mkdir -p build-linux-native
	cd build-linux-native; \
	  ../src/configure --host=$(TARGET) --prefix=/usr --build=i386-linux \
	  --enable-symvers=gnu --enable-__cxa_atexit --enable-languages=c,c++ \
	  --enable-shared --disable-sim --disable-tui --disable-gdbtk

build_termcap:
	rm -rf termcap-1.3.1
	tar -xzf $(shell pwd)/termcap-1.3.1.tar.gz
	cd termcap-1.3.1; \
	./configure --host=$(TARGET) \
		--prefix=/usr \
		--build=i386-linux \
		--enable-shared
	# build termcap for n64 ABI
	$(MAKE) -C termcap-1.3.1 CC=$(TARGET)-gcc AR=$(TARGET)-ar RANLIB=$(TARGET)-ranlib all
	cp termcap-1.3.1/termcap.h $(SYSROOT)/usr/include
	cp termcap-1.3.1/libtermcap.a $(SYSROOT)/usr/lib64
	# build termcap for n32 ABI
	$(MAKE) -C termcap-1.3.1 clean;
	$(MAKE) -C termcap-1.3.1 CC="$(TARGET)-gcc -mabi=n32" AR=$(TARGET)-ar RANLIB=$(TARGET)-ranlib all
	cp termcap-1.3.1/libtermcap.a $(SYSROOT)/usr/lib32
	# build termcap for uclibc
	$(MAKE) -C termcap-1.3.1 clean;
	$(MAKE) -C termcap-1.3.1 CC="$(TARGET)-gcc -muclibc" AR=$(TARGET)-ar RANLIB=$(TARGET)-ranlib all
	cp termcap-1.3.1/termcap.h $(SYSROOT)/uclibc/usr/include
	cp termcap-1.3.1/libtermcap.a $(SYSROOT)/uclibc/usr/lib
