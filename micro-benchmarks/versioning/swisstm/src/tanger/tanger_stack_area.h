/**
 * This class (and all of stack handling) is based on tinySTM-0.9.9.
 *
 *  @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#ifndef WLPDSTM_TANGER_STACK_AREA_H_
#define WLPDSTM_TANGER_STACK_AREA_H_

#include <stdint.h>
#include <stdlib.h>

#include "../stm/common/tls.h"
#include "../stm/common/cache_aligned_alloc.h"

namespace wlpdstm {

	class StackArea : public CacheAlignedAlloc {
	public:
		void ThreadInit() {
			low = high = 0;
			data = NULL;
			size = 0;			
		}
		
		uintptr_t low;
		uintptr_t high;
		void *data;
		size_t size;
	};

	class ThreadStackArea : public Tls<StackArea, false, true> {
	};
}

#define TLS_STACK_AREA wlpdstm::ThreadStackArea::Get()

#endif /* WLPDSTM_TANGER_STACK_AREA_H_ */
