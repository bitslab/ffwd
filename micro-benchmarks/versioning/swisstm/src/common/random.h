/**
 *  @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 */

#ifndef WLPDSTM_RANDOM_H_
#define WLPDSTM_RANDOM_H_

#include <stdlib.h>

namespace wlpdstm {

	class Random {
		public:
			int Get() {
				unsigned ret = rand_r(&seed);
				return ret;
			}

		private:
			unsigned seed;
	};
}

#endif /* WLPDSTM_RANDOM_H_ */
