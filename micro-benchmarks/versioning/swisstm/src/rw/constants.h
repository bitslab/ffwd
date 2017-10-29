/**
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#ifndef WLPDSTM_RW_CONSTANTS_H_
#define WLPDSTM_RW_CONSTANTS_H_

#include "../common/constants.h"

namespace wlpdstm {

	// this depends on the number of available hw threads
	// it can safely be a lower number than the number of
	// hw threads, as some background threads need to execute too
	// (e.g. 60 or maybe even 56 is enough on Niagara)
	const unsigned MAX_THREADS = 64;
}

#endif /* WLPDSTM_RW_CONSTANTS_H_ */
