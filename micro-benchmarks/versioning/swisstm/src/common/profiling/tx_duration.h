/**
 * Adaptively profile transaction durations. The goal of this profiler is to
 * gather more precise information about transaction durations without overheads of
 * collecting detailed information about reads, writes, etc.
 *
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#ifndef WLPDSTM_PROFILING_TX_DURATION_H_
#define WLPDSTM_PROFILING_TX_DURATION_H_

//#define MEASURE_NON_TX_CODE

#include <stdint.h>

#include "../random.h"
#include "../word.h"
#include "../timing.h"
#include "../print_utils.h"

namespace wlpdstm {

	template<class STATS, class STATS_TYPE>
	class TxDurationProfiling {
		public:
			static void GlobalInit() {
				// empty
			}
			
			void ThreadInit(STATS *stats_);
			void ThreadShutdown();

			bool ShouldProfile() const;
			void StartProfilingTx();
			void EndProfilingTx(uint64_t tx_duration);
			static uint64_t CalculateEstimate(uint64_t old_value, uint64_t new_value);

			void EstimateProfilingCosts();

			/////////////////////
			// callbacks start //
			/////////////////////

			void ThreadStart();
			void ThreadEnd();

			void TxStartStart(int lex_tx_id);
			void TxStartStartForEstimatingCosts();
			void TxStartEnd();
#ifdef MEASURE_NON_TX_CODE
			void ProfileNonTxCode();
#endif /* MEASURE_NON_TX_CODE */

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
			Random rand;

			uint64_t tx_start_cycles;
#ifdef MEASURE_NON_TX_CODE
			uint64_t tx_commit_cycles;
#endif /* MEASURE_NON_TX_CODE */

            // TODO: think if having this per lexical transaction makes any difference
            uint64_t estimated_tx_duration;
            uint64_t estimated_profiling_cost;
            uint64_t current_profiling_cost;

            uint32_t sampling_period;
            bool profiling_this_tx;
			unsigned consecutive_count;
#ifdef MEASURE_NON_TX_CODE
			bool last_committed;
#endif /* MEASURE_NON_TX_CODE */

            // costs of different operations
            uint64_t cost_tx_start_start;
			uint64_t cost_tx_commit_end;
			uint64_t cost_tx_restart_end;
			uint64_t cost_tx_abort_end;

			///////////////
			// constants //
			///////////////
			static const unsigned MINIMUM_SAMPLING_PERIOD = 1;
			static const unsigned SHOULD_PROFILE_RAND_VALUE = 0;

			// overhead should be less than 1 / ACCEPTABLE_OVERHEAD_SHIFT
			// 1 / SHIFT_1_PCT = 1 / 128
			static const unsigned SHIFT_1_PCT = 7;
			static const unsigned SHIFT_05_PCT = 8;
			static const unsigned SHIFT_01_PCT = 10;
			static const unsigned ACCEPTABLE_OVERHEAD_SHIFT = SHIFT_1_PCT;

			static const unsigned PROFILING_COSTS_ESTIMATION_ITERATIONS = 1000;
			static const unsigned PROFILING_COSTS_ESTIMATION_LEXICAL_TX_ID = 1;

			static const unsigned CONSECUTIVE_TRANSACTION_PROFILE = 16;
	};
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::TxDurationProfiling<STATS, STATS_TYPE>::ThreadInit(STATS *stats_) {
	stats = stats_;

	estimated_tx_duration = 0;
	estimated_profiling_cost = 0;
	current_profiling_cost = 0;
	consecutive_count = 0;

	sampling_period = MINIMUM_SAMPLING_PERIOD;

	EstimateProfilingCosts();

	profiling_this_tx = false;
#ifdef MEASURE_NON_TX_CODE
	last_committed = false;
#endif /* MEASURE_NON_TX_CODE */
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::TxDurationProfiling<STATS, STATS_TYPE>::StartProfilingTx() {
	if(consecutive_count-- == 0) {
		consecutive_count = rand.Get() % CONSECUTIVE_TRANSACTION_PROFILE + 1;

		if((rand.Get() % sampling_period) == SHOULD_PROFILE_RAND_VALUE) {
			profiling_this_tx = true;
		} else {
			profiling_this_tx = false;
		}
	}

	if(profiling_this_tx) {
		current_profiling_cost = 0;
	}
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::TxDurationProfiling<STATS, STATS_TYPE>::EndProfilingTx(uint64_t tx_duration) {
	// update estimates
	if(estimated_tx_duration == 0) {
		estimated_tx_duration = tx_duration - current_profiling_cost;
		estimated_profiling_cost = current_profiling_cost;
	} else {
		estimated_tx_duration = CalculateEstimate(estimated_tx_duration, tx_duration - current_profiling_cost);
		estimated_profiling_cost = CalculateEstimate(estimated_profiling_cost, current_profiling_cost);
	}

	// adjust sampling period
	sampling_period = (estimated_profiling_cost << ACCEPTABLE_OVERHEAD_SHIFT) / estimated_tx_duration;

	if(sampling_period < MINIMUM_SAMPLING_PERIOD) {
		sampling_period = MINIMUM_SAMPLING_PERIOD;
	}
}

template<class STATS, class STATS_TYPE>
inline uint64_t wlpdstm::TxDurationProfiling<STATS, STATS_TYPE>::CalculateEstimate(uint64_t old_value, uint64_t new_value) {
	return (old_value + new_value) >> 1;
}

template<class STATS, class STATS_TYPE>
inline bool wlpdstm::TxDurationProfiling<STATS, STATS_TYPE>::ShouldProfile() const {
	return profiling_this_tx;
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::TxDurationProfiling<STATS, STATS_TYPE>::ThreadShutdown() {
	// empty
}

#ifdef MEASURE_NON_TX_CODE
template<class STATS, class STATS_TYPE>
inline void wlpdstm::TxDurationProfiling<STATS, STATS_TYPE>::ProfileNonTxCode() {
	stats->IncrementStatisticsPerTxType(STATS_TYPE::NON_TX_DURATION, get_clock_count() - tx_commit_cycles);
	stats->IncrementStatisticsPerTxType(STATS_TYPE::NON_TX_PROFILED);
}
#endif /* MEASURE_NON_TX_CODE */

template<class STATS, class STATS_TYPE>
inline void wlpdstm::TxDurationProfiling<STATS, STATS_TYPE>::TxStartStart(int lex_tx_id) {
#ifdef MEASURE_NON_TX_CODE
	if(ShouldProfile() && last_committed) {
		ProfileNonTxCode();
	}
#endif /* MEASURE_NON_TX_CODE */

	StartProfilingTx();

	if(ShouldProfile()) {
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_PROFILED_ALL);
		tx_start_cycles = get_clock_count();
		current_profiling_cost += cost_tx_start_start;
	}
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::TxDurationProfiling<STATS, STATS_TYPE>::TxStartEnd() {
	// empty
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::TxDurationProfiling<STATS, STATS_TYPE>::TxCommitStart() {
	// empty
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::TxDurationProfiling<STATS, STATS_TYPE>::TxCommitEnd() {
	if(ShouldProfile()) {
		uint64_t commit_end_cycles = get_clock_count();
		uint64_t tx_duration = commit_end_cycles - tx_start_cycles;
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_DURATION_COMMITTED, tx_duration);
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_PROFILED_COMMITS);

		current_profiling_cost += cost_tx_commit_end;
		EndProfilingTx(tx_duration);

#ifdef MEASURE_NON_TX_CODE
		last_committed = true;
		tx_commit_cycles = get_clock_count();
#endif /* MEASURE_NON_TX_CODE */
	}	
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::TxDurationProfiling<STATS, STATS_TYPE>::ReadWordStart(Word *addr) {
	// empty
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::TxDurationProfiling<STATS, STATS_TYPE>::ReadWordEnd(Word res) {
	// empty
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::TxDurationProfiling<STATS, STATS_TYPE>::WriteWordStart(Word *addr, Word val, Word mask) {
	// empty
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::TxDurationProfiling<STATS, STATS_TYPE>::WriteWordEnd() {
	// empty
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::TxDurationProfiling<STATS, STATS_TYPE>::TxMallocStart(size_t size) {
	// empty
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::TxDurationProfiling<STATS, STATS_TYPE>::TxMallocEnd(void *ptr) {
	// empty
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::TxDurationProfiling<STATS, STATS_TYPE>::TxFreeStart(void *ptr, size_t size) {
	// empty
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::TxDurationProfiling<STATS, STATS_TYPE>::TxFreeEnd() {
	// empty
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::TxDurationProfiling<STATS, STATS_TYPE>::TxRestartStart() {
	// empty
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::TxDurationProfiling<STATS, STATS_TYPE>::TxRestartEnd() {
	if(ShouldProfile()) {
		uint64_t restart_end_cycles = get_clock_count();
		uint64_t tx_duration = restart_end_cycles - tx_start_cycles;
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_DURATION_RESTARTED, tx_duration);
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_PROFILED_RESTARTS);

		current_profiling_cost += cost_tx_restart_end;
		EndProfilingTx(tx_duration);

#ifdef MEASURE_NON_TX_CODE
		last_committed = false;
#endif /* MEASURE_NON_TX_CODE */
	}	
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::TxDurationProfiling<STATS, STATS_TYPE>::TxAbortStart() {
	// empty
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::TxDurationProfiling<STATS, STATS_TYPE>::TxAbortEnd() {
	if(ShouldProfile()) {
		uint64_t abort_end_cycles = get_clock_count();
		uint64_t tx_duration = abort_end_cycles - tx_start_cycles;
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_DURATION_ABORTED, tx_duration);
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_PROFILED_ABORTS);

		current_profiling_cost += cost_tx_abort_end;
		EndProfilingTx(tx_duration);

#ifdef MEASURE_NON_TX_CODE
		// also measure non-tx code after abort
		last_committed = true;
		tx_commit_cycles = get_clock_count();
#endif /* MEASURE_NON_TX_CODE */
	}
}

#define ESTIMATE_FUN_PROFILING_COST(function_name, result_name)                                      \
	do {                                                                                             \
		uint64_t start_cycles = get_clock_count();                                                   \
																	                                 \
		for(unsigned i = 0;i < PROFILING_COSTS_ESTIMATION_ITERATIONS;i++) {                          \
			function_name();                                                                         \
		}                                                                                            \
																                                     \
		result_name = (get_clock_count() - start_cycles) / PROFILING_COSTS_ESTIMATION_ITERATIONS;    \
	} while(0)

// NOTE: this is maybe a bit of a hack, but never mind
#define REVERT_STAT(stat_name)  stats->IncrementStatisticsPerTxType(stat_name, -stats->GetStatForLexicalTx(stat_name))

template<class STATS, class STATS_TYPE>
inline void wlpdstm::TxDurationProfiling<STATS, STATS_TYPE>::EstimateProfilingCosts() {
	// turn on profiling so something actually happens
	profiling_this_tx = true;
#ifdef MEASURE_NON_TX_CODE
	last_committed = false;
	uint64_t profile_non_tx_code_cost;
#endif /* MEASURE_NON_TX_CODE */

	// estimate costs for all profiling functions
	ESTIMATE_FUN_PROFILING_COST(TxStartStartForEstimatingCosts, cost_tx_start_start);
	ESTIMATE_FUN_PROFILING_COST(TxCommitEnd, cost_tx_commit_end);
#ifdef MEASURE_NON_TX_CODE
	ESTIMATE_FUN_PROFILING_COST(ProfileNonTxCode, profile_non_tx_code_cost);
	cost_tx_commit_end += profile_non_tx_code_cost;
#endif /* MEASURE_NON_TX_CODE */
	ESTIMATE_FUN_PROFILING_COST(TxRestartEnd, cost_tx_restart_end);
	ESTIMATE_FUN_PROFILING_COST(TxAbortEnd, cost_tx_abort_end);

	// update stats
	TxStartStart(-1);
	stats->IncrementStatisticsPerTxType(STATS_TYPE::PROFILING_COST_START, cost_tx_start_start);
	stats->IncrementStatisticsPerTxType(STATS_TYPE::PROFILING_COST_COMMIT, cost_tx_commit_end);
	stats->IncrementStatisticsPerTxType(STATS_TYPE::PROFILING_COST_RESTART, cost_tx_restart_end);
	stats->IncrementStatisticsPerTxType(STATS_TYPE::PROFILING_COST_ABORT, cost_tx_abort_end);

	// revert stats updated during cost estimation
	REVERT_STAT(STATS_TYPE::TX_PROFILED_ALL);
	REVERT_STAT(STATS_TYPE::TX_PROFILED_COMMITS);
	REVERT_STAT(STATS_TYPE::TX_PROFILED_RESTARTS);
	REVERT_STAT(STATS_TYPE::TX_PROFILED_ABORTS);
	REVERT_STAT(STATS_TYPE::TX_DURATION_COMMITTED);
	REVERT_STAT(STATS_TYPE::TX_DURATION_RESTARTED);
	REVERT_STAT(STATS_TYPE::TX_DURATION_ABORTED);
#ifdef MEASURE_NON_TX_CODE
	REVERT_STAT(STATS_TYPE::NON_TX_PROFILED);
	REVERT_STAT(STATS_TYPE::NON_TX_DURATION);
#endif /* MEASURE_NON_TX_CODE */
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::TxDurationProfiling<STATS, STATS_TYPE>::TxStartStartForEstimatingCosts() {
	TxStartStart(PROFILING_COSTS_ESTIMATION_LEXICAL_TX_ID);
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::TxDurationProfiling<STATS, STATS_TYPE>::ThreadStart() {
	// empty
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::TxDurationProfiling<STATS, STATS_TYPE>::ThreadEnd() {
	// empty
}

#endif /* WLPDSTM_PROFILING_ADAPTIVE_H_ */
