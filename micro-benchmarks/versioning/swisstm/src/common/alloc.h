/**
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#ifndef WLPDSTM_ALLOC_H_
#define WLPDSTM_ALLOC_H_

namespace wlpdstm {

	class Alloc {
		public:
			static void *Malloc(size_t size) {
				return ::malloc(size);
			}

			static void Free(void *ptr) {
				::free(ptr);
			}
	};

	/**
	 * Use non-tx free/malloc provided by wlpdstm.
	 */
	class WlpdstmAlloced {
	public:
		void* operator new(size_t size) {
			return Alloc::Malloc(size);
		}
		
		void* operator new[](size_t size) {
			return Alloc::Malloc(size);
		}
		
		void operator delete(void* ptr) {
			Alloc::Free(ptr);
		}
		
		void operator delete[](void *ptr) {
			return Alloc::Free(ptr);
		}		
	};	
}

#endif /* WLPDSTM_ALLOC_H_ */
