/**
 * Energy related stuff.
 *
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#ifndef WLPDSTM_ENERGY_H_
#define WLPDSTM_ENERGY_H_

#include <stdint.h>

#include "random.h"

namespace wlpdstm {

	class Energy {
		public:
			void start_measurement();
			uint64_t get_consumed_energy();

		protected:
			Random random;
	};
}

inline void wlpdstm::Energy::start_measurement() {
	// TODO
}

inline uint64_t wlpdstm::Energy::get_consumed_energy() {
	return random.Get();
}

#endif /* WLPDSTM_ENERGY_H_ */
