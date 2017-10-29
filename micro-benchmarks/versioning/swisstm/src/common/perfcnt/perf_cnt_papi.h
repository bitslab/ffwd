/**
 *  @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 */

#ifndef WLPDSTM_PERF_CNT_PAPI_H_
#define WLPDSTM_PERF_CNT_PAPI_H_

#include <stdint.h>
#include <pthread.h>
#include <papi.h>

#include "../timing.h"

#define MAX_EVENTS 8

#define INSTRUCTION_COUNT_EVENT_NAME PAPI_TOT_INS
#define CACHE_MISSES_EVENT_NAME PAPI_L2_TCM

#define INSTRUCTION_COUNT_EVENT_IDX 0
#define CACHE_MISSES_EVENT_IDX 1

namespace wlpdstm {
	
	class PerfCntPapi {
	public:
		void Init();
		void Start();
		void End();
		
		uint64_t GetElapsedCycles();
		uint64_t GetRetiredInstructions();
		uint64_t GetCacheMisses();
		
	public:
		static void GlobalInit();
		
	private:
		uint64_t cycles;
		uint64_t ret_inst;
		uint64_t cache_misses;

		int event_set;
		long_long counts[MAX_EVENTS];
        uint64_t cycles_before;
	};
	
	typedef PerfCntPapi PerfCntImpl;
}

inline void wlpdstm::PerfCntPapi::GlobalInit() {
	// initialize lib
    if(!PAPI_is_initialized()) {
        PAPI_library_init(PAPI_VER_CURRENT);
    }
}

inline void wlpdstm::PerfCntPapi::Init() {
	// initialize local state
	cycles = 0;
	ret_inst = 0;
	cache_misses = 0;

	// initialize lib
	PAPI_thread_init(pthread_self);

	// create event set
	event_set = PAPI_NULL;
	
	if(PAPI_create_eventset(&event_set) != PAPI_OK) {
		printf("Error creating event set\n");
	}

	// add events to the set
	if(PAPI_add_event(event_set, INSTRUCTION_COUNT_EVENT_NAME) != PAPI_OK) {
		printf("Error adding inst_count event\n");
	}

	if(PAPI_add_event(event_set, CACHE_MISSES_EVENT_NAME) != PAPI_OK) {
		printf("Error adding cache_misses event\n");
	}

	if(PAPI_start(event_set) != PAPI_OK) {
		printf("Error starting collection\n");
	}
}

inline void wlpdstm::PerfCntPapi::Start() {
	// read elapsed cycles
	cycles_before = get_clock_count();
	
	// read performance counters
	if(PAPI_accum(event_set, counts) != PAPI_OK) {
		printf("Error accum events\n");
	}
}

inline void wlpdstm::PerfCntPapi::End() {
	// read elapsed cycles
    uint64_t cycles_after = get_clock_count();
	cycles = cycles_after - cycles_before;
    cycles_before = cycles_after;
	
	// read performance counters
	if(PAPI_accum(event_set, counts) != PAPI_OK) {
		printf("Error reading events\n");
	}

	ret_inst = counts[INSTRUCTION_COUNT_EVENT_IDX];
	cache_misses = counts[CACHE_MISSES_EVENT_IDX];
}

inline uint64_t wlpdstm::PerfCntPapi::GetElapsedCycles() {
	return cycles;
}

inline uint64_t wlpdstm::PerfCntPapi::GetRetiredInstructions() {
	return ret_inst;
}

inline uint64_t wlpdstm::PerfCntPapi::GetCacheMisses() {
	return cache_misses;
}


#endif /* WLPDSTM_PERF_CNT_PAPI_H_ */
