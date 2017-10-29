CC = gcc
NPROC = $(shell nproc)
CFLAGSX86 = -pipe -O3 -ftree-vectorize -ftree-vectorizer-verbose=0 -msse3 -march=native -mtune=native -finline-functions -lpthread -DN_THREADS=$(nthreads) -DUSE_CPUS=$(NPROC) -D_GNU_SOURCE

FFWD_DIR =../../include
LDLIB = -I$(FFWD_DIR)

OS =$(shell uname -s)

X86:
	gcc -O3 -Wall -g $(LDLIB) $(CFLAGSX86) -c -o locksstack.o locksstack.c -DFFWD;
	gcc -O3 -Wall -g $(LDLIB) -DT$(NPROC) -mcmodel=large -c -o libffwd.o $(FFWD_DIR)/libffwd.c -lnuma -lpthread -DFFWD;
	cc -O3 $(LDLIB) -DT$(NPROC) -o LocksStack.run.ffwd libffwd.o locksstack.o -lpthread -lnuma -DFFWD;

	gcc -O3 -Wall $(LDLIB) -g $(CFLAGSX86) -c -o locksqueue.o locksqueue.c -DFFWD;
	gcc -O3 -Wall $(LDLIB) -g -DT$(NPROC) -mcmodel=large -c -o libffwd.o $(FFWD_DIR)/libffwd.c -lnuma -lpthread -DFFWD;
	cc -O3 $(LDLIB) -DT$(NPROC) -o LocksQueue.run.ffwd libffwd.o locksqueue.o -lpthread -lnuma -DFFWD;

# A Fecth&Multiply, a stack and a queue object implemented with Oyama, et al method
	$(CC) $(CFLAGSX86) oyamauobject.c -o OyamaUObject.run -lpthread
# A Fecth&Multiply, a stack and a queue object based on P-Sim universal construction
	$(CC) $(CFLAGSX86) sim.c -o Sim.run -lpthread
	$(CC) $(CFLAGSX86) simstack.c -o SimStack.run -lpthread
	$(CC) $(CFLAGSX86) simqueue.c -o SimQueue.run -lpthread
# A Fecth&Multiply, a stack and a queue object based on CC-Synch universal construction
	$(CC) $(CFLAGSX86) ccuobject.c -o ccUObject.run -lpthread
	$(CC) $(CFLAGSX86) ccstack.c -o ccStack.run -lpthread
	$(CC) $(CFLAGSX86) ccqueue.c -o ccQueue.run -lpthread
# A Fecth&Multiply, a stack and a queue object based on DSM-Synch universal construction
	$(CC) $(CFLAGSX86) dsmuobject.c -o dsmUObject.run -lpthread
	$(CC) $(CFLAGSX86) dsmstack.c -o dsmStack.run -lpthread
	$(CC) $(CFLAGSX86) dsmqueue.c -o dsmQueue.run -lpthread
# A lock-free Fecth&Multiply, a lock-free stack and a lock-free queue object 
	$(CC) $(CFLAGSX86) lfobject.c -o LFobject.run -lpthread
	$(CC) $(CFLAGSX86) lfstack.c -o LFStack.run -lpthread
	$(CC) $(CFLAGSX86) msqueue.c -o MSQueue.run -lpthread
# A Fecth&Multiply, a stack and a queue object implemented with high performance spinlocks
	$(CC) $(CFLAGSX86) locksuobject.c -o LocksUobject.run -lpthread
	$(CC) $(CFLAGSX86) locksstack.c -o LocksStack.run -lpthread
	$(CC) $(CFLAGSX86) locksstack.c -o LocksStack.run.spin -lpthread -DSPIN
	$(CC) $(CFLAGSX86) locksstack.c -o LocksStack.run.mcs -lpthread -DMCS
	$(CC) $(CFLAGSX86) locksstack.c -o LocksStack.run.ticket -lpthread -DTICKET
	$(CC) $(CFLAGSX86) locksstack.c -o LocksStack.run.mutex -lpthread -DMUTEX
	$(CC) $(CFLAGSX86) locksstack.c -o LocksStack.run.flat -lpthread liblock.c flatcombining.c -DFLAT -lpthread -lnuma -DFLATRCL -DTYPE_NOINFO=TYPE_FLATCOMBINING
	$(CC) $(CFLAGSX86) locksstack.c -o LocksStack.run.rcl -lpthread liblock.c rcl.c -DRCL -lpthread -lnuma -DFLATRCL -DTYPE_NOINFO=TYPE_RCL

	$(CC) $(CFLAGSX86) locksqueue.c -o LocksQueue.run -lpthread
	$(CC) $(CFLAGSX86) locksqueue.c -o LocksQueue.run.spin -lpthread -DSPIN
	$(CC) $(CFLAGSX86) locksqueue.c -o LocksQueue.run.mcs -lpthread -DMCS
	$(CC) $(CFLAGSX86) locksqueue.c -o LocksQueue.run.ticket -lpthread -DTICKET
	$(CC) $(CFLAGSX86) locksqueue.c -o LocksQueue.run.mutex -lpthread -DMUTEX
	$(CC) $(CFLAGSX86) locksqueue.c -o LocksQueue.run.flat -lpthread liblock.c flatcombining.c -DFLAT -lpthread -lnuma -DFLATRCL -DTYPE_NOINFO=TYPE_FLATCOMBINING
	$(CC) $(CFLAGSX86) locksqueue.c -o LocksQueue.run.rcl -lpthread liblock.c rcl.c -DRCL -lpthread -lnuma -DFLATRCL -DTYPE_NOINFO=TYPE_RCL

# Hierarchical universal constructions
	$(CC) $(CFLAGSX86) huobject.c -o hUObject.run -lpthread
	$(CC) $(CFLAGSX86) hstack.c -o hStack.run -lpthread
	$(CC) $(CFLAGSX86) hqueue.c -o hQueue.run -lpthread
	
clean:
	rm -f *.o *.run *.run.* *~ a.out res.txt

