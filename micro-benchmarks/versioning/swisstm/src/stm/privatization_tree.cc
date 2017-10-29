/**
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 */

#include "privatization_tree.h"

// this is const, but should be aligned and occupy whole cache lines
CACHE_LINE_ALIGNED unsigned wlpdstm::PrivatizationTree::map_count_to_root_idx[MAX_THREADS];

CACHE_LINE_ALIGNED wlpdstm::PaddedWord wlpdstm::PrivatizationTree::tree_nodes[PRIVATIZATION_TREE_NODE_COUNT];

CACHE_LINE_ALIGNED wlpdstm::PaddedWord wlpdstm::PrivatizationTree::proxy_nodes[PRIVATIZATION_TREE_PROXY_COUNT];

// for debug
CACHE_LINE_ALIGNED wlpdstm::PaddedSpinTryLock wlpdstm::PrivatizationTree::tree_node_locks[PRIVATIZATION_TREE_NODE_COUNT];
CACHE_LINE_ALIGNED wlpdstm::PaddedSpinTryLock wlpdstm::PrivatizationTree::proxy_node_locks[PRIVATIZATION_TREE_PROXY_COUNT];

Word wlpdstm::PrivatizationTree::last_sibling_ts[MAX_THREADS];
Word wlpdstm::PrivatizationTree::last_my_ts[MAX_THREADS];
Word wlpdstm::PrivatizationTree::last_parent_ts[MAX_THREADS];
