/**
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 */

#include "transaction.h"

wlpdstm::VersionLock wlpdstm::TransactionDynamic::version_lock_table[FULL_VERSION_LOCK_TABLE_SIZE];

CACHE_LINE_ALIGNED wlpdstm::GlobalTimestamp wlpdstm::TransactionDynamic::commit_ts;

#ifndef GREEN_CM
CACHE_LINE_ALIGNED wlpdstm::GlobalTimestamp wlpdstm::TransactionDynamic::cm_ts;
#endif /* GREEN_CM */

CACHE_LINE_ALIGNED wlpdstm::PaddedWord wlpdstm::TransactionDynamic::minimum_observed_ts;

CACHE_LINE_ALIGNED wlpdstm::PaddedSpinTryLock wlpdstm::TransactionDynamic::minimum_observed_ts_lock;

wlpdstm::TransactionDynamic *wlpdstm::TransactionDynamic::transactions[MAX_THREADS];

Word wlpdstm::TransactionDynamic::thread_count;

CACHE_LINE_ALIGNED wlpdstm::PaddedBool wlpdstm::TransactionDynamic::synchronization_in_progress;

#ifdef PRIVATIZATION_QUIESCENCE
CACHE_LINE_ALIGNED volatile Word wlpdstm::TransactionDynamic::quiescence_timestamp_array[MAX_THREADS];
#endif /* PRIVATIZATION_QUIESCENCE */

#ifdef DYNAMIC_DYNAMIC
wlpdstm::TransactionDynamic::SetFunPtr wlpdstm::TransactionDynamic::set_fun_ptr_table[TM_IMPLEMENTATION_VARIANT_COUNT];
#endif /* DYNAMIC_DYNAMIC */
