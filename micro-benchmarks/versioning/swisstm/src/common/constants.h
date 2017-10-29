/**
 * These are the general constants used in all STM implementations.
 *
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 */

#ifndef WLPDSTM_CONSTANTS_H_
#define WLPDSTM_CONSTANTS_H_

#ifdef WLPDSTM_X86
#define CACHE_LINE_SIZE_BYTES 64
#define LOG_CACHE_LINE_SIZE_BYTES 6
#elif defined WLPDSTM_SPARC
#define CACHE_LINE_SIZE_BYTES 64
#define LOG_CACHE_LINE_SIZE_BYTES 6
#endif /* arch */

#define CACHE_LINE_ALIGNED __attribute__((aligned(CACHE_LINE_SIZE_BYTES)))

#include "word.h"

namespace wlpdstm {

#ifdef WLPDSTM_32
	const unsigned ADDRESS_SPACE_SIZE = 32;
	const unsigned LOG_BYTES_IN_WORD = 2;
#elif defined WLPDSTM_64
	const unsigned ADDRESS_SPACE_SIZE = 64;
	const unsigned LOG_BYTES_IN_WORD = 3;
#endif /* X86_64 */

	static const Word LOG_ENTRY_UNMASKED = ~0x0;

	const uintptr_t BYTES_IN_WORD = 1 << LOG_BYTES_IN_WORD;
	const uintptr_t WORD_ADDRESS_MASK = BYTES_IN_WORD - 1;

	inline Word *get_word_address(void *address) {
		return (Word *)((uintptr_t)address & ~WORD_ADDRESS_MASK);
	}

	inline unsigned get_byte_in_word_index(void *address) {
		return (uintptr_t)address & (uintptr_t)WORD_ADDRESS_MASK;
	}

	union word_to_bytes {
		uint8_t bytes[BYTES_IN_WORD];
		Word word;
	};

    const int MAX_LEXICAL_TX = 64;
}

#define NO_LEXICAL_TX -1

// The size of start_buf paddig in cache lines.
// This should be bigger than the biggest start_buf, or will cause cache misses.
#define START_BUF_PADDING_SIZE 10

#endif /* WLPDSTM_CONSTANTS_H_ */
