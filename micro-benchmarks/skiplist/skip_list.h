#define SKIPLIST_MAX_LEVEL 14
#define UPDATE_RATIO 300
#define ITERATION 1000000
#define MULTIPLIER 5003

typedef struct data
{
    int nr_ins;
    int nr_del;
    int nr_find;
    int id;
} data;

typedef struct snode {
    int key;
    int value;
    struct snode **forward;
} snode;

typedef struct skiplist {
    int level;
    struct snode *header;
} skiplist;

extern skiplist * list __attribute__((aligned(128)));