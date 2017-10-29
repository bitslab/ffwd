/**
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#include "transaction.h"

CACHE_LINE_ALIGNED wlpdstm::TransactionEpoch::OwnershipRecord wlpdstm::TransactionEpoch::orec_table[OWNERSHIP_TABLE_SIZE];

CACHE_LINE_ALIGNED wlpdstm::TransactionEpoch::PaddedTxCounter wlpdstm::TransactionEpoch::counters[MAX_THREADS];

CACHE_LINE_ALIGNED wlpdstm::TransactionEpoch *wlpdstm::TransactionEpoch::transactions[MAX_THREADS];

CACHE_LINE_ALIGNED wlpdstm::PaddedWord wlpdstm::TransactionEpoch::thread_count;

//CACHE_LINE_ALIGNED wlpdstm::TransactionEpoch::CleanupWorkItem wlpdstm::TransactionEpoch::cleanup_queue[CLEANUP_WORK_ITEMS];
//
//CACHE_LINE_ALIGNED wlpdstm::PaddedWord wlpdstm::TransactionEpoch::cleanup_counter;
//
//CACHE_LINE_ALIGNED wlpdstm::PaddedWord wlpdstm::TransactionEpoch::cleanup_done_counter;
//
//Word wlpdstm::TransactionEpoch::cleanup_snapshot_thread_count;
//
//pthread_t wlpdstm::TransactionEpoch::cleanup_threads[MAX_CLEANUP_THREADS];
//
//unsigned wlpdstm::TransactionEpoch::cleanup_thread_count;
//
//#ifdef MM_EPOCH
//CACHE_LINE_ALIGNED wlpdstm::PaddedWord wlpdstm::TransactionEpoch::cleanup_memory_counter;
//
//CACHE_LINE_ALIGNED wlpdstm::PaddedWord wlpdstm::TransactionEpoch::cleanup_memory_done_counter;
//#endif /* MM_EPOCH */

#ifdef USE_TOUCHED_ORECS
CACHE_LINE_ALIGNED volatile uint8_t wlpdstm::TransactionEpoch::touched_orecs[MAX_THREADS][TOUCHED_ORECS_ARRAY_SIZE];

//CACHE_LINE_ALIGNED uint8_t wlpdstm::TransactionEpoch::cleanup_touched_orecs[TOUCHED_ORECS_ARRAY_SIZE];
#endif /* TOUCHED_ORECS */

#ifdef USE_MINIMUM_READER_VERSION
CACHE_LINE_ALIGNED wlpdstm::TransactionEpoch::PaddedTxCounter wlpdstm::TransactionEpoch::min_counter;
#endif /* USE_MINIMUM_READER_VERSION */


