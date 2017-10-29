FFWD_DIR =../../include
LDLIB = -I$(FFWD_DIR)
NPROC=$(shell nproc)

CFLAGS = -O3 -g -Wall -mcmodel=large $(LDLIB) -DT$(NPROC)


all: ffwd_skiplist mcs_skiplist

ffwd_skiplist: $(FFWD_DIR)/libffwd.c skip_list.c
	gcc $(CFLAGS) -c -o libffwd.o $(FFWD_DIR)/libffwd.c -lnuma -lpthread -DFFWD -lrt
	gcc -g -O3 -Wall -DT$(NPROC) $(LDLIB) -c -o skip_list.o skip_list.c -lpthread -lnuma -DFFWD -lrt
	cc -g -O3 -o ffwd_skiplist libffwd.o skip_list.o -DT$(NPROC) -lpthread -lnuma -DFFWD -lrt

mcs_skiplist: skip_list.c
	gcc $(CFLAGS) -o mcs_skiplist skip_list.c -DMCS -lpthread -lnuma -lrt

clean:
	rm -f libffwd.o skip_list.o ffwd_skiplist mcs_skiplist
