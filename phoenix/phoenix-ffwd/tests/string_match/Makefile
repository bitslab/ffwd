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

STR_MATCH_OBJS = string_match.o
STR_MATCH_SEQ_OBJS = string_match-seq.o
STR_MATCH_PTHREAD_OBJS = string_match-pthread.o
STR_MATCH_RCL_OBJS = string_match-rcl.o
STR_MATCH_FFWD_OBJS = string_match-ffwd.o
STR_MATCH_PTHREAD_RCL_OBJS = string_match-pthread-rcl.o

PROGS = string_match-ffwd string_match-rcl string_match-pthread-rcl string_match string_match-seq string_match-pthread

.PHONY: default all clean

default: all

all: $(PROGS)

string_match: $(STR_MATCH_OBJS) $(LIB_DEP)
	$(CC) $(CFLAGS) -o $@ $(STR_MATCH_OBJS) $(LIBS)

string_match-seq: $(STR_MATCH_SEQ_OBJS)
	$(CC) $(CFLAGS) -o $@ $(STR_MATCH_SEQ_OBJS) $(LIBS)

string_match-pthread: $(STR_MATCH_PTHREAD_OBJS)
	$(CC) $(CFLAGS) -o $@ $(STR_MATCH_PTHREAD_OBJS) $(LIBS)

string_match-rcl: $(STR_MATCH_RCL_OBJS) $(LIB_DEP)
	$(CC) $(CFLAGS) -o $@ $(STR_MATCH_RCL_OBJS) $(LIBS_RCL)

string_match-ffwd: $(STR_MATCH_FFWD_OBJS) $(LIB_DEP)
	$(CC) $(CFLAGS) -DFFWD -o $@ $(STR_MATCH_FFWD_OBJS) -lpthread $(HOME)/$(LIB_DIR)/libphoenix_ffwd.a $(LIBLOCK) -lpapi -lrt

string_match-ffwd.o: string_match-rcl.c
	$(CC) $(CFLAGS) -DFFWD -c $< -o $@ -I$(HOME)/$(INC_DIR)

string_match-pthread-rcl: $(STR_MATCH_PTHREAD_RCL_OBJS)
	$(CC) $(CFLAGS) -o $@ $(STR_MATCH_PTHREAD_RCL_OBJS) $(LIBS_RCL)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ -I$(HOME)/$(INC_DIR)

clean:
	rm -f $(PROGS) $(STR_MATCH_OBJS) $(STR_MATCH_SEQ_OBJS) $(STR_MATCH_PTHREAD_OBJS) $(STR_MATCH_RCL_OBJS) $(STR_MATCH_FFWD_OBJS) $(STR_MATCH_PTHREAD_RCL_OBJS)
