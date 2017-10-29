/**
 * This memory manager leaks memory, but is useful for debugging.
 *
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 */

#ifndef WLPDSTM_MEMORY_LEAK_H_
#define WLPDSTM_MEMORY_LEAK_H_

#include <stdlib.h>

#include "../constants.h"
#include "../../common/alloc.h"

namespace wlpdstm {
		
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
	void TxStart(Epoch epoch);

	void TxCommit();
	
	void TxAbort();
	
	// allocation / deallocation
//	static void *Malloc(size_t size);
//	
//	static void Free(void *ptr);
	
	void *TxMalloc(size_t size);
	
	void TxFree(void *ptr);
	
	// init functions
	static void GlobalInit();
	
	void ThreadInit(unsigned tid);	

	void CleanupEpoch(Epoch epoch);
};
}

/////////////////////////////////////////
// memory manager implementation start //
/////////////////////////////////////////

inline void wlpdstm::MemoryManager::GlobalInit() {
	// empty
}

inline void wlpdstm::MemoryManager::ThreadInit(unsigned t) {
	// empty
}

inline void wlpdstm::MemoryManager::TxStart(Epoch epoch) {
	// empty
}

inline void wlpdstm::MemoryManager::TxCommit() {
	// empty
}

inline void wlpdstm::MemoryManager::TxAbort() {
	// empty
}

//inline void *wlpdstm::MemoryManager::Malloc(size_t size) {
//	void *ret = ::malloc(size);
//	return ret;
//}

//inline void wlpdstm::MemoryManager::Free(void *ptr) {
//	::free(ptr);
//}

inline void *wlpdstm::MemoryManager::TxMalloc(size_t size) {
	return Malloc(size);
}

inline void wlpdstm::MemoryManager::TxFree(void *ptr) {
	// empty
}

inline void wlpdstm::MemoryManager::CleanupEpoch(Epoch epoch) {
	// empty
}

#endif /* WLPDSTM_MEMORY_LEAK_H_ */
