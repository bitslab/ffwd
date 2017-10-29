/**
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#ifndef WLPDSTM_EPOCHSTM_CONSTANTS_H_
#define WLPDSTM_EPOCHSTM_CONSTANTS_H_

#include <stdint.h>

#include "../common/constants.h"

//#define EPOCH_SIZE_IN_BYTES_1
#define EPOCH_SIZE_IN_BYTES_2

namespace wlpdstm {

	// this depends on the number of available hw threads
	// it can safely be a lower number than the number of
	// hw threads, as some background threads need to execute too
	// (e.g. 60 or maybe even 56 is enough on Niagara)
	const unsigned MAX_THREADS = 64;

	const unsigned MAX_CLEANUP_THREADS = 128;

	const unsigned MIN_EPOCH = 1;
	const unsigned NO_EPOCH = MIN_EPOCH - 1;

#ifdef EPOCH_SIZE_IN_BYTES_1
	
	typedef uint8_t Epoch;

	const unsigned MAX_EPOCH = 255;
	const uint8_t EPOCH_MASK = 0xff;

#elif defined EPOCH_SIZE_IN_BYTES_2

	typedef uint16_t Epoch;
	
	const unsigned MAX_EPOCH = (1 << 16) - 1;
	const uint16_t EPOCH_MASK = 0xffff;

#endif /* EPOCH_SIZE_IN_BYTES_1 */

	const unsigned EPOCHS_IN_WORD = sizeof(Word) / sizeof(Epoch);
	const unsigned OREC_EPOCH_SIZE_WORDS = MAX_THREADS / EPOCHS_IN_WORD + (MAX_THREADS % EPOCHS_IN_WORD ? 1 : 0);

	// this should help with speeding up reader checks and cleanups
	const uint64_t NO_READERS_64 = (uint64_t)0;
	const Word NO_READERS_WORD = (Word)0;	
}

#endif /* WLPDSTM_EPOCHSTM_CONSTANTS_H_ */
