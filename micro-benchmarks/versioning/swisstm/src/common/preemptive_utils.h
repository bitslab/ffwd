#ifndef PREEMPTIVE_UTILS_H_
#define PREEMPTIVE_UTILS_H_

#ifdef WLPDSTM_LINUXOS
#define WLPDSTM_PTHREAD_YIELD
#elif defined WLPDSTM_MACOS
#define WLPDSTM_SCHED_YIELD
#elif defined WLPDSTM_SOLARIS
#define WLPDSTM_SCHED_YIELD
#endif

#ifdef WLPDSTM_PTHREAD_YIELD
#include <pthread.h>

namespace wlpdstm {

	// give up on this processor slice
	inline void pre_yield() {
		::pthread_yield();
	}
}

#elif defined WLPDSTM_SCHED_YIELD

#include <sched.h>

namespace wlpdstm {
	
	// give up on this processor slice
	inline void pre_yield() {
		::sched_yield();
	}
}

#endif /* WLPDSTM_LINUXOS */

#endif // PREEMPTIVE_UTILS_H_

