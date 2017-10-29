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

LIBS_RCL = -lpthread -L$(HOME)/$(LIB_DIR) -l$(PHOENIX_RCL) $(LIBLOCK) -lpapi -lrt
LIBS += -L$(HOME)/$(LIB_DIR) -l$(PHOENIX) -lrt

MM_OBJS = matrix_multiply.o
MM_SEQ_OBJS = matrix_multiply-seq.o
MM_PTHREAD_OBJS = matrix_multiply-pthread.o
MM_RCL_OBJS = matrix_multiply-rcl.o
MM_FFWD_OBJS = matrix_multiply-ffwd.o
MM_PTHREAD_RCL_OBJS = matrix_multiply-pthread-rcl.o

PROGS = matrix_multiply-ffwd matrix_multiply-rcl matrix_multiply-pthread-rcl matrix_multiply matrix_multiply-seq matrix_multiply-pthread

.PHONY: default all clean

default: all

all: $(PROGS)

matrix_multiply: $(MM_OBJS) $(LIB_DEP)
	$(CC) $(CFLAGS) -o $@ $(MM_OBJS) $(LIBS)

matrix_multiply-seq: $(MM_SEQ_OBJS)
	$(CC) $(CFLAGS) -o $@ $(MM_SEQ_OBJS) $(LIBS)

matrix_multiply-pthread: $(MM_PTHREAD_OBJS)
	$(CC) $(CFLAGS) -o $@ $(MM_PTHREAD_OBJS) $(LIBS)

matrix_multiply-rcl: $(MM_RCL_OBJS) $(LIB_DEP)
	$(CC) $(CFLAGS) -o $@ $(MM_RCL_OBJS) $(LIBS_RCL)

matrix_multiply-ffwd: $(MM_FFWD_OBJS) $(LIB_DEP)
	$(CC) $(CFLAGS) -DFFWD -o $@ $(MM_FFWD_OBJS) -lpthread $(HOME)/$(LIB_DIR)/libphoenix_ffwd.a $(LIBLOCK) -lpapi -lrt

matrix_multiply-pthread-rcl: $(MM_PTHREAD_RCL_OBJS)
	$(CC) $(CFLAGS) -o $@ $(MM_PTHREAD_RCL_OBJS) $(LIBS_RCL)

matrix_multiply-ffwd.o: matrix_multiply-rcl.c
	$(CC) $(CFLAGS) -DFFWD -c $< -o $@ -I$(HOME)/$(INC_DIR)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ -I$(HOME)/$(INC_DIR)

clean:
	rm -f $(PROGS) $(MM_OBJS) $(MM_SEQ_OBJS) $(MM_PTHREAD_OBJS) $(MM_RCL_OBJS) $(MM_PTHREAD_RCL_OBJS) $(MM_FFWD_OBJS)
