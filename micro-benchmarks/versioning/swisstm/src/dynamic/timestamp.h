#ifndef WLPDSTM_TIMESTAMP_H_
#define WLPDSTM_TIMESTAMP_H_

#include "constants.h"
#include "../common/atomic.h"
#include "../common/word.h"

namespace wlpdstm {

	class GlobalTimestamp {
		public:
			GlobalTimestamp() : ts(MINIMUM_TS + 1) {  }

			Word readCurrentTs() const {
				return ts;
			}

			Word readCurrentTsAcquire() const {
				return atomic_load_acquire(&ts);
			}

			Word getNextTsFull() {
				return fetch_and_inc_full(&ts);
			}

			Word getNextTsAcquire() {
				return fetch_and_inc_acquire(&ts);
			}

			Word getNextTsRelease() {
				return fetch_and_inc_release(&ts);
			}
		
			Word getNextTs() {
				return fetch_and_inc_no_barrier(&ts);
			}

			Word GenerateTsGV4();

			void restart() {
				ts = MINIMUM_TS + 1;
			}

		private:
			Word ts;

			char padding[CACHE_LINE_SIZE_BYTES - sizeof(Word)];
	};
}

inline Word wlpdstm::GlobalTimestamp::GenerateTsGV4() {
	Word old_val = atomic_load_no_barrier(&ts);
	Word replaced_val = compare_and_swap_release(&ts, old_val, old_val + 1);

	if(replaced_val != old_val) {
		return replaced_val;
	}

	return old_val + 1;
}

#endif // WLPDSTM_TIMESTAMP_H_
