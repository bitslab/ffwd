/**
 * Memory manager must postpone deallocation until the epoch gets gc-ed.
 *
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 */

#ifndef WLPDSTM_MEMORY_EPOCH_H_
#define WLPDSTM_MEMORY_EPOCH_H_

#include <stdlib.h>

#include "../../common/atomic.h"
#include "../../common/alloc.h"

#include "../constants.h"

#define MEMORY_BLOCK_BUFFER_SIZE 2048

namespace wlpdstm {

	typedef struct MemoryBlockS {
		void *memory;
		MemoryBlockS *next;
	} MemoryBlock;

	typedef struct MemoryBlockBufS {
		MemoryBlock memory_blocks[MEMORY_BLOCK_BUFFER_SIZE];
		MemoryBlockBufS *next;
	} MemoryBlockBuf;

	class MemoryManager : public Alloc {
		public:
			// tx lifetime
			void TxStart(Epoch epoch);

			void TxCommit();

			void TxAbort();

			void *TxMalloc(size_t size);

			void TxFree(void *ptr);

			// init functions
			static void GlobalInit();

			void ThreadInit(unsigned tid);

			void CleanupEpoch(Epoch epoch);

		private:
			MemoryBlock *AllocateFreeBlocks();
			MemoryBlock *GetFreeMemoryBlock();

		private:
			Epoch current_epoch;
			MemoryBlock *alloced_in_tx;
			MemoryBlock *freed_in_tx;

			MemoryBlock *free_blocks;
			volatile MemoryBlock *freed_by_cleanup;

			MemoryBlockBuf *block_buf;

			MemoryBlock *to_delete_in_epoch[MAX_EPOCH];
	};
}

inline void wlpdstm::MemoryManager::TxStart(Epoch epoch) {
	current_epoch = epoch;
}

inline void wlpdstm::MemoryManager::TxCommit() {
	if(freed_in_tx != NULL) {
		to_delete_in_epoch[current_epoch] = freed_in_tx;
		freed_in_tx = NULL;
	}

	// do nothing for allocations in this transaction
	if(alloced_in_tx != NULL) {
		// return alloced to free blocks
		MemoryBlock *curr = alloced_in_tx;

		while(curr->next != NULL) {
			curr = curr->next;
		}

		curr->next = free_blocks;
		free_blocks = alloced_in_tx;
		alloced_in_tx = NULL;
	}
}

// allocated memory cannot be deallocated immediately, as transaction
// might have written something to it, which it needs to revert later
inline void wlpdstm::MemoryManager::TxAbort() {
	if(alloced_in_tx != NULL) {
		to_delete_in_epoch[current_epoch] = alloced_in_tx;
		alloced_in_tx = NULL;
	}

	if(freed_in_tx != NULL) {
		MemoryBlock *curr = freed_in_tx;

		while(curr->next != NULL) {
			curr = curr->next;
		}

		curr->next = free_blocks;
		free_blocks = freed_in_tx;
		freed_in_tx = NULL;
	}
}

inline void *wlpdstm::MemoryManager::TxMalloc(size_t size) {
	void *ret = Malloc(size);
	MemoryBlock *mem_block = GetFreeMemoryBlock();
	mem_block->memory = ret;
	mem_block->next = alloced_in_tx;
	alloced_in_tx = mem_block;
	return ret;
}

inline void wlpdstm::MemoryManager::TxFree(void *ptr) {
	MemoryBlock *mem_block = GetFreeMemoryBlock();
	mem_block->memory = ptr;
	mem_block->next = freed_in_tx;
	freed_in_tx = mem_block;	
}

// init functions
inline void wlpdstm::MemoryManager::GlobalInit() {
	// do nothing
}

inline void wlpdstm::MemoryManager::ThreadInit(unsigned tid) {
	current_epoch = MIN_EPOCH - 1;
	alloced_in_tx = NULL;
	freed_in_tx = NULL;
	freed_by_cleanup = NULL;
	block_buf = NULL;

	free_blocks = AllocateFreeBlocks();

	// initialize to_delete array
	for(unsigned i = 0;i < MAX_EPOCH;i++) {
		to_delete_in_epoch[i] = NULL;
	}
}

inline wlpdstm::MemoryBlock *wlpdstm::MemoryManager::AllocateFreeBlocks() {
	MemoryBlockBuf *buf = (MemoryBlockBuf *)Malloc(sizeof(MemoryBlockBuf));

	// add this buffer to the list of all buffers, so buffers can be deallocated too
	buf->next = block_buf;
	block_buf = buf;

	// link all blocks
	for(unsigned i = 0;i < MEMORY_BLOCK_BUFFER_SIZE - 1;i++) {
		buf->memory_blocks[i].next = &buf->memory_blocks[i + 1];
	}

	buf->memory_blocks[MEMORY_BLOCK_BUFFER_SIZE - 1].next = NULL;

	// return the first block
	return &buf->memory_blocks[0];
}

inline wlpdstm::MemoryBlock *wlpdstm::MemoryManager::GetFreeMemoryBlock() {
	if(free_blocks == NULL) {
		if(freed_by_cleanup != NULL) {
			free_blocks = (MemoryBlock *)freed_by_cleanup;
			freed_by_cleanup = NULL;
		} else {
			free_blocks = AllocateFreeBlocks();
		}
	}

	MemoryBlock *ret = free_blocks;
	free_blocks = free_blocks->next;
	return ret;
}

inline void wlpdstm::MemoryManager::CleanupEpoch(Epoch epoch) {
	MemoryBlock *to_delete = to_delete_in_epoch[epoch];

	if(to_delete == NULL) {
		return;
	}

	to_delete_in_epoch[epoch] = NULL;

	// free all memory
	MemoryBlock *curr = to_delete;

	while(curr->next != NULL) {
		Free(curr->memory);
		curr = curr->next;
	}

	Free(curr->memory);

	// put back into freed memory
	while(true) {
		curr->next = (MemoryBlock *)freed_by_cleanup;

		if(atomic_cas_no_barrier(&freed_by_cleanup, curr->next, to_delete)) {
			break;
		}
	}

	// TODO: think about freeing blocks of buffers if needed
}

#endif /* WLPDSTM_MEMORY_EPOCH_H_ */
