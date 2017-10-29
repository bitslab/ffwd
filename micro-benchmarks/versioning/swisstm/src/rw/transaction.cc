/**
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#include "transaction.h"

CACHE_LINE_ALIGNED wlpdstm::TransactionRw::OwnershipRecord wlpdstm::TransactionRw::orec_table[OWNERSHIP_TABLE_SIZE];

CACHE_LINE_ALIGNED wlpdstm::TransactionRw *wlpdstm::TransactionRw::transactions[MAX_THREADS];

CACHE_LINE_ALIGNED wlpdstm::PaddedWord wlpdstm::TransactionRw::thread_count;
