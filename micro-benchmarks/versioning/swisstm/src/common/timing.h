/**
 *  @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#ifndef WLPDSTM_TIMING_H_
#define WLPDSTM_TIMING_H_

#include <stdint.h>
#include <time.h>
#include "atomic.h"

// NOTE: AMD64 does not work with a "=A" (ret) version
// have to check if Intel in 64 bit mode would work
inline uint64_t get_clock_count() {
#ifdef WLPDSTM_X86
#  ifdef WLPDSTM_32
	uint64_t ret;
	__asm__ volatile ("rdtsc" : "=A" (ret));
	return ret;
#  elif defined WLPDSTM_64
	uint32_t low, high;
	__asm__ volatile("rdtsc" : "=a" (low), "=d" (high));
	return (uint64_t)low | (((uint64_t)high) << 32);
#  endif /* WLPDSTM_64 */
#elif defined WLPDSTM_SPARC && defined WLPDSTM_64
	uint64_t ret;
	__asm__ __volatile__ ("rd %%tick, %0" : "=r" (ret));
	return ret;
#else
	// avoid compiler complaints
	return 0; 
#endif /* arch */
}

inline void wait_cycles(uint64_t cycles) {
	uint64_t start = get_clock_count();
	uint64_t end;

	while(true) {
		end = get_clock_count();

		if(end - start > cycles) {
			break;
		}
	}
}

inline void sleep_ns(uint64_t ns) {
#ifdef WLPDSTM_SOLARIS
	timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = ns;
	nanosleep(&ts, NULL);
#endif /* WLPDSTM_SOLARIS */
}

#endif /* WLPDSTM_TIMING_H_ */

