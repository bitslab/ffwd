/**
 * Lock implementation.
 *
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#ifndef WLPDSTM_LOCK_H_
#define WLPDSTM_LOCK_H_

#include "word.h"
#include "atomic.h"

namespace wlpdstm {

	class cas_lock {
		const static Word CLEAR = 0;
		const static Word TAKEN = 1;

		public:
			void init() volatile {
				flag = CLEAR;
			}

			void release() volatile {
				atomic_store_release(&flag, CLEAR);
			}

			bool try_lock() volatile {
				return atomic_cas_acquire(&flag, CLEAR, TAKEN);
			}

			void lock() volatile {
				while(!atomic_cas_acquire(&flag, CLEAR, TAKEN)) {
					if(flag == TAKEN) {
						continue;
					}
				}
			}

		protected:
			volatile Word flag;
	};
}

#endif /* WLPDSTM_LOCK_H_ */
