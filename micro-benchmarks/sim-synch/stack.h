#include "system.h"
#define PSIZE                   4096
#define Object                     int32_t

typedef struct HalfPoolStruct {
    void *p[PSIZE];
    int index;
    int obj_size;
} HalfPoolStruct;

typedef struct PoolStruct {
    void *p[PSIZE];
    int index;
    int obj_size;
    int32_t align[PAD_CACHE(sizeof(HalfPoolStruct))];
} PoolStruct;

typedef struct ListNode {
    Object value;		      // initially, there is a sentinel node 
    volatile struct ListNode *next; 	        // in the queue where Head and Tail point to.
} ListNode;	

void init_pool(PoolStruct *pool, int obj_size);

extern PoolStruct *pool_node_server;