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

L_REG_OBJS = linear_regression.o
L_REG_SEQ_OBJS = linear_regression-seq.o
L_REG_PTHREAD_OBJS = linear_regression-pthread.o
L_REG_RCL_OBJS = linear_regression-rcl.o
L_REG_FFWD_OBJS = linear_regression-ffwd.o
L_REG_PTHREAD_RCL_OBJS = linear_regression-pthread-rcl.o

PROGS = linear_regression-ffwd linear_regression-rcl linear_regression-pthread-rcl linear_regression linear_regression-seq linear_regression-pthread

.PHONY: default all clean

default: all

all: $(PROGS)

linear_regression: $(L_REG_OBJS) $(LIB_DEP)
	$(CC) $(CFLAGS) -o $@ $(L_REG_OBJS) $(LIBS)

linear_regression-seq: $(L_REG_SEQ_OBJS)
	$(CC) $(CFLAGS) -o $@ $(L_REG_SEQ_OBJS) $(LIBS)

linear_regression-pthread: $(L_REG_PTHREAD_OBJS)
	$(CC) $(CFLAGS) -o $@ $(L_REG_PTHREAD_OBJS) $(LIBS)

linear_regression-rcl: $(L_REG_RCL_OBJS) $(LIB_DEP)
	$(CC) $(CFLAGS) -o $@ $(L_REG_RCL_OBJS) $(LIBS_RCL)

linear_regression-ffwd: $(L_REG_FFWD_OBJS) $(LIB_DEP)
	$(CC) $(CFLAGS) -DFFWD -o $@ $(L_REG_FFWD_OBJS) -lpthread $(HOME)/$(LIB_DIR)/libphoenix_ffwd.a $(LIBLOCK) -lpapi -lrt

linear_regression-ffwd.o: linear_regression-rcl.c
	$(CC) $(CFLAGS) -DFFWD -c $< -o $@ -I$(HOME)/$(INC_DIR)

linear_regression-pthread-rcl: $(L_REG_PTHREAD_RCL_OBJS)
	$(CC) $(CFLAGS) -o $@ $(L_REG_PTHREAD_RCL_OBJS) $(LIBS_RCL)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ -I$(HOME)/$(INC_DIR)

clean:
	rm -f $(PROGS) $(L_REG_OBJS) $(L_REG_SEQ_OBJS) $(L_REG_PTHREAD_OBJS) $(L_REG_RCL_OBJS) $(L_REG_PTHREAD_RCL_OBJS) $(L_REG_FFWD_OBJS)
