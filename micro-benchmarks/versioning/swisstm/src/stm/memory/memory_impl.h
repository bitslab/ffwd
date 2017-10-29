/**
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 */

#ifndef WLPDSTM_MEMORY_IMPL_H_
#define WLPDSTM_MEMORY_IMPL_H_

#include <stdlib.h>

#include "../../common/atomic.h"
#include "../constants.h"
#include "../stats.h"

//////////////////////////////
// include allocators start //
//////////////////////////////

#ifdef MM_TBB_MALLOC
#include "scalable_allocator.h"
#elif defined MM_HOARD
#include "mm_hoard.h"
#elif defined MM_LEA
#include "mm_lea.h"
#endif


////////////////////////////
// include allocators end //
////////////////////////////

namespace wlpdstm {

	////////////////////
	// helper classes //
	////////////////////

	/**
	 * Struct used for buffering freed pointers.
	 */
	template<unsigned SIZE = 2048>
	struct PtrBufferT {
		// next to acquire
		unsigned next_ptr;

		// where to rollback
		unsigned rollback_next_ptr;

		// timestamp
		Word ts;

		bool init_ts;

		// pointers for chaining
		PtrBufferT<SIZE> *next;

		PtrBufferT<SIZE> *prev;

		void *ptrs[SIZE];

		void Clear() {
			next_ptr = 0;
			rollback_next_ptr = 0;
			next = NULL;
			prev = NULL;
			init_ts = false;
		}

		void Rollback() {
			next_ptr = rollback_next_ptr;
		}

		void Commit() {
			rollback_next_ptr = next_ptr;
		}

		bool IsFull() const {
			return next_ptr == SIZE;
		}

		unsigned GetSize() const {
			return SIZE;
		}
	};

	typedef PtrBufferT<> PtrBuffer;

	////////////////////
	// memory manager //
	////////////////////

	/**
	 * This is a main memory manager class. Typically, one instance of
	 * this class should be created per thread.
	 */
	class MemoryManager {
		public:
			// tx lifetime
			void TxStart();

			template <class TX>
			void TxCommit(Word commit_ts);

			void TxAbort();

			// allocation / deallocation
			static void *Malloc(size_t size);

			static void Free(void *ptr);

			void *TxMalloc(size_t size);

			void TxFree(void *ptr);

			// init functions
			static void GlobalInit();

			void ThreadInit(unsigned tid);

		private:
			PtrBuffer *AllocatePtrBuffer();

			PtrBuffer *GetCleanPtrBuffer();

			void PutReusable(PtrBuffer *first, PtrBuffer *last);

			void PutFreedLogically(PtrBuffer *first, PtrBuffer *last);

			template <class TX>
			void FreeDominated();

			void FreePtrs(PtrBuffer *buf);

			void FreeMemoryPtr(void *ptr);

		private:
			// mem dealloced in this tx
			PtrBuffer *freed_in_tx;
			PtrBuffer *freed_in_tx_old;

			// mem allocated in this tx
			PtrBuffer *alloced_in_tx;
			PtrBuffer *alloced_in_tx_old;

			// logically freed pointers
			PtrBuffer *freed_logically;

			// reusable pointer buffers
			PtrBuffer *reusable;

			Word last_minimum_ts;

#ifdef COLLECT_STATS
			ThreadStatistics *stats;

		public:	
			void InitStats(ThreadStatistics *st) {
				stats = st;
			}

		private:
			void IncrementStatistics(Statistics::Type type) {
				stats->IncrementStatistics(type);
			}
#else
			void IncrementStatistics(Statistics::Type type) {
				// empty
			}
#endif /* COLLECT_STATS */
	};
}

/////////////////////////////////////////
// memory manager implementation start //
/////////////////////////////////////////

inline void wlpdstm::MemoryManager::GlobalInit() {
	// empty
}

inline void wlpdstm::MemoryManager::ThreadInit(unsigned t) {
	// initialize buffer pointers
	freed_in_tx = AllocatePtrBuffer();
	freed_in_tx_old = freed_in_tx;

	alloced_in_tx = AllocatePtrBuffer();
	alloced_in_tx_old = alloced_in_tx;

	freed_logically = NULL;

	reusable = NULL;

	last_minimum_ts = MINIMUM_TS;
}

inline void wlpdstm::MemoryManager::TxStart() {
	// empty
}

template <class TX>
inline void wlpdstm::MemoryManager::TxCommit(Word commit_ts) {
	// forget allocations in this tx
	if(alloced_in_tx_old != alloced_in_tx) {
		PutReusable(alloced_in_tx_old->next, alloced_in_tx);

		// update old ptr
		alloced_in_tx = alloced_in_tx_old;
	}

	alloced_in_tx->next_ptr = 0;

	// store deletions in freed_logically
	freed_in_tx->Commit();

	if(freed_in_tx_old != freed_in_tx) {
		// copy timestamp to first pointer buffer
		freed_in_tx_old->ts = commit_ts;
		freed_in_tx_old->init_ts = true;

		// chain list at the beginning of freed_logically
		PutFreedLogically(freed_in_tx_old, freed_in_tx->prev);

		// update old ptr
		freed_in_tx_old = freed_in_tx;

		// try to actually release some memory
		FreeDominated<TX>();
	}
}

inline void wlpdstm::MemoryManager::TxAbort() {
	// forget deallocations in this tx
	freed_in_tx_old->Rollback();

	if(freed_in_tx_old != freed_in_tx) {
		PutReusable(freed_in_tx_old->next, freed_in_tx);

		// update old ptr
		freed_in_tx = freed_in_tx_old;
	}

	// rollback allocations
	// TODO: perhaps postpone this as well? although it does not seem
	// to make too much sense or buy anything
	for(PtrBuffer *curr = alloced_in_tx_old;curr != NULL;curr = curr->next) {
		FreePtrs(curr);
	}

	// put reusable
	if(alloced_in_tx != alloced_in_tx_old) {
		PutReusable(alloced_in_tx_old->next, alloced_in_tx);
		alloced_in_tx = alloced_in_tx_old;
	}

	// maintain pointers
	alloced_in_tx->next_ptr = 0;
}

inline wlpdstm::PtrBuffer *wlpdstm::MemoryManager::AllocatePtrBuffer() {
	PtrBuffer *ret = (PtrBuffer *)Malloc(sizeof(PtrBuffer));
	ret->Clear();
	return ret;
}

inline wlpdstm::PtrBuffer *wlpdstm::MemoryManager::GetCleanPtrBuffer() {
	if(reusable != NULL) {
		PtrBuffer *ret = reusable;
		PtrBuffer *next = reusable->next;
		reusable = next;

		if(next != NULL) {
			next->prev = NULL;
		}

		ret->Clear();

		return ret;
	}

	return AllocatePtrBuffer();
}

inline void wlpdstm::MemoryManager::PutReusable(PtrBuffer *first, PtrBuffer *last) {
	last->next = reusable;

	if(reusable != NULL) {
		reusable->prev = last;
	}

	first->prev = NULL;
	reusable = first;
}

inline void wlpdstm::MemoryManager::PutFreedLogically(PtrBuffer *first, PtrBuffer *last) {
	last->next = freed_logically;

	if(freed_logically != NULL) {
		freed_logically->prev = last;
	}

	first->prev = NULL;
	freed_logically = first;
}

inline void *wlpdstm::MemoryManager::Malloc(size_t size) {
#ifdef MM_TBB_MALLOC
	void *ret = scalable_malloc(size);
#elif defined MM_HOARD
	void *ret = hoard_malloc(size);
#elif defined MM_LEA
	void *ret = lea_malloc(size);
#else
	void *ret = ::malloc(size);
#endif

	return ret;
}

inline void wlpdstm::MemoryManager::Free(void *ptr) {
#ifdef MM_TBB_MALLOC
	scalable_free(ptr);
#elif defined MM_HOARD
	hoard_free(ptr);
#elif defined MM_LEA
	lea_free(ptr);
#else
	::free(ptr);
#endif
}

inline void *wlpdstm::MemoryManager::TxMalloc(size_t size) {
	// first allocate memory
	void *ret = Malloc(size);

	// remember allocation
	alloced_in_tx->ptrs[alloced_in_tx->next_ptr++] = ret;

	// make sure freed_in_tx is never full
	if(alloced_in_tx->IsFull()) {
		PtrBuffer *pbuf = GetCleanPtrBuffer();
		alloced_in_tx->next = pbuf;
		pbuf->prev = alloced_in_tx;
		alloced_in_tx = pbuf;
	}

	return ret;
}

inline void wlpdstm::MemoryManager::TxFree(void *ptr) {
	// remember deallocation
	freed_in_tx->ptrs[freed_in_tx->next_ptr++] = ptr;

	// make sure freed_in_tx is never full
	if(freed_in_tx->IsFull()) {
		PtrBuffer *pbuf = GetCleanPtrBuffer();
		freed_in_tx->next = pbuf;
		pbuf->prev = freed_in_tx;
		freed_in_tx = pbuf;
	}
}

template <class TX>
inline void wlpdstm::MemoryManager::FreeDominated() {
#ifdef DETAILED_STATS
	IncrementStatistics(Statistics::FREE_DOMINATED);
#endif /* DETAILED_STATS */
	Word minimum_ts = TX::GetMinimumObservedTs();

	if(last_minimum_ts == minimum_ts) {
		minimum_ts = TX::UpdateMinimumObservedTs();
#ifdef DETAILED_STATS
		IncrementStatistics(Statistics::UPDATE_MINIMUM_TS);
#endif /* DETAILED_STATS */

		if(last_minimum_ts == minimum_ts) {
			return;
		}
	}

	last_minimum_ts = minimum_ts;

	// find the first one to delete
	PtrBuffer *curr = freed_logically;

	while(curr != NULL) {
		if(curr->init_ts) {
			if(curr->ts <= minimum_ts) {
				break;
			}
		}

		curr = curr->next;
	}

	// delete all ptrs in dominated buffers and put buffers in reusable list
	if(curr != NULL) {
		PtrBuffer *first = curr;
		PtrBuffer *last;

		if(first->prev == NULL) {
			freed_logically = NULL;
		} else {
			first->prev->next = NULL;
		}

		// all the rest are also dominated
		while(curr != NULL) {
			last = curr;
			FreePtrs(curr);
			curr = curr->next;
		}

		PutReusable(first, last);
	}
}

inline void wlpdstm::MemoryManager::FreeMemoryPtr(void *ptr) {
	Free(ptr);
}

inline void wlpdstm::MemoryManager::FreePtrs(PtrBuffer *buf) {
	unsigned end = buf->next_ptr;

	for(unsigned i = 0;i < end;i++) {
		FreeMemoryPtr(buf->ptrs[i]);
	}
}

///////////////////////////////////////
// memory manager implementation end //
///////////////////////////////////////


#endif /* WLPDSTM_MEMORY_IMPL_H_ */
