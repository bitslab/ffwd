/**
 * Memory manager that deallocates memory at the end of transaction. For
 * this to work correctly, memory writes have to be rolled back at the end
 * of a transaction.
 *
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 */

#ifndef WLPDSTM_MEMORY_TXEND_H_
#define WLPDSTM_MEMORY_TXEND_H_

#include <stdlib.h>

#include "../constants.h"
#include "../../common/atomic.h"
#include "../../common/alloc.h"

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
	
	// pointers for chaining
	PtrBufferT<SIZE> *next;
	
	PtrBufferT<SIZE> *prev;
	
	void *ptrs[SIZE];
	
	void Clear() {
		next_ptr = 0;
		rollback_next_ptr = 0;
		next = NULL;
		prev = NULL;
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
class MemoryManager : public Alloc {
public:
	// tx lifetime
	void TxStart();
	
	void TxCommit();
	
	void TxAbort();
	
	void *TxMalloc(size_t size);
	
	void TxFree(void *ptr);
	
	// init functions
	static void GlobalInit();
	
	void ThreadInit(unsigned tid);
	
private:
	PtrBuffer *AllocatePtrBuffer();
	
	PtrBuffer *GetCleanPtrBuffer();
	
	void PutReusable(PtrBuffer *first, PtrBuffer *last);
	
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
	
	// reusable pointer buffers
	PtrBuffer *reusable;
	
//#ifdef COLLECT_STATS
//	ThreadStatistics *stats;
//	
//public:	
//	void InitStats(ThreadStatistics *st) {
//		stats = st;
//	}
//	
//private:
//	void IncrementStatistics(Statistics::Type type) {
//		stats->stats->Increment(type);
//	}
//#else
//	void IncrementStatistics(Statistics::Type type) {
//		// empty
//	}
//#endif /* COLLECT_STATS */
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
	
	reusable = NULL;
}

inline void wlpdstm::MemoryManager::TxStart() {
	// empty
}

inline void wlpdstm::MemoryManager::TxCommit() {
	// forget allocations in this tx
	if(alloced_in_tx_old != alloced_in_tx) {
		PutReusable(alloced_in_tx_old->next, alloced_in_tx);
		
		// update old ptr
		alloced_in_tx = alloced_in_tx_old;
	}
	
	alloced_in_tx->next_ptr = 0;
	
	// free deletions now
	// rollback allocations
	for(PtrBuffer *curr = freed_in_tx_old;curr != NULL;curr = curr->next) {
		FreePtrs(curr);
	}
	
	// put reusable
	if(freed_in_tx != freed_in_tx_old) {
		PutReusable(freed_in_tx_old->next, freed_in_tx);
		freed_in_tx = freed_in_tx_old;
	}
	
	// maintain pointers
	freed_in_tx->next_ptr = 0;
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


#endif /* WLPDSTM_MEMORY_TXEND_H_ */
