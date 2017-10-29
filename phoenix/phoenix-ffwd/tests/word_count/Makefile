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

LIBS_RCL = -lpthread -L$(HOME)/$(LIB_DIR) -l$(PHOENIX_RCL) $(LIBLOCK) -lpapi
LIBS += -L$(HOME)/$(LIB_DIR) -l$(PHOENIX)

WC_OBJS := word_count.o sort.o
WC_OBJS_SEQ := word_count-seq.o
WC_OBJS_PTHREAD := word_count-pthread.o sort-pthread.o
WC_RCL_OBJS := sort.o word_count-rcl.o
WC_OBJS_RCL_PTHREAD := word_count-pthread-rcl.o sort-pthread-rcl.o

PROGS := word_count-rcl word_count-pthread-rcl word_count word_count-pthread word_count-seq

.PHONY: default all clean

default: all

all: $(PROGS)

word_count: $(WC_OBJS) $(LIB_DEP)
	$(CC) $(CFLAGS) -o $@ $(WC_OBJS) $(LIBS)

word_count-pthread: $(WC_OBJS_PTHREAD)
	$(CC) $(CFLAGS) -o $@ $(WC_OBJS_PTHREAD) $(LIBS)

word_count-seq: $(WC_OBJS_SEQ)
	$(CC) $(CFLAGS) -o $@ $(WC_OBJS_SEQ) $(LIBS)

word_count-rcl: $(WC_RCL_OBJS) $(LIB_DEP)
	$(CC) $(CFLAGS) -o $@ $(WC_RCL_OBJS) $(LIBS_RCL)

word_count-pthread-rcl: $(WC_OBJS_RCL_PTHREAD)
	$(CC) $(CFLAGS) -o $@ $(WC_OBJS_RCL_PTHREAD) $(LIBS_RCL)

%.o: %.c *.h
	$(CC) $(CFLAGS) -c $< -o $@ -I$(HOME)/$(INC_DIR)

clean:
	rm -f $(PROGS) $(WC_OBJS) $(WC_OBJS_PTHREAD) $(WC_OBJS_SEQ) $(WC_RCL_OBJS) $(WC_OBJS_RCL_PTHREAD)
