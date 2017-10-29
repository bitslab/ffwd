/**
 * Used for adaptive locking.
 *
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 */

#ifndef WLPDSTM_LARGE_LOCK_SET_H_
#define WLPDSTM_LARGE_LOCK_SET_H_

#include <stdint.h>
#include <string.h>

namespace wlpdstm {

	template <unsigned SIZE>
	class LargeLockSet {
			static const uint32_t MIN_SET_VERSION = 0;
			static const uint32_t MAX_SET_VERSION = (uint32_t)-1;

		public:
			LargeLockSet() {
				Reset();
			}

			bool Set(uint32_t el) {
				if(Contains(el)) {
					return true;
				}

				table[el] = version;
				return false;
			}

			bool Contains(uint32_t el) const {
				return table[el] == version;
			}

			void Clear() {
				if(version == MAX_SET_VERSION) {
					Reset();
				} else {
					++version;
				}
			}

		private:
			void Reset() {
				version = MIN_SET_VERSION + 1;
				memset(&table, MIN_SET_VERSION, sizeof(uint32_t) * SIZE);
			}

		private:
			uint32_t version;
			uint32_t table[SIZE];
	};
}

#endif /* WLPDSTM_LARGE_LOCK_SET_H_ */
