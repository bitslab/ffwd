
-include .root
BASEDIR?=$(PWD)
ROOT=$(BASEDIR)/../../

NB_CORES=$(shell cat /proc/cpuinfo | grep processor | wc -l)
SPLASH=splash2

CC := gcc
CFLAGS := -g -O3 -pthread -D_POSIX_C_SOURCE=200112 -I$(BASEDIR) -I$(ROOT)/liblock
#CFLAGS := $(CFLAGS) -Wall -W -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -Wdisabled-optimization
#CFLAGS := $(CFLAGS) -Wpadded -Winline -Wpointer-arith -Wsign-compare -Wendif-labels -g

LDFLAGS := -lm -llock -rdynamic

-include $(ROOT)/liblock/Makefile.config

OBJS += $(BASEDIR)/liblock-config.o

MACROS := $(BASEDIR)/$(SPLASH)/codes/null_macros/c.m4.null
## +EDIT
#M4 := m4 -s -Ulen -Uindex
# We disable synclines because of inaccuracy issues leading several lock declarations to be mapped to the same
# line by addr2line.
M4 := m4 -Ulen -Uindex
## -EDIT

x = *

APP_DIRS=\
	apps/barnes \
	apps/ocean/non_contiguous_partitions \
	apps/volrend \
	apps/raytrace \
	apps/water-nsquared \
  apps/water-spatial \
	apps/ocean/contiguous_partitions \
	apps/radiosity/glibdumb \
	apps/radiosity/glibps \
	apps/radiosity  \
	apps/fmm

BUILD_DIRS=$(APP_DIRS) \
	kernels/fft \
	kernels/lu/contiguous_blocks \
	kernels/lu/non_contiguous_blocks \
	kernels/cholesky \

#	kernels/radix

.PHONY: all bootstrap tidy clean distclean
.SECONDARY: 
.SUFFIXES:

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $^ $(CFLAGS) -o $(TARGET) $(LDFLAGS)

#build: .root $(SPLASH) $(patsubst %, $(BASEDIR)/$(SPLASH)/codes/%/.build, $(BUILD_DIRS))

rebuild: $(SPLASH)
	rm -f $(BASEDIR)/$(DO)/.build &&	$(MAKE) -j$(NB_CORES) $(BASEDIR)/$(DO)/.build

rebuild-all:
	for F in $(APP_DIRS); do make rebuild DO=$(SPLASH)/codes/$$F; done

#run: build
#	./run.sh

$(BASEDIR)/$(SPLASH)/%/.build: .liblock
	touch $(dir $@)/*.[ch]
	$(MAKE) -j$(NB_CORES) -C $(dir $@) && touch $@

.liblock: $(wildcard $(LIBLOCK)/*.[ch])
	$(MAKE) -j$(NB_CORES) -C $(ROOT)/liblock
	touch $@

$(BASEDIR)/liblock-config.o: $(BASEDIR)/liblock-config.c
	$(CC) -c $^ $(CFLAGS) -o $@

.root: Makefile
	echo "BASEDIR=$(BASEDIR)" > $@

splash2: $(ROOT)/liblock/splash2.out
	rm -Rf $(BASEDIR)/$@
	cp -Rf $(ROOT)/splash2/splash2-base $@

	cd $@ && patch -p1 < $<; cd ..

	rm -f $@/codes/Makefile.config
	ln -s $(BASEDIR)/Makefile $(BASEDIR)/$@/codes/Makefile.config

splash2-base: 
	rm -Rf $(BASEDIR)/$@
	cp -Rf $(ROOT)/splash2/$@ .
	for F in $$(find $@ -iname "*.c"); do cat $$F | sed -e 's/pthread_create/splash2_thread_create/g' > $$F.tmp; \mv $$F.tmp $$F; done
	rm -f $@/codes/Makefile.config
	ln -s $(BASEDIR)/Makefile $(BASEDIR)/$@/codes/Makefile.config

clean:
	rm -rf splash2 .root

distclean: clean
	rm -Rf splash2-base

.SUFFIXES:
.SUFFIXES:	.o .c .C .h .H

.H.h:
	$(M4) $(MACROS) $*.H > $*.h

.C.c:
	$(M4) $(MACROS) $*.C > $*.c

.c.o:
	$(CC) -c $(CFLAGS) $*.c

.C.o:
	$(M4) $(MACROS) $*.C > $*.c
	$(CC) -c $(CFLAGS) $*.c

tidy:
	rm -f *~ \#*
