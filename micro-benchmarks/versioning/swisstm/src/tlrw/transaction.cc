/**
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#include "transaction.h"

CACHE_LINE_ALIGNED wlpdstm::TransactionTlrw::OwnershipRecord wlpdstm::TransactionTlrw::orec_table[OWNERSHIP_TABLE_SIZE];

CACHE_LINE_ALIGNED wlpdstm::TransactionTlrw *wlpdstm::TransactionTlrw::transactions[MAX_THREADS];

CACHE_LINE_ALIGNED wlpdstm::PaddedWord wlpdstm::TransactionTlrw::thread_count;
