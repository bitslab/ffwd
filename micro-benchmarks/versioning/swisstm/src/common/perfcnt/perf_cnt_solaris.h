/**
 *  @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 */

#ifndef WLPDSTM_PERF_CNT_SOLARIS_H_
#define WLPDSTM_PERF_CNT_SOLARIS_H_

#include "../timing.h"

#include <libcpc.h>
#include <errno.h>
#include <sys/lwp.h>
#include <stdint.h>

#define INSTRUCTION_COUNT_EVENT_NAME "Instr_cnt"
#define CACHE_MISSES_EVENT_NAME "DC_miss"

namespace wlpdstm {

	class PerfCntSolaris {
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

			cpc_t *cpc;
			cpc_set_t *set;
			int ret_inst_ind;
			int cache_misses_ind;
			cpc_buf_t *diff, *after, *before;
//            uint64_t cycles_before;
	};

	typedef PerfCntSolaris PerfCntImpl;
}

inline void wlpdstm::PerfCntSolaris::GlobalInit() {
	/* nothing */
}

inline void wlpdstm::PerfCntSolaris::Init() {
	// initialize local state
	cycles = 0;
	ret_inst = 0;	

	// I skip error handling (maybe not the best thing to do, but I assume Niagara2 CPU here)
	cpc = cpc_open(CPC_VER_CURRENT);
	set = cpc_set_create(cpc);
	ret_inst_ind = cpc_set_add_request(cpc, set, INSTRUCTION_COUNT_EVENT_NAME, 0, CPC_COUNT_USER, 0, NULL);
	cache_misses_ind = cpc_set_add_request(cpc, set, CACHE_MISSES_EVENT_NAME, 0, CPC_COUNT_USER, 0, NULL);
	
	diff = cpc_buf_create(cpc, set);
	after = cpc_buf_create(cpc, set);
	before = cpc_buf_create(cpc, set);
	
	cpc_bind_curlwp(cpc, set, 0);
}

inline void wlpdstm::PerfCntSolaris::Start() {
	// read performance counters
	cpc_set_sample(cpc, set, before);

	// read elapsed cycles
	cycles = get_clock_count();
}

inline void wlpdstm::PerfCntSolaris::End() {
	// read elapsed cycles
    cycles = get_clock_count() - cycles;

	// read performance counters
	cpc_set_sample(cpc, set, after);
	cpc_buf_sub(cpc, diff, after, before);
	cpc_buf_get(cpc, diff, ret_inst_ind, &ret_inst);
	cpc_buf_get(cpc, diff, cache_misses_ind, &cache_misses);
}

inline uint64_t wlpdstm::PerfCntSolaris::GetElapsedCycles() {
	return cycles;
}

inline uint64_t wlpdstm::PerfCntSolaris::GetRetiredInstructions() {
	return ret_inst;
}

inline uint64_t wlpdstm::PerfCntSolaris::GetCacheMisses() {
	return cache_misses;
}


#endif /* WLPDSTM_PERF_CNT_SOLARIS_H_ */
