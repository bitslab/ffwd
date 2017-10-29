/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Thread management for memcached.
 */
#include "memcached.h"
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <liblock.h>
#include "liblock-config.h"
//;
#include <stdint.h>

#define ITEMS_PER_ALLOC 64

/* An item in the connection queue. */
typedef struct conn_queue_item CQ_ITEM;
struct conn_queue_item {
    int               sfd;
    enum conn_states  init_state;
    int               event_flags;
    int               read_buffer_size;
    enum network_transport     transport;
    CQ_ITEM          *next;
};

/* A connection queue. */
typedef struct conn_queue CQ;
struct conn_queue {
    CQ_ITEM *head;
    CQ_ITEM *tail;
    pthread_mutex_t lock;
    pthread_cond_t  cond;
};

/* Lock for cache operations (item_*, assoc_*) */
liblock_lock_t cache_lock;

/* Connection lock around accepting new connections */
pthread_mutex_t conn_lock = PTHREAD_MUTEX_INITIALIZER;

/* Lock for global stats */
static pthread_mutex_t stats_lock;

/* Free list of CQ_ITEM structs */
static CQ_ITEM *cqi_freelist;
static pthread_mutex_t cqi_freelist_lock;

static LIBEVENT_DISPATCHER_THREAD dispatcher_thread;

/*
 * Each libevent instance has a wakeup pipe, which other threads
 * can use to signal that they've put a new connection on its queue.
 */
static LIBEVENT_THREAD *threads;

/*
 * Number of worker threads that have finished setting themselves up.
 */
static int init_count = 0;
static pthread_mutex_t init_lock;
static pthread_cond_t init_cond;


static void thread_libevent_process(int fd, short which, void *arg);

/*
 * Initializes a connection queue.
 */
static void cq_init(CQ *cq) {
    pthread_mutex_init(&cq->lock, NULL);
    pthread_cond_init(&cq->cond, NULL);
    cq->head = NULL;
    cq->tail = NULL;
}

/*
 * Looks for an item on a connection queue, but doesn't block if there isn't
 * one.
 * Returns the item, or NULL if no item is available
 */
static CQ_ITEM *cq_pop(CQ *cq) {
    CQ_ITEM *item;

    pthread_mutex_lock(&cq->lock);
    item = cq->head;
    if (NULL != item) {
        cq->head = item->next;
        if (NULL == cq->head)
            cq->tail = NULL;
    }
    pthread_mutex_unlock(&cq->lock);

    return item;
}

/*
 * Adds an item to a connection queue.
 */
static void cq_push(CQ *cq, CQ_ITEM *item) {
    item->next = NULL;

    pthread_mutex_lock(&cq->lock);
    if (NULL == cq->tail)
        cq->head = item;
    else
        cq->tail->next = item;
    cq->tail = item;
    pthread_cond_signal(&cq->cond);
    pthread_mutex_unlock(&cq->lock);
}

/*
 * Returns a fresh connection queue item.
 */
static CQ_ITEM *cqi_new(void) {
    CQ_ITEM *item = NULL;
    pthread_mutex_lock(&cqi_freelist_lock);
    if (cqi_freelist) {
        item = cqi_freelist;
        cqi_freelist = item->next;
    }
    pthread_mutex_unlock(&cqi_freelist_lock);

    if (NULL == item) {
        int i;

        /* Allocate a bunch of items at once to reduce fragmentation */
        item = malloc(sizeof(CQ_ITEM) * ITEMS_PER_ALLOC);
        if (NULL == item)
            return NULL;

        /*
         * Link together all the new items except the first one
         * (which we'll return to the caller) for placement on
         * the freelist.
         */
        for (i = 2; i < ITEMS_PER_ALLOC; i++)
            item[i - 1].next = &item[i];

        pthread_mutex_lock(&cqi_freelist_lock);
        item[ITEMS_PER_ALLOC - 1].next = cqi_freelist;
        cqi_freelist = &item[1];
        pthread_mutex_unlock(&cqi_freelist_lock);
    }

    return item;
}


/*
 * Frees a connection queue item (adds it to the freelist.)
 */
static void cqi_free(CQ_ITEM *item) {
    pthread_mutex_lock(&cqi_freelist_lock);
    item->next = cqi_freelist;
    cqi_freelist = item;
    pthread_mutex_unlock(&cqi_freelist_lock);
}


/*
 * Creates a worker thread.
 */
static void create_worker(void *(*func)(void *), void *arg) {
    pthread_t       thread;
    pthread_attr_t  attr;
    int             ret;

    pthread_attr_init(&attr);

    if ((ret = liblock_thread_create(&thread, &attr, func, arg)) != 0) {
        fprintf(stderr, "Can't create thread: %s\n",
                strerror(ret));
        exit(1);
    }
}

/*
 * Sets whether or not we accept new connections.
 */
void accept_new_conns(const bool do_accept) {
    pthread_mutex_lock(&conn_lock);
    do_accept_new_conns(do_accept);
    pthread_mutex_unlock(&conn_lock);
}
/****************************** LIBEVENT THREADS *****************************/

/*
 * Set up a thread's information.
 */
static void setup_thread(LIBEVENT_THREAD *me) {
    me->base = event_init();
    if (! me->base) {
        fprintf(stderr, "Can't allocate event base\n");
        exit(1);
    }

    /* Listen for notifications from other threads */
    event_set(&me->notify_event, me->notify_receive_fd,
              EV_READ | EV_PERSIST, thread_libevent_process, me);
    event_base_set(me->base, &me->notify_event);

    if (event_add(&me->notify_event, 0) == -1) {
        fprintf(stderr, "Can't monitor libevent notify pipe\n");
        exit(1);
    }

    me->new_conn_queue = malloc(sizeof(struct conn_queue));
    if (me->new_conn_queue == NULL) {
        perror("Failed to allocate memory for connection queue");
        exit(EXIT_FAILURE);
    }
    cq_init(me->new_conn_queue);

    if (pthread_mutex_init(&me->stats.mutex, NULL) != 0) {
        perror("Failed to initialize mutex");
        exit(EXIT_FAILURE);
    }

    me->suffix_cache = cache_create("suffix", SUFFIX_SIZE, sizeof(char*),
                                    NULL, NULL);
    if (me->suffix_cache == NULL) {
        fprintf(stderr, "Failed to create suffix cache\n");
        exit(EXIT_FAILURE);
    }
}


/*
 * Worker thread: main event loop
 */
static void *worker_libevent(void *arg) {
    LIBEVENT_THREAD *me = arg;

    /* Any per-thread setup can happen here; thread_init() will block until
     * all threads have finished initializing.
     */

    pthread_mutex_lock(&init_lock);
    init_count++;
    pthread_cond_signal(&init_cond);
    pthread_mutex_unlock(&init_lock);

    event_base_loop(me->base, 0);
    return NULL;
}


/*
 * Processes an incoming "handle a new connection" item. This is called when
 * input arrives on the libevent wakeup pipe.
 */
static void thread_libevent_process(int fd, short which, void *arg) {
    LIBEVENT_THREAD *me = arg;
    CQ_ITEM *item;
    char buf[1];

    if (read(fd, buf, 1) != 1)
        if (settings.verbose > 0)
            fprintf(stderr, "Can't read from libevent pipe\n");

    item = cq_pop(me->new_conn_queue);

    if (NULL != item) {
        conn *c = conn_new(item->sfd, item->init_state, item->event_flags,
                           item->read_buffer_size, item->transport, me->base);
        if (c == NULL) {
            if (IS_UDP(item->transport)) {
                fprintf(stderr, "Can't listen for events on UDP socket\n");
                exit(1);
            } else {
                if (settings.verbose > 0) {
                    fprintf(stderr, "Can't listen for events on fd %d\n",
                        item->sfd);
                }
                close(item->sfd);
            }
        } else {
            c->thread = me;
        }
        cqi_free(item);
    }
}

/* Which thread we assigned a connection to most recently. */
static int last_thread = -1;

/*
 * Dispatches a new connection to another thread. This is only ever called
 * from the main thread, either during initialization (for UDP) or because
 * of an incoming connection.
 */
void dispatch_conn_new(int sfd, enum conn_states init_state, int event_flags,
                       int read_buffer_size, enum network_transport transport) {
    CQ_ITEM *item = cqi_new();
    int tid = (last_thread + 1) % settings.num_threads;

    LIBEVENT_THREAD *thread = threads + tid;

    last_thread = tid;

    item->sfd = sfd;
    item->init_state = init_state;
    item->event_flags = event_flags;
    item->read_buffer_size = read_buffer_size;
    item->transport = transport;

    cq_push(thread->new_conn_queue, item);

    MEMCACHED_CONN_DISPATCH(sfd, thread->thread_id);
    if (write(thread->notify_send_fd, "", 1) != 1) {
        perror("Writing to thread notify pipe");
    }
}

/*
 * Returns true if this is the thread that listens for new TCP connections.
 */
int is_listen_thread() {
    return pthread_self() == dispatcher_thread.thread_id;
}

union instance26 {struct input24{rel_time_t exptime;char *key;int nbytes;int flags;size_t nkey;} input24;};
void * function27(void *ctx25);
void *function27(void *ctx25) {
    {
        struct input24 *incontext22=&(((union instance26 *)ctx25)->input24);
        item *it;
        rel_time_t exptime=incontext22->exptime;
        char *key=incontext22->key;
        int nbytes=incontext22->nbytes;
        int flags=incontext22->flags;
        size_t nkey=incontext22->nkey;
        {
            it = do_item_alloc(key, nkey, flags, exptime, nbytes);
        }
        return (void *)(uintptr_t)it;
    }
}

/********************************* ITEM ACCESS *******************************/

/*
 * Allocates a new item.
 */
item *item_alloc(char *key, size_t nkey, int flags, rel_time_t exptime, int nbytes) {
    item *it;
    { union instance26 instance26 = {
        {
            exptime,
            key,
            nbytes,
            flags,
            nkey,
        },
    };
    
    it =(item *)(uintptr_t)(liblock_execute_operation(&cache_lock, (void *)(uintptr_t)(&instance26), &function27));
    }
    return it;
}

union instance33 {struct input31{const size_t nkey;const char *key;} input31;};
void * function34(void *ctx32);
void *function34(void *ctx32) {
    {
        struct input31 *incontext29=&(((union instance33 *)ctx32)->input31);
        item *it;
        const size_t nkey=incontext29->nkey;
        const char *key=incontext29->key;
        {
            it = do_item_get(key, nkey);
        }
        return (void *)(uintptr_t)it;
    }
}

/*
 * Returns an item if it hasn't been marked as expired,
 * lazy-expiring as needed.
 */
item *item_get(const char *key, const size_t nkey) {
    item *it;
    { union instance33 instance33 = {
        {
            nkey,
            key,
        },
    };
    
    it =(item *)(uintptr_t)(liblock_execute_operation(&cache_lock, (void *)(uintptr_t)(&instance33), &function34));
    }
    return it;
}

void * function41(void *ctx39);
void *function41(void *ctx39) {
    {
        int ret;
        item *it=(item *)(uintptr_t)ctx39;
        {
            ret = do_item_link(it);
        }
        return (void *)(uintptr_t)ret;
    }
}

/*
 * Links an item into the LRU and hashtable.
 */
int item_link(item *it) {
    int ret;

    {
    
    ret =(int)(uintptr_t)(liblock_execute_operation(&cache_lock, (void *)(uintptr_t)(it), &function41));
    }
    return ret;
}

void * function48(void *ctx46);
void *function48(void *ctx46) {
    {
        item *it=(item *)(uintptr_t)ctx46;
        {
            do_item_remove(it);
        }
        return NULL;
    }
}

/*
 * Decrements the reference count on an item and adds it to the freelist if
 * needed.
 */
void item_remove(item *it) {
    {
    
    liblock_execute_operation(&cache_lock, (void *)(uintptr_t)(it), &function48); }
}

/*
 * Replaces one item with another in the hashtable.
 * Unprotected by a mutex lock since the core server does not require
 * it to be thread-safe.
 */
int item_replace(item *old_it, item *new_it) {
    return do_item_replace(old_it, new_it);
}

void * function55(void *ctx53);
void *function55(void *ctx53) {
    {
        item *it=(item *)(uintptr_t)ctx53;
        {
            do_item_unlink(it);
        }
        return NULL;
    }
}

/*
 * Unlinks an item from the LRU and hashtable.
 */
void item_unlink(item *it) {
    {
    
    liblock_execute_operation(&cache_lock, (void *)(uintptr_t)(it), &function55); }
}

void * function62(void *ctx60);
void *function62(void *ctx60) {
    {
        item *it=(item *)(uintptr_t)ctx60;
        {
            do_item_update(it);
        }
        return NULL;
    }
}

/*
 * Moves an item to the back of the LRU queue.
 */
void item_update(item *it) {
    {
    
    liblock_execute_operation(&cache_lock, (void *)(uintptr_t)(it), &function62); }
}

union instance68 {struct input66{const int64_t delta;const size_t nkey;const char *key;uint64_t *cas;conn *c;char *buf;int incr;} input66;};
void * function69(void *ctx67);
void *function69(void *ctx67) {
    {
        struct input66 *incontext64=&(((union instance68 *)ctx67)->input66);
        enum delta_result_type ret;
        const int64_t delta=incontext64->delta;
        const size_t nkey=incontext64->nkey;
        const char *key=incontext64->key;
        uint64_t *cas=incontext64->cas;
        conn *c=incontext64->c;
        char *buf=incontext64->buf;
        int incr=incontext64->incr;
        {
            ret = do_add_delta(c, key, nkey, incr, delta, buf, cas);
        }
        return (void *)(uintptr_t)ret;
    }
}

/*
 * Does arithmetic on a numeric item value.
 */
enum delta_result_type add_delta(conn *c, const char *key,
                                 const size_t nkey, int incr,
                                 const int64_t delta, char *buf,
                                 uint64_t *cas) {
    enum delta_result_type ret;

    { union instance68 instance68 = {
        {
            delta,
            nkey,
            key,
            cas,
            c,
            buf,
            incr,
        },
    };
    
    ret =(enum delta_result_type)(uintptr_t)(liblock_execute_operation(&cache_lock, (void *)(uintptr_t)(&instance68), &function69));
    }
    return ret;
}

union instance75 {struct input73{item *it;conn *c;int comm;} input73;};
void * function76(void *ctx74);
void *function76(void *ctx74) {
    {
        struct input73 *incontext71=&(((union instance75 *)ctx74)->input73);
        enum store_item_type ret;
        item *it=incontext71->it;
        conn *c=incontext71->c;
        int comm=incontext71->comm;
        {
            ret = do_store_item(it, comm, c);
        }
        return (void *)(uintptr_t)ret;
    }
}

/*
 * Stores an item in the cache (high level, obeys set/add/replace semantics)
 */
enum store_item_type store_item(item *it, int comm, conn* c) {
    enum store_item_type ret;

    { union instance75 instance75 = {
        {
            it,
            c,
            comm,
        },
    };
    
    ret =(enum store_item_type)(uintptr_t)(liblock_execute_operation(&cache_lock, (void *)(uintptr_t)(&instance75), &function76));
    }
    return ret;
}

void * function83(void *ctx81);
void *function83(void *ctx81) {
    {
        {
            do_item_flush_expired();
        }
        return NULL;
    }
}

/*
 * Flushes expired items after a flush_all call
 */
void item_flush_expired() {
    {
    liblock_execute_operation(&cache_lock, (void *)(uintptr_t)(NULL), &function83); }
}

union instance89 {struct input87{unsigned int *bytes;unsigned int limit;unsigned int slabs_clsid;} input87;};
void * function90(void *ctx88);
void *function90(void *ctx88) {
    {
        struct input87 *incontext85=&(((union instance89 *)ctx88)->input87);
        char *ret;
        unsigned int *bytes=incontext85->bytes;
        unsigned int limit=incontext85->limit;
        unsigned int slabs_clsid=incontext85->slabs_clsid;
        {
            ret = do_item_cachedump(slabs_clsid, limit, bytes);
        }
        return (void *)(uintptr_t)ret;
    }
}

/*
 * Dumps part of the cache
 */
char *item_cachedump(unsigned int slabs_clsid, unsigned int limit, unsigned int *bytes) {
    char *ret;

    { union instance89 instance89 = {
        {
            bytes,
            limit,
            slabs_clsid,
        },
    };
    
    ret =(char *)(uintptr_t)(liblock_execute_operation(&cache_lock, (void *)(uintptr_t)(&instance89), &function90));
    }
    return ret;
}

union instance96 {struct input94{ADD_STAT add_stats;void *c;} input94;};
void * function97(void *ctx95);
void *function97(void *ctx95) {
    {
        struct input94 *incontext92=&(((union instance96 *)ctx95)->input94);
        ADD_STAT add_stats=incontext92->add_stats;
        void *c=incontext92->c;
        {
            do_item_stats(add_stats, c);
        }
        return NULL;
    }
}

/*
 * Dumps statistics about slab classes
 */
void  item_stats(ADD_STAT add_stats, void *c) {
    { union instance96 instance96 = {
        {
            add_stats,
            c,
        },
    };
    
    liblock_execute_operation(&cache_lock, (void *)(uintptr_t)(&instance96), &function97); }
}

union instance103 {struct input101{ADD_STAT add_stats;void *c;} input101;};
void * function104(void *ctx102);
void *function104(void *ctx102) {
    {
        struct input101 *incontext99=&(((union instance103 *)ctx102)->input101);
        ADD_STAT add_stats=incontext99->add_stats;
        void *c=incontext99->c;
        {
            do_item_stats_sizes(add_stats, c);
        }
        return NULL;
    }
}

/*
 * Dumps a list of objects of each size in 32-byte increments
 */
void  item_stats_sizes(ADD_STAT add_stats, void *c) {
    { union instance103 instance103 = {
        {
            add_stats,
            c,
        },
    };
    
    liblock_execute_operation(&cache_lock, (void *)(uintptr_t)(&instance103), &function104); }
}

/******************************* GLOBAL STATS ******************************/

void STATS_LOCK() {
    pthread_mutex_lock(&stats_lock);
}

void STATS_UNLOCK() {
    pthread_mutex_unlock(&stats_lock);
}

void threadlocal_stats_reset(void) {
    int ii, sid;
    for (ii = 0; ii < settings.num_threads; ++ii) {
        pthread_mutex_lock(&threads[ii].stats.mutex);

        threads[ii].stats.get_cmds = 0;
        threads[ii].stats.get_misses = 0;
        threads[ii].stats.delete_misses = 0;
        threads[ii].stats.incr_misses = 0;
        threads[ii].stats.decr_misses = 0;
        threads[ii].stats.cas_misses = 0;
        threads[ii].stats.bytes_read = 0;
        threads[ii].stats.bytes_written = 0;
        threads[ii].stats.flush_cmds = 0;
        threads[ii].stats.conn_yields = 0;
        threads[ii].stats.auth_cmds = 0;
        threads[ii].stats.auth_errors = 0;

        for(sid = 0; sid < MAX_NUMBER_OF_SLAB_CLASSES; sid++) {
            threads[ii].stats.slab_stats[sid].set_cmds = 0;
            threads[ii].stats.slab_stats[sid].get_hits = 0;
            threads[ii].stats.slab_stats[sid].delete_hits = 0;
            threads[ii].stats.slab_stats[sid].incr_hits = 0;
            threads[ii].stats.slab_stats[sid].decr_hits = 0;
            threads[ii].stats.slab_stats[sid].cas_hits = 0;
            threads[ii].stats.slab_stats[sid].cas_badval = 0;
        }

        pthread_mutex_unlock(&threads[ii].stats.mutex);
    }
}

void threadlocal_stats_aggregate(struct thread_stats *stats) {
    int ii, sid;

    /* The struct has a mutex, but we can safely set the whole thing
     * to zero since it is unused when aggregating. */
    memset(stats, 0, sizeof(*stats));

    for (ii = 0; ii < settings.num_threads; ++ii) {
        pthread_mutex_lock(&threads[ii].stats.mutex);

        stats->get_cmds += threads[ii].stats.get_cmds;
        stats->get_misses += threads[ii].stats.get_misses;
        stats->delete_misses += threads[ii].stats.delete_misses;
        stats->decr_misses += threads[ii].stats.decr_misses;
        stats->incr_misses += threads[ii].stats.incr_misses;
        stats->cas_misses += threads[ii].stats.cas_misses;
        stats->bytes_read += threads[ii].stats.bytes_read;
        stats->bytes_written += threads[ii].stats.bytes_written;
        stats->flush_cmds += threads[ii].stats.flush_cmds;
        stats->conn_yields += threads[ii].stats.conn_yields;
        stats->auth_cmds += threads[ii].stats.auth_cmds;
        stats->auth_errors += threads[ii].stats.auth_errors;

        for (sid = 0; sid < MAX_NUMBER_OF_SLAB_CLASSES; sid++) {
            stats->slab_stats[sid].set_cmds +=
                threads[ii].stats.slab_stats[sid].set_cmds;
            stats->slab_stats[sid].get_hits +=
                threads[ii].stats.slab_stats[sid].get_hits;
            stats->slab_stats[sid].delete_hits +=
                threads[ii].stats.slab_stats[sid].delete_hits;
            stats->slab_stats[sid].decr_hits +=
                threads[ii].stats.slab_stats[sid].decr_hits;
            stats->slab_stats[sid].incr_hits +=
                threads[ii].stats.slab_stats[sid].incr_hits;
            stats->slab_stats[sid].cas_hits +=
                threads[ii].stats.slab_stats[sid].cas_hits;
            stats->slab_stats[sid].cas_badval +=
                threads[ii].stats.slab_stats[sid].cas_badval;
        }

        pthread_mutex_unlock(&threads[ii].stats.mutex);
    }
}

void slab_stats_aggregate(struct thread_stats *stats, struct slab_stats *out) {
    int sid;

    out->set_cmds = 0;
    out->get_hits = 0;
    out->delete_hits = 0;
    out->incr_hits = 0;
    out->decr_hits = 0;
    out->cas_hits = 0;
    out->cas_badval = 0;

    for (sid = 0; sid < MAX_NUMBER_OF_SLAB_CLASSES; sid++) {
        out->set_cmds += stats->slab_stats[sid].set_cmds;
        out->get_hits += stats->slab_stats[sid].get_hits;
        out->delete_hits += stats->slab_stats[sid].delete_hits;
        out->decr_hits += stats->slab_stats[sid].decr_hits;
        out->incr_hits += stats->slab_stats[sid].incr_hits;
        out->cas_hits += stats->slab_stats[sid].cas_hits;
        out->cas_badval += stats->slab_stats[sid].cas_badval;
    }
}

/*
 * Initializes the thread subsystem, creating various worker threads.
 *
 * nthreads  Number of worker event handler threads to spawn
 * main_base Event base for main thread
 */
void thread_init(int nthreads, struct event_base *main_base) {
    int         i;

    liblock_lock_init(TYPE_EXPERIENCE, DEFAULT_ARG, &cache_lock, NULL);
    pthread_mutex_init(&stats_lock, NULL);

    pthread_mutex_init(&init_lock, NULL);
    pthread_cond_init(&init_cond, NULL);

    pthread_mutex_init(&cqi_freelist_lock, NULL);
    cqi_freelist = NULL;

    threads = calloc(nthreads, sizeof(LIBEVENT_THREAD));
    if (! threads) {
        perror("Can't allocate thread descriptors");
        exit(1);
    }

    dispatcher_thread.base = main_base;
    dispatcher_thread.thread_id = pthread_self();

    for (i = 0; i < nthreads; i++) {
        int fds[2];
        if (pipe(fds)) {
            perror("Can't create notify pipe");
            exit(1);
        }

        threads[i].notify_receive_fd = fds[0];
        threads[i].notify_send_fd = fds[1];

        setup_thread(&threads[i]);
    }

    /* Create threads after we've done all the libevent setup. */
    for (i = 0; i < nthreads; i++) {
        create_worker(worker_libevent, &threads[i]);
    }

    /* Wait for all the threads to set themselves up before returning. */
    pthread_mutex_lock(&init_lock);
    while (init_count < nthreads) {
        pthread_cond_wait(&init_cond, &init_lock);
    }
    pthread_mutex_unlock(&init_lock);
}

