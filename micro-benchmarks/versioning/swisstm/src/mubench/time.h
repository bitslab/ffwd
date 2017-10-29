/**
 * All functions related to time are defined in this header file. The
 * principle behind is to use as light-weight wrappers as possible.
 * 
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 */

#ifndef MUBENCH_TIME_H_
#define MUBENCH_TIME_H_

#include <stdint.h>

	
/**
 * Get time in ms since epoch. This function might be architecture
 * specific.
 */
uint64_t get_time_ms();

#define MILLISECONDS_IN_SECOND 1000
#define NANOSECONDS_IN_MILLISECOND 1000000

#if defined MUBENCH_LINUXOS || defined MUBENCH_SOLARIS

#define NANOSECONDS_IN_SECOND 1000000000

#include <ctime>
#include <sys/time.h>

// define functions as inline here
inline uint64_t get_time_ms() {
	struct timespec t;
	clock_gettime(CLOCK_REALTIME, &t);
	return t.tv_sec * MILLISECONDS_IN_SECOND +
		t.tv_nsec / NANOSECONDS_IN_MILLISECOND;
}

#elif defined MUBENCH_MACOS

#include <CoreServices/CoreServices.h>
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <time.h>

inline uint64_t get_time_ns() {
	uint64_t time = mach_absolute_time();
	Nanoseconds nano = AbsoluteToNanoseconds(*(AbsoluteTime *)&time);
	return *(uint64_t *)&nano;
}

inline uint64_t get_time_ms() {
	return get_time_ns() / NANOSECONDS_IN_MILLISECOND;
}


#endif /* arch */

#endif
