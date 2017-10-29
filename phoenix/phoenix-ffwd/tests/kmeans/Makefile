#------------------------------------------------------------------------------
# Copyright (c) 2007-2009, Stanford University
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of Stanford University nor the names of its 
#       contributors may be used to endorse or promote products derived from 
#       this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY STANFORD UNIVERSITY ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL STANFORD UNIVERSITY BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#------------------------------------------------------------------------------ 

# This Makefile requires GNU make.

HOME = ../..

include $(HOME)/Defines.mk

LIBS_RCL = -lpthread -L$(HOME)/$(LIB_DIR) -l$(PHOENIX_RCL) $(LIBLOCK) -lpthread -lpapi
LIBS += -L$(HOME)/$(LIB_DIR) -l$(PHOENIX) -lpthread

KMEANS_OBJS = kmeans.o
KMEANS_SEQ_OBJS = kmeans-seq.o
KMEANS_PTHREAD_OBJS = kmeans-pthread.o
KMEANS_RCL_OBJS = kmeans-rcl.o
KMEANS_PTHREAD_RCL_OBJS = kmeans-pthread-rcl.o

PROGS = kmeans-rcl kmeans-pthread-rcl kmeans kmeans-seq kmeans-pthread

.PHONY: default all clean

default: all

all: $(PROGS)

kmeans: $(KMEANS_OBJS) $(LIB_DEP)
	$(CC) $(CFLAGS) -o $@ $(KMEANS_OBJS) $(LIBS)

kmeans-seq: $(KMEANS_SEQ_OBJS)
	$(CC) $(CFLAGS) -o $@ $(KMEANS_SEQ_OBJS) $(LIBS)

kmeans-pthread: $(KMEANS_PTHREAD_OBJS)
	$(CC) $(CFLAGS) -o $@ $(KMEANS_PTHREAD_OBJS) $(LIBS)

kmeans-rcl: $(KMEANS_RCL_OBJS) $(LIB_DEP_RCL)
	$(CC) $(CFLAGS) -o $@ $(KMEANS_RCL_OBJS) $(LIBS_RCL)

kmeans-pthread-rcl: $(KMEANS_PTHREAD_RCL_OBJS) $(LIB_DEP_RCL)
	$(CC) $(CFLAGS) -o $@ $(KMEANS_PTHREAD_RCL_OBJS) $(LIBS_RCL)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ -I$(HOME)/$(INC_DIR)

clean:
	rm -f $(PROGS) $(KMEANS_OBJS) $(KMEANS_SEQ_OBJS) $(KMEANS_PTHREAD_OBJS) $(KMEANS_RCL_OBJS) $(KMEANS_PTHREAD_RCL_OBJS)
