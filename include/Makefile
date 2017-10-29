SHELL = /bin/bash

CFLAGS = -O3 -g
LIBS = -lnuma -lpthread -lrt
MCMODEL = -mcmodel=large
ARCH = $(shell ./platform.sh)

NODES=/sys/devices/system/node

NPROC=$(shell nproc)
N_SOCK=$(shell ./num_sock.sh)
$(eval clients=$(shell echo $$((($(NPROC)/$(N_SOCK))-1))))

# Core ordering:
# STC : socket-filling
# CTS (default case) : core-filling

all: ffwd_sample

libffwd.o: libffwd.c ffwd.h
	gcc $(CFLAGS) -Wall $(MCMODEL) -DT$(NPROC) -c -o libffwd.o libffwd.c $(LIBS);

ffwd_sample: libffwd.o
	gcc $(CFLAGS) -DT$(NPROC) -o ffwd_sample libffwd.o ffwd_sample.c $(LIBS);

clean:
	rm -f libffwd.o ffwd_sample
