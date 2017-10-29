/**
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 */

#ifndef WLPDSTM_PRIVATIZATION_TREE_H_
#define WLPDSTM_PRIVATIZATION_TREE_H_

#include <assert.h>
#include <stdio.h>

#include "constants.h"
#include "../common/word.h"
#include "../common/padded.h"
#include "../common/atomic.h"

#define PRIVATIZATION_TREE_NODE_COUNT (MAX_THREADS * 2 - 1)
#define PRIVATIZATION_TREE_PROXY_COUNT (PRIVATIZATION_TREE_NODE_COUNT >> 1)

namespace wlpdstm {

	class PrivatizationTree {
		public:
			void ThreadInit(unsigned tid, Word *tc);

			void setNonMinimumTs(Word ts);

//			void setMinimumTs();

			void wait(Word ts);

			static void Clean();

			static void GlobalInit();

		private:
			static void InitializeCountToRootMap();

			static unsigned get_parent_node_idx(unsigned node_idx);

			static unsigned get_sibling_node_idx(unsigned node_idx);

			static unsigned get_proxy_node_idx(unsigned node_idx);

			unsigned get_root_node_idx(unsigned count);

			unsigned is_root_node_idx(unsigned node_idx);

			static unsigned wlpdstm_log2(uint64_t input);

			Word calculate_parent_ts(Word my_ts, Word sibling_ts);

		////////////////
		// data start //
		////////////////
		private:
			unsigned my_tree_node_idx;

			volatile Word *thread_count;

			static unsigned map_count_to_root_idx[MAX_THREADS];

			static PaddedWord tree_nodes[PRIVATIZATION_TREE_NODE_COUNT];

			static PaddedWord proxy_nodes[PRIVATIZATION_TREE_PROXY_COUNT];

			// for debug
			static PaddedSpinTryLock tree_node_locks[PRIVATIZATION_TREE_NODE_COUNT];
			static PaddedSpinTryLock proxy_node_locks[PRIVATIZATION_TREE_PROXY_COUNT];

			static Word last_sibling_ts[MAX_THREADS];
			static Word last_my_ts[MAX_THREADS];
			static Word last_parent_ts[MAX_THREADS];

		//////////////
		// data end //
		//////////////		
	};
}

inline unsigned wlpdstm::PrivatizationTree::get_parent_node_idx(unsigned node_idx) {
	return (node_idx >> 1) + MAX_THREADS;
}

inline unsigned wlpdstm::PrivatizationTree::get_sibling_node_idx(unsigned node_idx) {
	return (node_idx & ~1) + (1 - (node_idx & 1));
}

inline unsigned wlpdstm::PrivatizationTree::get_proxy_node_idx(unsigned node_idx) {
	return node_idx >> 1;
}

inline unsigned wlpdstm::PrivatizationTree::get_root_node_idx(unsigned count) {
	return map_count_to_root_idx[count - 1];
}

inline unsigned wlpdstm::PrivatizationTree::is_root_node_idx(unsigned node_idx) {
	return node_idx == get_root_node_idx(*thread_count);
}

/* this works only for two threads
inline void wlpdstm::PrivatizationTree::setNonMinimumTs(Word ts) {
	atomic_store_full(&(tree_nodes[my_tree_node_idx].val), ts);
	
	unsigned node_idx = my_tree_node_idx;
	
	while(!is_root_node_idx(node_idx)) {
		unsigned sibling_node_idx = get_sibling_node_idx(node_idx);
		unsigned parent_node_idx = get_parent_node_idx(node_idx);
		ts = tree_nodes[node_idx].val;
		
		Word sibling_ts_1, sibling_ts_2;
		Word parent_ts;
		
		do {
			sibling_ts_1 = atomic_load_acquire(&(tree_nodes[sibling_node_idx].val));
			parent_ts = calculate_parent_ts(ts, sibling_ts_1);
			atomic_store_release(&(tree_nodes[parent_node_idx].val), parent_ts);
			sibling_ts_2 = atomic_load_acquire(&(tree_nodes[sibling_node_idx].val));
		} while(sibling_ts_1 != sibling_ts_2);

		last_sibling_ts[node_idx] = sibling_ts_1;
		last_my_ts[node_idx] = ts;
		last_parent_ts[node_idx] = parent_ts;

		node_idx = parent_node_idx;
	}
}
*/
 
inline void wlpdstm::PrivatizationTree::setNonMinimumTs(Word ts) {
	atomic_store_full(&(tree_nodes[my_tree_node_idx].val), ts);

	unsigned node_idx = my_tree_node_idx;

	while(!is_root_node_idx(node_idx)) {
		unsigned sibling_node_idx = get_sibling_node_idx(node_idx);
		unsigned proxy_node_idx = get_proxy_node_idx(node_idx);
		unsigned parent_node_idx = get_parent_node_idx(node_idx);
		ts = tree_nodes[node_idx].val;

		Word sibling_ts_1, sibling_ts_2;
		Word parent_ts;
		bool should_break;

		do {
			sibling_ts_1 = atomic_load_acquire(&(tree_nodes[sibling_node_idx].val));
			parent_ts = calculate_parent_ts(ts, sibling_ts_1);			
			atomic_store_full(&(proxy_nodes[proxy_node_idx].val), parent_ts);
			sibling_ts_2 = atomic_load_acquire(&(tree_nodes[sibling_node_idx].val));
		} while(sibling_ts_1 != sibling_ts_2);

		Word proxy_ts_1, proxy_ts_2;
		
		do {
			proxy_ts_1 = atomic_load_acquire(&(proxy_nodes[proxy_node_idx].val));

			if(proxy_ts_1 == tree_nodes[parent_node_idx].val) {
				should_break = true;
			} else {
				atomic_store_full(&(tree_nodes[parent_node_idx].val), proxy_ts_1);
				should_break = false;
			}

			proxy_ts_2 = atomic_load_acquire(&(proxy_nodes[proxy_node_idx].val));
		} while(proxy_ts_1 != proxy_ts_2);

		if(should_break) {
			break;
		}

		node_idx = parent_node_idx;
	}
}

/*
inline void wlpdstm::PrivatizationTree::setNonMinimumTs(Word ts) {
	tree_nodes[my_tree_node_idx].val = ts;
	
	unsigned node_idx = my_tree_node_idx;
	
	while(!is_root_node_idx(node_idx)) {
		unsigned sibling_node_idx = get_sibling_node_idx(node_idx);
//		unsigned proxy_node_idx = get_proxy_node_idx(node_idx);
		unsigned parent_node_idx = get_parent_node_idx(node_idx);
		ts = tree_nodes[node_idx].val;

		while(!tree_node_locks[parent_node_idx].try_lock()) {
		}
		
		Word sibling_ts = tree_nodes[sibling_node_idx].val;
		Word parent_ts = calculate_parent_ts(ts, sibling_ts);
		tree_nodes[parent_node_idx].val = parent_ts;

		tree_node_locks[parent_node_idx].release();
		
		node_idx = parent_node_idx;
	}
}
*/

inline Word wlpdstm::PrivatizationTree::calculate_parent_ts(Word my_ts, Word sibling_ts) {
	Word ret;

	if(my_ts == MINIMUM_TS || (sibling_ts != MINIMUM_TS && sibling_ts < my_ts)) {
		//return sibling_ts;
		ret = sibling_ts;
	} else {
		ret = my_ts;
	}

//	printf("my_ts=%d sibling_ts=%d ret=%d\n", my_ts, sibling_ts, ret);

	return ret;

	//return my_ts;

//	if(sibling_ts == MINIMUM_TS || my_ts < sibling_ts) {
//		return my_ts;
//	}

//	return sibling_ts;
}

/*
inline void wlpdstm::PrivatizationTree::setMinimumTs() {
	//tree_nodes[my_tree_node_idx].val = MINIMUM_TS;
	atomic_store_release(&(tree_nodes[my_tree_node_idx].val), MINIMUM_TS);
	
	unsigned node_idx = my_tree_node_idx;
	
	while(!is_root_node_idx(node_idx)) {
		unsigned sibling_node_idx = get_sibling_node_idx(node_idx);
		unsigned proxy_node_idx = get_proxy_node_idx(node_idx);
		unsigned parent_node_idx = get_parent_node_idx(node_idx);
		Word ts = tree_nodes[node_idx].val;
		
		Word sibling_ts_1, sibling_ts_2;
		Word parent_ts;
		
		do {
			//sibling_ts_1 = tree_nodes[sibling_node_idx].val;
			sibling_ts_1 = atomic_load_acquire(&(tree_nodes[sibling_node_idx].val));
			parent_ts = calculate_parent_ts(ts, sibling_ts_1);
			
//			if(proxy_nodes[proxy_node_idx].val == parent_ts) {
//				// no need to go up and update
//				return;
//			} else {
				//proxy_nodes[proxy_node_idx].val = parent_ts;
				atomic_store_release(&(proxy_nodes[proxy_node_idx].val), parent_ts);
//			}
			
			sibling_ts_2 = atomic_load_acquire(&(tree_nodes[sibling_node_idx].val));
		} while(sibling_ts_1 != sibling_ts_2);
		
		Word proxy_ts_1, proxy_ts_2;
		
		do {
//			proxy_ts_1 = proxy_nodes[proxy_node_idx].val;
			proxy_ts_1 = atomic_load_acquire(&(proxy_nodes[proxy_node_idx].val));
			//tree_nodes[parent_node_idx].val = proxy_ts_1;
			atomic_store_release(&(tree_nodes[parent_node_idx].val), proxy_ts_1);
//			proxy_ts_2 = atomic_load_acquire(&(proxy_nodes[proxy_node_idx].val));
			proxy_ts_2 = atomic_load_acquire(&(proxy_nodes[proxy_node_idx].val));
		} while(proxy_ts_1 != proxy_ts_2);
		
		node_idx = parent_node_idx;
	}
}*/

inline void wlpdstm::PrivatizationTree::wait(Word ts) {
	// wait only on the root of the tree
	while(true) {
		unsigned root_node_idx = get_root_node_idx(*thread_count);
		//Word root_ts = tree_nodes[root_node_idx].val;
		Word root_ts = atomic_load_acquire(&(tree_nodes[root_node_idx].val));

		if(root_ts == MINIMUM_TS || ts <= root_ts) {
			break;
		}
	}
}


inline void wlpdstm::PrivatizationTree::GlobalInit() {
	InitializeCountToRootMap();
	Clean();
}

#define MAX_SHIFT 63

/**
 * This function is only called during initialization, so it does not need to
 * be particularly efficient.
 */
inline unsigned wlpdstm::PrivatizationTree::wlpdstm_log2(uint64_t input) {
	unsigned i =  0;

	while(i <= MAX_SHIFT) {
		if(input <= (uint64_t)1 << i) {
			return i;
		}
		
		++i;
	}
	
	assert(0);
	return 0; // to avoid compiler complaints
}

inline void wlpdstm::PrivatizationTree::InitializeCountToRootMap() {
	for(unsigned i = 0;i < MAX_THREADS;i++) {
		unsigned go_up_count = wlpdstm_log2((uint64_t)i + 1);
		unsigned root_idx = i;

		for(unsigned j = 0;j < go_up_count;j++) {
			root_idx = get_parent_node_idx(root_idx);
		}

		map_count_to_root_idx[i] = root_idx;
	}
}

inline void wlpdstm::PrivatizationTree::Clean() {
	for(unsigned i = 0;i < PRIVATIZATION_TREE_NODE_COUNT;i++) {
		tree_nodes[i].val = MINIMUM_TS;
	}
	
	for(unsigned i = 0;i < PRIVATIZATION_TREE_PROXY_COUNT;i++) {
		proxy_nodes[i].val = MINIMUM_TS;
	}
}

inline void wlpdstm::PrivatizationTree::ThreadInit(unsigned tid, Word *tc) {
	thread_count = tc;
	my_tree_node_idx = tid;
}


#endif /* WLPDSTM_PRIVATIZATION_TREE_H_ */
