/**
 * Allocate memory aligned on a boundary. Boundary is a power of two.
 * 
 * @author Aleksandar Dragojevic
 */

#ifndef WLPDSTM_CACHE_ALIGNED_ALLOC_H_
#define WLPDSTM_CACHE_ALIGNED_ALLOC_H_

#include <stdint.h>
#include <stdlib.h>

#include "constants.h"
#include "alloc.h"

namespace wlpdstm {

	template<uintptr_t BOUNDARY, class MM>
	void *malloc_aligned(size_t size) {
		size_t actual_size = size + BOUNDARY + sizeof(void *);
		uintptr_t mem_block = (uintptr_t)MM::Malloc(actual_size);
		uintptr_t ret = ((mem_block + sizeof(void *) + BOUNDARY) & ~(BOUNDARY - 1));
		void **back_ptr = (void **)(ret - sizeof(void *));
		*back_ptr = (void *)mem_block;
		return (void *)ret;		
	}

	template<class MM>
	void free_aligned(void *ptr) {
		void **back_ptr = (void **)((uintptr_t)ptr - sizeof(void *));
		MM::Free(*back_ptr);
	}

	template<uintptr_t BOUNDARY, class MM>
	class AlignedAlloc {
		public:
			void *operator new(size_t size) {
				return malloc_aligned<BOUNDARY, MM>(size);
			}

			void operator delete(void *ptr) {
				free_aligned<MM>(ptr);
			}
	};

	typedef AlignedAlloc<CACHE_LINE_SIZE_BYTES, Alloc> CacheAlignedAlloc;
}

#endif /* WLPDSTM_CACHE_ALIGNED_ALLOC_H_ */
