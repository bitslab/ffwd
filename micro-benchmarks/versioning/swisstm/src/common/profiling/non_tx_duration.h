/**
 * Adaptive profiling of transaction duration. The goal of this profiler is to
 * gather more precise information about transaction durations without overheads of
 * collecting precise information about reads, writes, etc.
 *
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#ifndef WLPDSTM_PROFILING_NON_TX_DURATION_H_
#define WLPDSTM_PROFILING_NON_TX_DURATION_H_

#include <stdint.h>

#include "../random.h"
#include "../word.h"
#include "../timing.h"

namespace wlpdstm {

	template<class STATS, class STATS_TYPE>
	class NonTxDurationProfiling {
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

			void TxStartStart(int lex_tx_id);
			void TxStartStartForEstimatingCosts();
			void TxStartEnd();
			void ProfileNonTxCode();

			void ThreadStart();
			void ThreadEnd();

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

            // TODO: think if having this per lexical transaction makes any difference
            uint64_t estimated_tx_duration;
            uint64_t estimated_profiling_cost;
            uint64_t current_profiling_cost;

            uint32_t sampling_period;
            bool profiling_this_tx;
			unsigned consecutive_count;
			bool last_committed;

            // costs of different operations
            uint64_t cost_tx_start;
			uint64_t cost_tx_commit;
			uint64_t cost_tx_restart;

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
inline void wlpdstm::NonTxDurationProfiling<STATS, STATS_TYPE>::ThreadInit(STATS *stats_) {
	stats = stats_;

	estimated_tx_duration = 0;
	estimated_profiling_cost = 0;
	current_profiling_cost = 0;
	consecutive_count = 0;

	sampling_period = MINIMUM_SAMPLING_PERIOD;

	EstimateProfilingCosts();

	profiling_this_tx = false;
	last_committed = false;
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::NonTxDurationProfiling<STATS, STATS_TYPE>::StartProfilingTx() {
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
inline void wlpdstm::NonTxDurationProfiling<STATS, STATS_TYPE>::EndProfilingTx(uint64_t tx_duration) {
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
inline uint64_t wlpdstm::NonTxDurationProfiling<STATS, STATS_TYPE>::CalculateEstimate(uint64_t old_value, uint64_t new_value) {
	return (old_value + new_value) >> 1;
}

template<class STATS, class STATS_TYPE>
inline bool wlpdstm::NonTxDurationProfiling<STATS, STATS_TYPE>::ShouldProfile() const {
	return profiling_this_tx;
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::NonTxDurationProfiling<STATS, STATS_TYPE>::ThreadShutdown() {
	// empty
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::NonTxDurationProfiling<STATS, STATS_TYPE>::ProfileNonTxCode() {
	if(ShouldProfile() && last_committed) {
		uint64_t tx_duration = get_clock_count() - tx_start_cycles;
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_DURATION_COMMITTED, tx_duration);
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_PROFILED_COMMITS);

		current_profiling_cost += cost_tx_commit;
		EndProfilingTx(tx_duration);
	}
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::NonTxDurationProfiling<STATS, STATS_TYPE>::TxStartStart(int lex_tx_id) {
	ProfileNonTxCode();

	StartProfilingTx();

	if(ShouldProfile()) {
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_PROFILED_ALL);
		last_committed = true;
		current_profiling_cost += cost_tx_start;
		tx_start_cycles = get_clock_count();
	}
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::NonTxDurationProfiling<STATS, STATS_TYPE>::TxStartEnd() {
	// empty
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::NonTxDurationProfiling<STATS, STATS_TYPE>::TxCommitStart() {
	// empty
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::NonTxDurationProfiling<STATS, STATS_TYPE>::TxCommitEnd() {
	// empty
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::NonTxDurationProfiling<STATS, STATS_TYPE>::ReadWordStart(Word *addr) {
	// empty
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::NonTxDurationProfiling<STATS, STATS_TYPE>::ReadWordEnd(Word res) {
	// empty
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::NonTxDurationProfiling<STATS, STATS_TYPE>::WriteWordStart(Word *addr, Word val, Word mask) {
	// empty
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::NonTxDurationProfiling<STATS, STATS_TYPE>::WriteWordEnd() {
	// empty
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::NonTxDurationProfiling<STATS, STATS_TYPE>::TxMallocStart(size_t size) {
	// empty
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::NonTxDurationProfiling<STATS, STATS_TYPE>::TxMallocEnd(void *ptr) {
	// empty
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::NonTxDurationProfiling<STATS, STATS_TYPE>::TxFreeStart(void *ptr, size_t size) {
	// empty
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::NonTxDurationProfiling<STATS, STATS_TYPE>::TxFreeEnd() {
	// empty
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::NonTxDurationProfiling<STATS, STATS_TYPE>::TxRestartStart() {
	// empty
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::NonTxDurationProfiling<STATS, STATS_TYPE>::TxRestartEnd() {
	if(ShouldProfile()) {
		uint64_t restart_end_cycles = get_clock_count();
		uint64_t tx_duration = restart_end_cycles - tx_start_cycles;
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_DURATION_RESTARTED, tx_duration);
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_PROFILED_RESTARTS);

		current_profiling_cost += cost_tx_restart;
		last_committed = false;
		EndProfilingTx(tx_duration);
	}	
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::NonTxDurationProfiling<STATS, STATS_TYPE>::TxAbortStart() {
	// empty
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::NonTxDurationProfiling<STATS, STATS_TYPE>::TxAbortEnd() {
	// empty
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
inline void wlpdstm::NonTxDurationProfiling<STATS, STATS_TYPE>::EstimateProfilingCosts() {
	// turn on profiling so something actually happens
	profiling_this_tx = true;

	// estimate costs for all profiling functions
	ESTIMATE_FUN_PROFILING_COST(TxStartStartForEstimatingCosts, cost_tx_start);
	last_committed = true;
	ESTIMATE_FUN_PROFILING_COST(ProfileNonTxCode, cost_tx_commit);
	ESTIMATE_FUN_PROFILING_COST(TxRestartEnd, cost_tx_restart);

	// update stats
	TxStartStart(-1);
	stats->IncrementStatisticsPerTxType(STATS_TYPE::PROFILING_COST_START, cost_tx_start);
	stats->IncrementStatisticsPerTxType(STATS_TYPE::PROFILING_COST_COMMIT, cost_tx_commit);
	stats->IncrementStatisticsPerTxType(STATS_TYPE::PROFILING_COST_RESTART, cost_tx_restart);

	// revert stats updated during cost estimation
	REVERT_STAT(STATS_TYPE::TX_PROFILED_ALL);
	REVERT_STAT(STATS_TYPE::TX_PROFILED_COMMITS);
	REVERT_STAT(STATS_TYPE::TX_PROFILED_RESTARTS);
	REVERT_STAT(STATS_TYPE::TX_DURATION_COMMITTED);
	REVERT_STAT(STATS_TYPE::TX_DURATION_RESTARTED);
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::NonTxDurationProfiling<STATS, STATS_TYPE>::TxStartStartForEstimatingCosts() {
	last_committed = false;
	TxStartStart(PROFILING_COSTS_ESTIMATION_LEXICAL_TX_ID);
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::NonTxDurationProfiling<STATS, STATS_TYPE>::ThreadStart() {
	// empty
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::NonTxDurationProfiling<STATS, STATS_TYPE>::ThreadEnd() {
	// empty
}

#endif /* WLPDSTM_PROFILING_NON_TX_DURATION_H_ */
