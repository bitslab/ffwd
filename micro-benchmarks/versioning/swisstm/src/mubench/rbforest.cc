/**
 * Benchmark that atomically executes different operations that involve
 * several red-black trees.
 *
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#include <cstdlib>
#include <iostream>
#include <pthread.h>

// rbtree
#include "tm_spec.h"
#include "rbtree.h"
#include "rbtree.c"

#include "../common/cache_aligned_alloc.h"
#include "../common/constants.h"
#include "barrier.h"
#include "time.h"

enum optype_t {
    OP_TREE_LOOKUP = 0,
    OP_TREE_INSERT,
    OP_TREE_DELETE,
    OP_FOREST_LOOKUP,
    OP_FOREST_INSERT,
    OP_FOREST_DELETE,
    OP_TREE_MOVE,
    OP_TOTAL
};

struct thread_data_t;

typedef unsigned (*operation_fun_t)(TM_ARGDECL thread_data_t *thread_data);

struct operation_t {
    optype_t type;
    operation_fun_t fun;
};

// other possible operations:
//    insert if not in the tree, remove otherwise
//    do the same but for the whole forest

// default parameters
#define DEFAULT_OPERATION_COUNT    10000
#define DEFAULT_THREAD_COUNT           1
#define DEFAULT_FOREST_SIZE            2
#define DEFAULT_MAX_KEY_VALUE       1024
#define DEFAULT_TREE_LOOKUP_RATIO     60
#define DEFAULT_TREE_INSERT_RATIO      5
#define DEFAULT_TREE_DELETE_RATIO      5
#define DEFAULT_FOREST_LOOKUP_RATIO   15
#define DEFAULT_FOREST_INSERT_RATIO    5
#define DEFAULT_FOREST_DELETE_RATIO    5
#define DEFAULT_MOVE_RATIO             5

struct op_ratios_t {
    unsigned tree_lookup_ratio;
    unsigned tree_insert_ratio;
    unsigned tree_delete_ratio;
    unsigned forest_lookup_ratio;
    unsigned forest_insert_ratio;
    unsigned forest_delete_ratio;
    unsigned tree_move_ratio;
};

struct cmd_line_args_t {
    unsigned operation_count;
    unsigned thread_count;
    unsigned forest_size;
    unsigned max_key_value;
    op_ratios_t op_ratios;
};

struct thread_data_t {
    unsigned operation_count;
    unsigned forest_size;
    unsigned max_key_value;

    rbtree_t **forest;
    operation_t *operations;
    unsigned total_operations;
    unsigned rand_seed;

    // return
    unsigned op_exec[OP_TOTAL];

    // barriers
    barrier_t *start_barrier;
    barrier_t *end_barrier;
};

// command line args
static void print_usage();
static void parse_cmd_line_args(cmd_line_args_t &args, int argc, char **argv);

// operations
static unsigned op_tree_lookup(TM_ARGDECL thread_data_t *thread_data);
static unsigned op_tree_insert(TM_ARGDECL thread_data_t *thread_data);
static unsigned op_tree_delete(TM_ARGDECL thread_data_t *thread_data);
static unsigned op_forest_lookup(TM_ARGDECL thread_data_t *thread_data);
static unsigned op_forest_insert(TM_ARGDECL thread_data_t *thread_data);
static unsigned op_forest_delete(TM_ARGDECL thread_data_t *thread_data);
static unsigned op_tree_move(TM_ARGDECL thread_data_t *thread_data);

static rbtree_t *init_rbtree(unsigned max_val);
static operation_t *init_operations(cmd_line_args_t &args);
static unsigned get_total_op_ratio(op_ratios_t &op);

static void *thread_fun(void *thread_data);
static unsigned thread_main(thread_data_t *thread_data, bool measure_time = false);

int main(int argc, char **argv) {
    // initialize stm
    TM_STARTUP();

    // parse command line arguments
    cmd_line_args_t args;
    parse_cmd_line_args(args, argc, argv);

    // initialize rbforest
    rbtree_t **rbforest = (rbtree_t **)malloc(sizeof(rbtree_t *) * args.forest_size);

    for(unsigned i = 0;i < args.forest_size;i++) {
        rbforest[i] = init_rbtree(args.max_key_value);
    }

    // create barriers
    barrier_t *start_barrier = (barrier_t *)wlpdstm::malloc_aligned<CACHE_LINE_SIZE_BYTES,
            wlpdstm::Alloc>(sizeof(barrier_t));
    barrier_t *end_barrier = (barrier_t *)wlpdstm::malloc_aligned<CACHE_LINE_SIZE_BYTES,
            wlpdstm::Alloc>(sizeof(barrier_t));
    init_barrier(start_barrier, args.thread_count);
    init_barrier(end_barrier, args.thread_count);

    // initialize thread data
    thread_data_t **thread_data = (thread_data_t **)malloc(sizeof(thread_data_t *) * args.thread_count);
    float ops_per_thread = (float)args.operation_count / args.thread_count;
    unsigned prev_ops = 0;
    float ops = 0;

    for(unsigned i = 0;i < args.thread_count;i++) {
        thread_data_t *td = (thread_data_t *)wlpdstm::malloc_aligned<CACHE_LINE_SIZE_BYTES,
            wlpdstm::Alloc>(sizeof(thread_data_t));
        thread_data[i] = td;

        ops += ops_per_thread;
        td->operation_count = ops - prev_ops;
        prev_ops = ops;

        td->forest_size = args.forest_size;
        td->max_key_value = args.max_key_value;
        td->forest = rbforest;

        // initialize operations
        td->operations = init_operations(args);
        td->total_operations = get_total_op_ratio(args.op_ratios);

        td->rand_seed = i;

        for(unsigned i = 0;i < OP_TOTAL;i++) {
            td->op_exec[OP_TOTAL] = 0;
        }

        td->start_barrier = start_barrier;
        td->end_barrier = end_barrier;
    }

    // start threads
    pthread_t *threads = NULL;

    if(args.thread_count > 1) {
        threads = (pthread_t *)malloc(sizeof(pthread_t) * (args.thread_count - 1));

        for(unsigned i = 0;i < args.thread_count - 1;i++) {
            if(pthread_create(threads + i, NULL, thread_fun, (void *)thread_data[i + 1])) {
                std::cerr << "Cannot create thread " << i << ". Exiting." << std::endl;
                exit(1);
            }
        }
    }

    unsigned time = thread_main(thread_data[0], true);

    std::cout << "Total execution time: " << time << "ms" << std::endl;

    // free data
    // there is no function to remove all elements from a tree
    // TODO: maybe write one, but this is not a big deal as
    //       the benchmark is finishing anyway
    for(unsigned i = 0;i < args.forest_size;i++) {
        rbtree_free(rbforest[i]);
    }

    free(rbforest);

    // free thread data
    for(unsigned i = 0;i < args.thread_count;i++) {
        free(thread_data[i]->operations);
        wlpdstm::free_aligned<wlpdstm::Alloc>(thread_data[i]);
    }

    free(thread_data);

    wlpdstm::free_aligned<wlpdstm::Alloc>(start_barrier);
    wlpdstm::free_aligned<wlpdstm::Alloc>(end_barrier);

    // shutdown STM
    TM_SHUTDOWN();
}

static void print_usage() {
    std::cout << "rbforest -o <operation_count> -t <thread_count> "
        "-f <forest_size> -M <max_value> "
        "-l <tree_lookup_ratio> -i <tree_insert_ratio> -d <tree_delete_ratio> "
        "-L <forest_lookup_ratio> -I <forest_insert_ratio> -D <forest_delete_ratio> "
        "-m <move ratio> "<< std::endl;
    exit(0);
}

static void parse_cmd_line_args(cmd_line_args_t &args, int argc, char **argv) {
    // set default
    args.operation_count = DEFAULT_OPERATION_COUNT;
    args.thread_count = DEFAULT_THREAD_COUNT;
    args.forest_size = DEFAULT_FOREST_SIZE;
    args.max_key_value = DEFAULT_MAX_KEY_VALUE;
    args.op_ratios.tree_lookup_ratio = DEFAULT_TREE_LOOKUP_RATIO;
    args.op_ratios.tree_insert_ratio = DEFAULT_TREE_INSERT_RATIO;
    args.op_ratios.tree_delete_ratio = DEFAULT_TREE_DELETE_RATIO;
    args.op_ratios.forest_lookup_ratio = DEFAULT_FOREST_LOOKUP_RATIO;
    args.op_ratios.forest_insert_ratio = DEFAULT_FOREST_INSERT_RATIO;
    args.op_ratios.forest_delete_ratio = DEFAULT_FOREST_DELETE_RATIO;
    args.op_ratios.tree_move_ratio = DEFAULT_MOVE_RATIO;

    // parse arguments
    int c;

    while((c = getopt(argc, argv, "o:t:f:M:l:i:d:L:I:D:m:?")) != -1) {
            switch(c) {
                    case 'o':
                            args.operation_count = atoi(optarg);
                            break;
                    case 't':
                            args.thread_count = atoi(optarg);
                            break;
                    case 'f':
                            args.forest_size = atoi(optarg);
                            break;
                    case 'M':
                            args.max_key_value = atoi(optarg);
                            break;
                    case 'l':
                            args.op_ratios.tree_lookup_ratio = atoi(optarg);
                            break;
                    case 'i':
                            args.op_ratios.tree_insert_ratio = atoi(optarg);
                            break;
                    case 'd':
                            args.op_ratios.tree_delete_ratio = atoi(optarg);
                            break;
                    case 'L':
                            args.op_ratios.forest_lookup_ratio = atoi(optarg);
                            break;
                    case 'I':
                            args.op_ratios.forest_insert_ratio = atoi(optarg);
                            break;
                    case 'D':
                            args.op_ratios.forest_delete_ratio = atoi(optarg);
                            break;
                    case 'm':
                            args.op_ratios.tree_move_ratio = atoi(optarg);
                            break;
                    case '?':
                            print_usage();
            }
    }

    // some argument checking, but not very extensive
    if(args.operation_count == 0) {
            std::cerr << "operation count must be higher than 0" << std::endl;
            exit(0);
    }

    if(args.forest_size == 0) {
            std::cerr << "forest size must be higher than 0" << std::endl;
            exit(0);
    }

    if(args.max_key_value == 0) {
            std::cerr << "max value must be higher than 0" << std::endl;
            exit(0);
    }

    // print values
    std::cout << "Using parameters:\n"
            "\toperation count     = " << args.operation_count << "\n"
            "\tthread count        = " << args.thread_count << "\n"
            "\tforest size         = " << args.forest_size << "\n"
            "\tmax value           = " << args.max_key_value << "\n"
            "\ttree lookup ratio   = " << args.op_ratios.tree_lookup_ratio << "\n"
            "\ttree insert ratio   = " << args.op_ratios.tree_insert_ratio << "\n"
            "\ttree delete ratio   = " << args.op_ratios.tree_delete_ratio << "\n"
            "\tforest lookup ratio = " << args.op_ratios.forest_lookup_ratio << "\n"
            "\tforest insert ratio = " << args.op_ratios.forest_insert_ratio << "\n"
            "\tforest delete ratio = " << args.op_ratios.forest_delete_ratio << "\n"
            "\ttree move ratio     = " << args.op_ratios.tree_move_ratio << "\n"
                << std::endl;
}

static rbtree_t *init_rbtree(unsigned max_val) {
    rbtree_t *ret = rbtree_alloc();
    unsigned size = 0;

    while(size < max_val / 2) {
        unsigned val = rand() % max_val;

        if(!rbtree_insert(ret, val, val + 1)) {
            size++;
        }
    }

    return ret;
}

static unsigned get_total_op_ratio(op_ratios_t &op) {
    return op.tree_lookup_ratio +
        op.tree_insert_ratio +
        op.tree_delete_ratio +
        op.forest_lookup_ratio +
        op.forest_insert_ratio +
        op.forest_delete_ratio +
        op.tree_move_ratio;
}

static operation_t *init_operations(cmd_line_args_t &args) {
    unsigned total_ops = get_total_op_ratio(args.op_ratios);
    operation_t *ret = (operation_t *)malloc(sizeof(operation_t) * total_ops);

    unsigned last = 0;

    for(unsigned i = last;i < last + args.op_ratios.tree_lookup_ratio;i++) {
        ret[i].type = OP_TREE_LOOKUP;
        ret[i].fun = &op_tree_lookup;
    }

    last += args.op_ratios.tree_lookup_ratio;

    for(unsigned i = last;i < last + args.op_ratios.tree_insert_ratio;i++) {
        ret[i].type = OP_TREE_INSERT;
        ret[i].fun = &op_tree_insert;
    }

    last += args.op_ratios.tree_insert_ratio;

    for(unsigned i = last;i < last + args.op_ratios.tree_delete_ratio;i++) {
        ret[i].type = OP_TREE_DELETE;
        ret[i].fun = &op_tree_delete;
    }

    last += args.op_ratios.tree_delete_ratio;

    for(unsigned i = last;i < last + args.op_ratios.forest_lookup_ratio;i++) {
        ret[i].type = OP_FOREST_LOOKUP;
        ret[i].fun = &op_forest_lookup;
    }

    last += args.op_ratios.forest_lookup_ratio;

    for(unsigned i = last;i < last + args.op_ratios.forest_insert_ratio;i++) {
        ret[i].type = OP_FOREST_INSERT;
        ret[i].fun = &op_forest_insert;
    }

    last += args.op_ratios.forest_insert_ratio;

    for(unsigned i = last;i < last + args.op_ratios.forest_delete_ratio;i++) {
        ret[i].type = OP_FOREST_DELETE;
        ret[i].fun = &op_forest_delete;
    }

    last += args.op_ratios.forest_delete_ratio;

    for(unsigned i = last;i < last + args.op_ratios.tree_move_ratio;i++) {
        ret[i].type = OP_TREE_MOVE;
        ret[i].fun = &op_tree_move;
    }

    return ret;
}

static void *thread_fun(void *thread_data) {
    thread_main((thread_data_t *)thread_data);
    return NULL;
}

static unsigned thread_main(thread_data_t *thread_data, bool measure_time) {
    TM_THREAD_ENTER();

    unsigned time_ms = 0;
    unsigned ops = thread_data->operation_count;

    // start executing operations
    enter_barrier(thread_data->start_barrier);

    if(measure_time) {
        time_ms = get_time_ms();
    }

    while(ops > 0) {
        // choose an operation
        unsigned opidx = rand_r(&thread_data->rand_seed) % thread_data->total_operations;
        optype_t optype = thread_data->operations[opidx].type;
        operation_fun_t opfun = thread_data->operations[opidx].fun;

        thread_data->op_exec[optype] += opfun(TM_ARG thread_data);
        
        ops--;
    }

    // end executing operations
    enter_barrier(thread_data->end_barrier);

    if(measure_time) {
        time_ms = get_time_ms() - time_ms;
    }

    TM_THREAD_EXIT();

    return time_ms;
}

static unsigned op_tree_lookup(TM_ARGDECL thread_data_t *thread_data) {
    unsigned key = rand_r(&thread_data->rand_seed) % thread_data->max_key_value;
    unsigned rbtree_idx = rand_r(&thread_data->rand_seed) % thread_data->forest_size;
    rbtree_t *rbtree = thread_data->forest[rbtree_idx];
    unsigned ret = 0;

    START_ID(0);
    ret = (unsigned)TMrbtree_get(TM_ARG rbtree, key) == (key + 1);
    COMMIT

    return ret;
}

static unsigned op_tree_insert(TM_ARGDECL thread_data_t *thread_data) {
    unsigned key = rand_r(&thread_data->rand_seed) % thread_data->max_key_value;
    unsigned rbtree_idx = rand_r(&thread_data->rand_seed) % thread_data->forest_size;
    rbtree_t *rbtree = thread_data->forest[rbtree_idx];
    unsigned ret = 0;

    START_ID(1);
    ret = !TMrbtree_insert(TM_ARG rbtree, key, key + 1);
    COMMIT

    return ret;
}

static unsigned op_tree_delete(TM_ARGDECL thread_data_t *thread_data) {
    unsigned key = rand_r(&thread_data->rand_seed) % thread_data->max_key_value;
    unsigned rbtree_idx = rand_r(&thread_data->rand_seed) % thread_data->forest_size;
    rbtree_t *rbtree = thread_data->forest[rbtree_idx];
    unsigned ret = 0;

    START_ID(2);
    ret = TMrbtree_delete(TM_ARG rbtree, key);
    COMMIT

    return ret;
}

static unsigned op_forest_lookup(TM_ARGDECL thread_data_t *thread_data) {
    unsigned key = rand_r(&thread_data->rand_seed) % thread_data->max_key_value;
    unsigned forest_size = thread_data->forest_size;
    unsigned ret;

    START_ID(3);
    ret = 0;

    for(unsigned i = 0;i < forest_size;i++) {
        rbtree_t *rbtree = thread_data->forest[i];
        ret += (unsigned)TMrbtree_get(TM_ARG rbtree, key) == (key + 1);
    }
    COMMIT

    return ret;
}

static unsigned op_forest_insert(TM_ARGDECL thread_data_t *thread_data) {
    unsigned key = rand_r(&thread_data->rand_seed) % thread_data->max_key_value;
    unsigned forest_size = thread_data->forest_size;
    unsigned ret;

    START_ID(4);
    ret = 0;

    for(unsigned i = 0;i < forest_size;i++) {
        rbtree_t *rbtree = thread_data->forest[i];
        ret += !TMrbtree_insert(TM_ARG rbtree, key, key + 1);
    }
    COMMIT

    return ret;
}

static unsigned op_forest_delete(TM_ARGDECL thread_data_t *thread_data) {
    unsigned key = rand_r(&thread_data->rand_seed) % thread_data->max_key_value;
    unsigned forest_size = thread_data->forest_size;
    unsigned ret;

    START_ID(5);
    ret = 0;
    for(unsigned i = 0;i < forest_size;i++) {
        rbtree_t *rbtree = thread_data->forest[i];
        ret += TMrbtree_delete(TM_ARG rbtree, key);
    }
    COMMIT

    return ret;
}

static unsigned op_tree_move(TM_ARGDECL thread_data_t *thread_data) {
    unsigned key = rand_r(&thread_data->rand_seed) % thread_data->max_key_value;
    unsigned rbtree_src_idx = rand_r(&thread_data->rand_seed) % thread_data->forest_size;
    unsigned rbtree_dst_idx = rand_r(&thread_data->rand_seed) % thread_data->forest_size;
    rbtree_t *rbtree_src = thread_data->forest[rbtree_src_idx];
    rbtree_t *rbtree_dst = thread_data->forest[rbtree_dst_idx];
    unsigned ret;

    START_ID(6);
    ret = 0;

    if(TMrbtree_delete(TM_ARG rbtree_src, key)) {
        if(TMrbtree_insert(TM_ARG rbtree_dst, key, key + 1)) {
            // this means that element was not inserted
            ret = 1;
        }
    }
    COMMIT

    return ret;
}
