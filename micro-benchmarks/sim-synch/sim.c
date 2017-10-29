#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#include "config.h"
#include "primitives.h"
#include "tvec.h"
#include "rand.h"
#include "thread.h"

#define LOCAL_POOL_SIZE            16


typedef union CRStruct {
    volatile Object obj;
    char pad[CACHE_LINE_SIZE];
} CRStruct;

typedef struct STATE{
    CRStruct Critical[OBJECT_SIZE] CACHE_ALIGN;
} STATE;

typedef struct HalfObjectState {
    ToggleVector applied;
    STATE state;
    RetVal ret[N_THREADS];
#ifdef DEBUG
    int counter;
    int rounds CACHE_ALIGN;
#endif
} HalfObjectState;


typedef struct ObjectState {
    ToggleVector applied;
    STATE state;
    RetVal ret[N_THREADS];
#ifdef DEBUG
    int counter;
    int rounds CACHE_ALIGN;
#endif
    int32_t pad[PAD_CACHE(sizeof(HalfObjectState))];
} ObjectState;

typedef union pointer_t {
    struct StructData{
        int64_t seq : 48;
        int32_t index : 16;
    } struct_data;
    int64_t raw_data;
} pointer_t;


typedef struct SimThreadState {
    ToggleVector mask;
    ToggleVector toggle;
    ToggleVector my_bit;
    int local_index;
    int backoff;
} SimThreadState;


static int MAX_BACK;

// Shared variables
volatile pointer_t sp CACHE_ALIGN;

// Try to place a_toggles and anounce to 
// the same cache line
volatile ToggleVector a_toggles CACHE_ALIGN;
// _TVEC_BIWORD_SIZE_ is a performance workaround for 
// array announce. Size N_THREADS is algorithmically enough.
volatile ArgVal announce[N_THREADS + _TVEC_BIWORD_SIZE_] CACHE_ALIGN;
volatile ObjectState pool[LOCAL_POOL_SIZE * N_THREADS + 1] CACHE_ALIGN;


void SHARED_OBJECT_INIT(SimThreadState *th_state, int pid) {
    if (pid == 0) {
        sp.struct_data.index = LOCAL_POOL_SIZE * N_THREADS;
        sp.struct_data.seq = 0;
        TVEC_SET_ZERO((ToggleVector *)&a_toggles);

        // OBJECT'S INITIAL VALUE
        // ----------------------

        TVEC_SET_ZERO((ToggleVector *)&pool[LOCAL_POOL_SIZE * N_THREADS].applied);
        MAX_BACK *= 100;
#ifdef DEBUG
        pool[LOCAL_POOL_SIZE * N_THREADS].counter = 0;
        pool[LOCAL_POOL_SIZE * N_THREADS].rounds = 0;
#endif
        FullFence();
    }

    TVEC_SET_ZERO(&th_state->mask);
    TVEC_SET_ZERO(&th_state->my_bit);
    TVEC_SET_ZERO(&th_state->toggle);
    TVEC_REVERSE_BIT(&th_state->my_bit, pid);
    TVEC_SET_BIT(&th_state->mask, pid);
    th_state->toggle = TVEC_NEGATIVE(th_state->mask);
    th_state->local_index = 0;
    th_state->backoff = 1;
}


static inline void fetchAndMultiply(HalfObjectState *lsp_data, volatile Object arg, int pid) {
    int i;

#ifdef DEBUG
    lsp_data->counter++;
#endif
    for (i = 0; i < OBJECT_SIZE; i++)
        lsp_data->state.Critical[i].obj += 1;
    lsp_data->ret[pid] = lsp_data->state.Critical[0].obj;
}


static inline Object apply_op(void (*sfunc)(HalfObjectState *, volatile Object, int),  SimThreadState *th_state, Object arg, int pid) {
    ToggleVector diffs, l_toggles;
    pointer_t old_sp, new_sp;
    HalfObjectState *sp_data, *lsp_data;
    int i, j, prefix, mybank;

    announce[pid] = arg;                                                     // announce the operation
    mybank = TVEC_GET_BANK_OF_BIT(pid);
    TVEC_REVERSE_BIT(&th_state->my_bit, pid);
    TVEC_NEGATIVE_BANK(&th_state->toggle, &th_state->toggle, mybank);
    lsp_data = (HalfObjectState *)&pool[pid * LOCAL_POOL_SIZE + th_state->local_index];
    TVEC_ATOMIC_ADD_BANK(&a_toggles, &th_state->toggle, mybank);             // toggle pid's bit in a_toggles, Fetch&Add acts as a full write-barrier
#if N_THREADS > USE_CPUS
    if (simRandomRange(1, N_THREADS) > 4)
        sched_yield();
#else
    volatile int k;
    int backoff_limit;

    if (simRandomRange(1, N_THREADS) > 1) { 
        backoff_limit = th_state->backoff;
        for (k = 0; k < backoff_limit; k++)
            ;
    }
#endif

    for (j = 0; j < 2; j++) {
        old_sp = sp;                                                         // read reference to struct ObjectState
        sp_data = (HalfObjectState *)&pool[old_sp.struct_data.index];        // read reference of struct ObjectState in a local variable lsp
        TVEC_ATOMIC_COPY_BANKS(&diffs, &sp_data->applied, mybank);
        TVEC_XOR_BANKS(&diffs, &diffs, &th_state->my_bit, mybank);           // determine the set of active processes
        if (TVEC_IS_SET(diffs, pid))                                         // if the operation has already been applied return
            break;
        *lsp_data = *sp_data;
        l_toggles = a_toggles;                                               // This is an atomic read, since a_toogles is volatile
        if (old_sp.raw_data != sp.raw_data)
            continue;
        diffs = TVEC_XOR(lsp_data->applied, l_toggles);
#ifdef DEBUG
        lsp_data->rounds++;
#endif
        sfunc(lsp_data, arg, pid);
        TVEC_REVERSE_BIT(&diffs, pid);
        for (i = 0, prefix = 0; i < _TVEC_CELLS_; i++, prefix += _TVEC_BIWORD_SIZE_) {
            ReadPrefetch(&announce[prefix]);
            ReadPrefetch(&announce[prefix + 8]);
            ReadPrefetch(&announce[prefix + 16]);
            ReadPrefetch(&announce[prefix + 24]);
            while (diffs.cell[i] != 0L) {
                register int pos, proc_id;

                pos = bitSearchFirst(diffs.cell[i]);
                proc_id = prefix + pos;
                diffs.cell[i] ^= 1L << pos;
                sfunc(lsp_data, announce[proc_id], proc_id);
            }
        }
        lsp_data->applied = l_toggles;                                       // change applied to be equal to what was read in a_toggles
        new_sp.struct_data.seq = old_sp.struct_data.seq + 1;                 // increase timestamp
        new_sp.struct_data.index = LOCAL_POOL_SIZE * pid + th_state->local_index;    // store in mod_dw.index the index in pool where lsp will be stored
        if (old_sp.raw_data==sp.raw_data && 
            CAS64(&sp, old_sp.raw_data, new_sp.raw_data)) {                  // try to change sp to the value mod_dw
            th_state->local_index = (th_state->local_index + 1) % LOCAL_POOL_SIZE;   // if this happens successfully,use next item in pid's pool next time
            th_state->backoff = (th_state->backoff >> 1) | 1;
            return lsp_data->ret[pid];
        } else if (th_state->backoff < MAX_BACK) th_state->backoff <<= 1;
    }
    return pool[sp.struct_data.index].ret[pid];                              // return the value found in the record stored there
}

pthread_barrier_t barr;
int64_t d1 CACHE_ALIGN, d2;

static inline void Execute(void* Arg) {
    SimThreadState th_state;
    long i, rnum;
    long id = (long) Arg;
    volatile long j;

    SHARED_OBJECT_INIT(&th_state, id);
    setThreadId(id);
    _thread_pin(id);
    simSRandom((unsigned long)id + 1L);
    if (id == N_THREADS - 1)
        d1 = getTimeMillis();
    // Synchronization point
    int rc = pthread_barrier_wait(&barr);
    if (rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD) {
        printf("Could not wait on barrier\n");
        exit(EXIT_FAILURE);
    }
    start_cpu_counters(id);
    for (i = 0; i < RUNS; i++) {
        apply_op(fetchAndMultiply, &th_state, (Object) (id + 1L), id);
        rnum = simRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ;
    }
    stop_cpu_counters(id);
}

static inline void* EntryPoint(void* Arg) {
    Execute(Arg);
    return null;
}

static inline pthread_t StartThread(int arg) {
    long id = (long) arg;
    void *Arg = (void*) id;
    pthread_t thread_p;
    int thread_id;

    pthread_attr_t my_attr;
    pthread_attr_init(&my_attr);
    thread_id = pthread_create(&thread_p, &my_attr, EntryPoint, Arg);

    return thread_p;
}

int main(int argc, char *argv[]) {
    pthread_t threads[N_THREADS];
    int i;

    init_cpu_counters();
    if (argc != 2) {
        fprintf(stderr, "ERROR: Please set an upper bound for the backoff!\n");
        exit(EXIT_SUCCESS);
    } else {
        sscanf(argv[1], "%d", &MAX_BACK);
    }

    if (pthread_barrier_init(&barr, NULL, N_THREADS)) {
        printf("Could not create the barrier\n");
        return -1;
    }

    for (i = 0; i < N_THREADS; i++)
        threads[i] = StartThread(i);

    for (i = 0; i < N_THREADS; i++)
        pthread_join(threads[i], NULL);
    d2 = getTimeMillis();

    // printf("time: %d\t", (int) (d2 - d1));
    printf("%.3f\t", (double) (RUNS * 2 * N_THREADS) / (double) ((d2 - d1)*1000));
    
    printStats();
    
#ifdef DEBUG
    ObjectState *l = (ObjectState *)&pool[((pointer_t*)&sp)->struct_data.index];
    fprintf(stderr, "Object state long value: %d\n", l->counter);
    fprintf(stderr, "object counter: %d\n", l->counter);
    fprintf(stderr, "rounds: %d\n", l->rounds);
    fprintf(stderr, "Average helping: %f\n", (float)l->counter/l->rounds);
    fprintf(stderr, "\n");
#endif

    if (pthread_barrier_destroy(&barr)) {
        printf("Could not destroy the barrier\n");
        return -1;
    }
    return 0;
}
