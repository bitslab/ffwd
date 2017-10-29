/**
 * Simple profiling that collects basic information from profiled transactions.
 *
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#ifndef WLPDSTM_PROFILING_SIMPLE_H_
#define WLPDSTM_PROFILING_SIMPLE_H_

#include <stdint.h>

#include "../random.h"
#include "../sampling.h"
#include "../word.h"
#include "../timing.h"

namespace wlpdstm {

	template<class STATS, class STATS_TYPE>
	class SimpleProfiling {
		public:
			static void GlobalInit() {
				// empty
			}
			
			void ThreadInit(STATS *stats_) {
				stats = stats_;
				sampling.ThreadInit(&rand);
			}
			
			void ThreadShutdown() {
				// empty
			}
			
			/////////////////////
			// callbacks start //
			/////////////////////

			void ThreadStart();
			void ThreadEnd();
			
			void TxStartStart(int lex_tx_id);
			void TxStartEnd();
				
			void TxCommitStart();
			void TxCommitEnd();

			void ReadWordStart(Word *addr);			
			void ReadWordEnd(Word res);
				
			void WriteWordStart(Word *addr, Word val, Word mask = LOG_ENTRY_UNMASKED);			
			void WriteWordEnd();
				
			void TxMallocStart(size_t size);
			void TxMallocEnd(void *ptr);
				
			void TxFreeStart(void *ptr, size_t size);
			void TxFreeEnd();
				
			void TxRestartStart();
			void TxRestartEnd();
				
			void TxAbortStart();			
			void TxAbortEnd();
			
			///////////////////
			// callbacks end //
			///////////////////

		protected:
			STATS *stats;
			RandomTxSampling sampling;
			Random rand;

			uint64_t tx_start_cycles;
			uint64_t last_event_cycles;

			// it seems last event that occurred is needed here too to 
			// correctly handle restarts
	};
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::SimpleProfiling<STATS, STATS_TYPE>::TxStartStart(int lex_tx_id) {
	sampling.TxStart();

	if(sampling.IsSampling()) {
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_PROFILED_ALL);
		tx_start_cycles = get_clock_count();
	}
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::SimpleProfiling<STATS, STATS_TYPE>::TxStartEnd() {
	if(sampling.IsSampling()) {
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_DURATION_START, get_clock_count() - tx_start_cycles);
	}		
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::SimpleProfiling<STATS, STATS_TYPE>::TxCommitStart() {
	if(sampling.IsSampling()) {
		last_event_cycles = get_clock_count();
	}	
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::SimpleProfiling<STATS, STATS_TYPE>::TxCommitEnd() {
	if(sampling.IsSampling()) {
		uint64_t commit_end_cycles = get_clock_count();
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_DURATION_ALL, commit_end_cycles - tx_start_cycles);
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_DURATION_COMMIT, commit_end_cycles - last_event_cycles);
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_PROFILED_COMMITS);
	}	
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::SimpleProfiling<STATS, STATS_TYPE>::ReadWordStart(Word *addr) {
	if(sampling.IsSampling()) {
		last_event_cycles = get_clock_count();
	}	
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::SimpleProfiling<STATS, STATS_TYPE>::ReadWordEnd(Word res) {
	if(sampling.IsSampling()) {
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_DURATION_READ, get_clock_count() - last_event_cycles);
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_PROFILED_READS);
	}
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::SimpleProfiling<STATS, STATS_TYPE>::WriteWordStart(Word *addr, Word val, Word mask) {
	if(sampling.IsSampling()) {
		last_event_cycles = get_clock_count();
	}
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::SimpleProfiling<STATS, STATS_TYPE>::WriteWordEnd() {
	if(sampling.IsSampling()) {
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_DURATION_WRITE, get_clock_count() - last_event_cycles);
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_PROFILED_WRITES);
	}	
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::SimpleProfiling<STATS, STATS_TYPE>::TxMallocStart(size_t size) {
	if(sampling.IsSampling()) {
		last_event_cycles = get_clock_count();
	}
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::SimpleProfiling<STATS, STATS_TYPE>::TxMallocEnd(void *ptr) {
	if(sampling.IsSampling()) {
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_DURATION_ALLOC, get_clock_count() - last_event_cycles);
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_PROFILED_ALLOCS);
	}	
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::SimpleProfiling<STATS, STATS_TYPE>::TxFreeStart(void *ptr, size_t size) {
	if(sampling.IsSampling()) {
		last_event_cycles = get_clock_count();
	}
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::SimpleProfiling<STATS, STATS_TYPE>::TxFreeEnd() {
	if(sampling.IsSampling()) {
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_DURATION_FREE, get_clock_count() - last_event_cycles);
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_PROFILED_FREES);
	}		
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::SimpleProfiling<STATS, STATS_TYPE>::TxRestartStart() {
	if(sampling.IsSampling()) {
		last_event_cycles = get_clock_count();
	}
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::SimpleProfiling<STATS, STATS_TYPE>::TxRestartEnd() {
	if(sampling.IsSampling()) {
		uint64_t restart_end_cycles = get_clock_count();
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_DURATION_ALL, restart_end_cycles - tx_start_cycles);
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_DURATION_RESTART, restart_end_cycles - last_event_cycles);
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_PROFILED_RESTARTS);
	}	
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::SimpleProfiling<STATS, STATS_TYPE>::TxAbortStart() {
	if(sampling.IsSampling()) {
		last_event_cycles = get_clock_count();
	}	
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::SimpleProfiling<STATS, STATS_TYPE>::TxAbortEnd() {
	if(sampling.IsSampling()) {
		uint64_t abort_end_cycles = get_clock_count();
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_DURATION_ALL, abort_end_cycles - tx_start_cycles);
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_DURATION_ABORT, abort_end_cycles - last_event_cycles);
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_PROFILED_ABORTS);
	}
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::SimpleProfiling<STATS, STATS_TYPE>::ThreadStart() {
	// empty
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::SimpleProfiling<STATS, STATS_TYPE>::ThreadEnd() {
	// empty
}

#endif /* WLPDSTM_PROFILING_SIMPLE_H_ */
