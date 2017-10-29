#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <numa.h>
#include <getopt.h>
#include "skip_list.h"
#include "ffwd.h"
#include "locks.h"

skiplist * list __attribute__((aligned(128)));
unsigned long list_size;
char dummy[128];
int levelmax;
unsigned long range;

char _dummy[128];
static volatile int stop;

static int rand_level() {
    int level = 1;
    while (rand() < RAND_MAX / 2 && level < levelmax)
        level++;
    return level;
}

skiplist *skiplist_init() {
    int i;
    snode *header = (snode *) malloc(sizeof(struct snode));
    list->header = header;
    header->key = INT_MAX;
    header->forward = (snode **) malloc(
            sizeof(snode*) * (levelmax + 1));
    for (i = 0; i <= levelmax; i++) {
        header->forward[i] = list->header;
    }

    list->level = 1;

    return list;
}

int skiplist_insert_init(int key, int value) {
    snode *update[SKIPLIST_MAX_LEVEL + 1];
    snode *x = list->header;
    int i, level;
    for (i = list->level; i >= 1; i--) {
        while (x->forward[i]->key < key)
            x = x->forward[i];
        update[i] = x;
    }
    x = x->forward[1];

    if (key == x->key) {
        x->value = value;
        return 0;
    } else {
        level = rand_level();
        if (level > list->level) {
            for (i = list->level + 1; i <= level; i++) {
                update[i] = list->header;
            }
            list->level = level;
        }

        x = (snode *) malloc(sizeof(snode));
        x->key = key;
        x->value = value;
        x->forward = (snode **) malloc(sizeof(snode*) * (level + 1));
        for (i = 1; i <= level; i++) {
            x->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = x;
        }
    }
    return 0;
}

#ifdef MCS
    int skiplist_insert(int key, int value, mcs_lock_t * the_lock) {
#else
    int skiplist_insert(int key, int value) {
#endif
    snode *update[SKIPLIST_MAX_LEVEL + 1];

    LOCK(lock)

    snode *x = list->header;
    int i, level;
    for (i = list->level; i >= 1; i--) {
        while (x->forward[i]->key < key)
            x = x->forward[i];
        update[i] = x;
    }
    x = x->forward[1];

    if (key == x->key) {
        x->value = value;
        UNLOCK(lock)
        return 0;
    } else {
        level = rand_level();
        if (level > list->level) {
            for (i = list->level + 1; i <= level; i++) {
                update[i] = list->header;
            }
            list->level = level;
        }

        x = (snode *) malloc(sizeof(snode));
        x->key = key;
        x->value = value;
        x->forward = (snode **) malloc(sizeof(snode*) * (level + 1));
        for (i = 1; i <= level; i++) {
            x->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = x;
        }
    }
    UNLOCK(lock)
    return 0;
}

#ifdef MCS
    snode *skiplist_search(int key, mcs_lock_t * the_lock){
#else
    snode *skiplist_search(int key) {
#endif
    LOCK(lock)
    snode *x = list->header;
    int i;
    for (i = list->level; i >= 1; i--) {
        while (x->forward[i]->key < key)
            x = x->forward[i];
    }
    if (x->forward[1]->key == key) {
        UNLOCK(lock)
        return x->forward[1];
    } else {
        UNLOCK(lock)
        return NULL;
    }
    UNLOCK(lock)
    return NULL;
}

static void skiplist_node_free(snode *x) {
    if (x) {
        free(x->forward);
        free(x);
    }
}

#ifdef MCS
int skiplist_delete(int key, mcs_lock_t * the_lock) {
#else
int skiplist_delete(int key) {
#endif 
    int i;
    snode *update[SKIPLIST_MAX_LEVEL + 1];

    LOCK(lock)

    snode *x = list->header;
    for (i = list->level; i >= 1; i--) {
        while (x->forward[i]->key < key)
            x = x->forward[i];
        update[i] = x;
    }

    x = x->forward[1];
    if (x->key == key) {
        for (i = 1; i <= list->level; i++) {
            if (update[i]->forward[i] != x)
                break;
            update[i]->forward[i] = x->forward[i];
        }
        skiplist_node_free(x);

        while (list->level > 1 && list->header->forward[list->level]
                == list->header)
            list->level--;
            UNLOCK(lock)
        return 0;
    }
    
    UNLOCK(lock)
    return 1;
}

void skiplist_free(skiplist *list)
{
    snode *current_node = list->header->forward[1];
    while(current_node != list->header) {
        snode *next_node = current_node->forward[1];
        free(current_node->forward);
        free(current_node);
        current_node = next_node;
    }
    free(current_node->forward);
    free(current_node);
    free(list);
}

static void skiplist_dump(skiplist *list) {
    snode *x = list->header;
    while (x && x->forward[1] != list->header) {
        printf("%d[%d]->", x->forward[1]->key, x->forward[1]->value);
        x = x->forward[1];
    }
    printf("NIL\n");
}

int floor_log_2(unsigned int n) {
    int pos = 0;
    if (n >= 1<<16) { n >>= 16; pos += 16; }
    if (n >= 1<< 8) { n >>=  8; pos +=  8; }
    if (n >= 1<< 4) { n >>=  4; pos +=  4; }
    if (n >= 1<< 2) { n >>=  2; pos +=  2; }
    if (n >= 1<< 1) {           pos +=  1; }
    return ((n == 0) ? (-1) : pos);
}

void pin_to_core(int core_id){
    int num_cpu = numa_num_configured_cpus();
    struct bitmask * cpumask = numa_bitmask_alloc(num_cpu);
    numa_bitmask_setbit(cpumask, core_id);
    numa_sched_setaffinity(0, cpumask);
}

void *worker(void *th_data)
{
    unsigned long op;
    int i, j;
    int key;
    data *my_data = (data *)th_data;
    int seed = rand();
    int ret;

    unsigned long local_range = range;

    #ifdef FFWD
        GET_CONTEXT()
    #else
        pin_to_core(my_data->id);
    #endif

    #ifdef MCS
        mcs_lock_t *the_lock;
        the_lock = malloc (sizeof(mcs_lock_t));
        atomic_barrier();
    #endif

    while (stop == 0) {
        seed = (seed + MULTIPLIER) % 1000;
        op = seed;
        seed = (seed + MULTIPLIER) % local_range;
        key = seed;
        if (op < UPDATE_RATIO) {
            if (op < UPDATE_RATIO / 2) {
                #ifdef FFWD
                    FFWD_EXEC(0, skiplist_insert, ret, 2, key, my_data->id)
                #elif MCS
                    skiplist_insert(key, my_data->id, the_lock);
                #else
                    skiplist_insert(key, my_data->id);
                #endif
                my_data->nr_ins++;
            } 
            else {
                #ifdef FFWD
                    FFWD_EXEC(0, skiplist_delete, ret, 1, key)
                #elif MCS 
                    skiplist_delete(key, the_lock);
                #else
                    skiplist_delete(key);
                #endif
                my_data->nr_del++;
            }
        }        
        else {
            #ifdef FFWD
                FFWD_EXEC(0, skiplist_search, ret, 1, key)
            #elif MCS 
                skiplist_search(key, the_lock);
            #else
                skiplist_search(key);
            #endif
            my_data->nr_find++;
        }

        for (j=0; j<25; j++)
            __asm__ __volatile__ ("pause");

    }

    return NULL;
}

int main(int argc, char ** argv) {

    int c;
    int duration = 5000;
    int num_of_threads;
    while((c = getopt(argc, argv, "t:d:i:r:")) != -1){
        switch (c)
        {
            case 't':
            {
                num_of_threads = atoi(optarg);
                break;
            }
            case 'd':
            {
                duration = atoi(optarg);
                break;
            }
            case 'i' :
                list_size = atoi(optarg);
                break;
            case 'r' :
                range = atoi(optarg);
                break;
        }
    }

    if (list_size == 1)
        levelmax = 1;
    else
        levelmax = floor_log_2((unsigned int) range);

    list = (skiplist *)numa_alloc_onnode(sizeof(skiplist), 0);
    skiplist_init();

    #ifdef FFWD
        ffwd_init();
        launch_servers(1);
    #else
        INIT_LOCK(lock)
    #endif

    int i;
    for (i = 0; i < range; i+=(range/list_size)) {
        skiplist_insert_init(i, i);
    }

    unsigned long nr_read, nr_write, nr_txn;
    struct timeval start, end;
    struct timespec timeout;
    timeout.tv_sec = duration / 1000;
    timeout.tv_nsec = (duration % 1000) * 1000000;

    data** th_data = (data **)malloc(num_of_threads * sizeof(data *));
    for (i=0; i<num_of_threads; i++){
        th_data[i] = (data *)malloc(128);
    }

    pthread_t *th = malloc(sizeof(pthread_t) * num_of_threads);

    for (i=0; i<num_of_threads; i++){
        th_data[i]->id = i;
        th_data[i]->nr_ins = 0;
        th_data[i]->nr_del = 0;
        th_data[i]->nr_find = 0;
        #ifdef FFWD
            ffwd_thread_create(&th[i], 0, worker, (void *)(th_data[i]));
        #else
            pthread_create(&th[i], 0, worker, (void *)(th_data[i]));
        #endif

    }

    gettimeofday(&start, NULL);
    nanosleep(&timeout, NULL);
    stop = 1;

    gettimeofday(&end, NULL);


    for (i=0; i<num_of_threads; i++){
        pthread_join(th[i], 0);
    }


    duration = (end.tv_sec * 1000 + end.tv_usec / 1000) -
               (start.tv_sec * 1000 + start.tv_usec / 1000);
    nr_read = 0;
    nr_write = 0;
    for (i = 0;  i < num_of_threads; i++) {
        nr_read += (th_data[i]->nr_find);
        nr_write += (th_data[i]->nr_ins + th_data[i]->nr_del);
    }

    printf("Mops: %.3f\n", ((nr_read + nr_write)/duration)/1000.0);

    #ifdef FFWD
        ffwd_shutdown();
    #endif
    for (i=0; i<num_of_threads; i++){
        free(th_data[i]);
    }

    return 0;
}
