/**
 * History based profiling that collects basic information from profiled transactions.
 * It adapts profiling in order to reduce profiling overheads to acceptable amount.
 * This profiling class works correctly with STMs that use dynamic function calls.
 *
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#ifndef WLPDSTM_DYNAMIC_PROFILING_ADAPTIVE_DYNAMIC_H_
#define WLPDSTM_DYNAMIC_PROFILING_ADAPTIVE_DYNAMICH_

//#define PRINT_ESTIMATED_PROFILING_COSTS

#include <stdint.h>

#include "../../common/random.h"
#include "../../common/word.h"
#include "../../common/timing.h"
#include "../../common/print_utils.h"

namespace wlpdstm {

	template<class STATS, class STATS_TYPE>
	class AdaptiveDynamicProfiling {
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

			void TxCommitStart();
			void TxCommitEnd();

			void ReadWordStart(Word *addr);
			void ReadWordStartForEstimatingCosts();
			void ReadWordEnd(Word res);
			void ReadWordEndForEstimatingCosts();

			void WriteWordStart(Word *addr, Word val, Word mask = LOG_ENTRY_UNMASKED);
			void WriteWordStartForEstimatingCosts();
			void WriteWordEnd();

			void TxMallocStart(size_t size);
			void TxMallocStartForEstimatingCosts();
			void TxMallocEnd(void *ptr);
			void TxMallocEndForEstimatingCosts();

			void TxFreeStart(void *ptr, size_t size);
			void TxFreeStartForEstimatingCosts();
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
			uint64_t last_event_cycles;

			// TODO: it seems last event that occurred is needed here too 
			// to correctly handle restarts

            // TODO: think if having this per lexical transaction makes any difference
            uint64_t estimated_tx_duration;
            uint64_t estimated_profiling_cost;
            uint64_t current_profiling_cost;

            uint32_t sampling_period;
            bool profiling_this_tx;
			unsigned consecutive_count;

            // costs of different operations
            uint64_t cost_tx_start_start;
			uint64_t cost_tx_start_end;
				
			uint64_t cost_tx_commit_start;
			uint64_t cost_tx_commit_end;

			uint64_t cost_read_word_start;			
			uint64_t cost_read_word_end;

			uint64_t cost_write_word_start;
			uint64_t cost_write_word_end;

			uint64_t cost_tx_malloc_start;
			uint64_t cost_tx_malloc_end;

			uint64_t cost_tx_free_start;
			uint64_t cost_tx_free_end;

			uint64_t cost_tx_restart_start;
			uint64_t cost_tx_restart_end;

			uint64_t cost_tx_abort_start;
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
//			static const unsigned ACCEPTABLE_OVERHEAD_SHIFT = 32;

			static const unsigned PROFILING_COSTS_ESTIMATION_ITERATIONS = 1000;
			static const unsigned PROFILING_COSTS_ESTIMATION_LEXICAL_TX_ID = 1;

			static const unsigned CONSECUTIVE_TRANSACTION_PROFILE = 16;
	};
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::AdaptiveDynamicProfiling<STATS, STATS_TYPE>::ThreadInit(STATS *stats_) {
	stats = stats_;

	estimated_tx_duration = 0;
	estimated_profiling_cost = 0;
	current_profiling_cost = 0;
	consecutive_count = 0;

	sampling_period = MINIMUM_SAMPLING_PERIOD;

	EstimateProfilingCosts();
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::AdaptiveDynamicProfiling<STATS, STATS_TYPE>::StartProfilingTx() {
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
inline void wlpdstm::AdaptiveDynamicProfiling<STATS, STATS_TYPE>::EndProfilingTx(uint64_t tx_duration) {
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
inline uint64_t wlpdstm::AdaptiveDynamicProfiling<STATS, STATS_TYPE>::CalculateEstimate(uint64_t old_value, uint64_t new_value) {
	return (old_value + new_value) >> 1;
}

// TODO: this seems to cost a lot
template<class STATS, class STATS_TYPE>
inline bool wlpdstm::AdaptiveDynamicProfiling<STATS, STATS_TYPE>::ShouldProfile() const {
	return profiling_this_tx;
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::AdaptiveDynamicProfiling<STATS, STATS_TYPE>::ThreadShutdown() {
#ifdef PRINT_ESTIMATED_PROFILING_COSTS
	printf("Adaptive profiling data:\n");
	printf("========================\n");
	print_indent(stdout, 1);
	printf("estimated_tx_duration: %" PRIu64 "\n", estimated_tx_duration);
	print_indent(stdout, 1);
	printf("estimated_profiling_cost: %" PRIu64 "\n", estimated_profiling_cost);
	print_indent(stdout, 1);
	printf("sampling_period: %u\n", sampling_period);
#endif /* PRINT_ESTIMATED_PROFILING_COSTS */
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::AdaptiveDynamicProfiling<STATS, STATS_TYPE>::TxStartStart(int lex_tx_id) {
	StartProfilingTx();

	if(ShouldProfile()) {
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_PROFILED_ALL);
		tx_start_cycles = get_clock_count();
		current_profiling_cost += cost_tx_start_start;
	}
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::AdaptiveDynamicProfiling<STATS, STATS_TYPE>::TxStartEnd() {
	if(ShouldProfile()) {
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_DURATION_START, get_clock_count() - tx_start_cycles);
		current_profiling_cost += cost_tx_start_end;
	}		
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::AdaptiveDynamicProfiling<STATS, STATS_TYPE>::TxCommitStart() {
	if(ShouldProfile()) {
		last_event_cycles = get_clock_count();
		current_profiling_cost += cost_tx_commit_start;
	}	
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::AdaptiveDynamicProfiling<STATS, STATS_TYPE>::TxCommitEnd() {
	if(ShouldProfile()) {
		uint64_t commit_end_cycles = get_clock_count();
		uint64_t tx_duration = commit_end_cycles - tx_start_cycles;
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_DURATION_COMMITTED, tx_duration);
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_DURATION_COMMIT, commit_end_cycles - last_event_cycles);
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_PROFILED_COMMITS);

		current_profiling_cost += cost_tx_commit_end;
		EndProfilingTx(tx_duration);
	}	
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::AdaptiveDynamicProfiling<STATS, STATS_TYPE>::ReadWordStart(Word *addr) {
	if(ShouldProfile()) {
		last_event_cycles = get_clock_count();
		current_profiling_cost += cost_read_word_start;
	}
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::AdaptiveDynamicProfiling<STATS, STATS_TYPE>::ReadWordEnd(Word res) {
	if(ShouldProfile()) {
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_DURATION_READ, get_clock_count() - last_event_cycles);
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_PROFILED_READS);
		current_profiling_cost += cost_read_word_end;
	}
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::AdaptiveDynamicProfiling<STATS, STATS_TYPE>::WriteWordStart(Word *addr, Word val, Word mask) {
	if(ShouldProfile()) {
		last_event_cycles = get_clock_count();
		current_profiling_cost += cost_write_word_start;
	}
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::AdaptiveDynamicProfiling<STATS, STATS_TYPE>::WriteWordEnd() {
	if(ShouldProfile()) {
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_DURATION_WRITE, get_clock_count() - last_event_cycles);
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_PROFILED_WRITES);
		current_profiling_cost += cost_write_word_end;
	}	
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::AdaptiveDynamicProfiling<STATS, STATS_TYPE>::TxMallocStart(size_t size) {
	if(ShouldProfile()) {
		last_event_cycles = get_clock_count();
		current_profiling_cost += cost_tx_malloc_start;
	}
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::AdaptiveDynamicProfiling<STATS, STATS_TYPE>::TxMallocEnd(void *ptr) {
	if(ShouldProfile()) {
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_DURATION_ALLOC, get_clock_count() - last_event_cycles);
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_PROFILED_ALLOCS);
		current_profiling_cost += cost_tx_malloc_end;
	}	
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::AdaptiveDynamicProfiling<STATS, STATS_TYPE>::TxFreeStart(void *ptr, size_t size) {
	if(ShouldProfile()) {
		last_event_cycles = get_clock_count();
		current_profiling_cost += cost_tx_free_start;
	}
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::AdaptiveDynamicProfiling<STATS, STATS_TYPE>::TxFreeEnd() {
	if(ShouldProfile()) {
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_DURATION_FREE, get_clock_count() - last_event_cycles);
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_PROFILED_FREES);
		current_profiling_cost += cost_tx_free_end;
	}		
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::AdaptiveDynamicProfiling<STATS, STATS_TYPE>::TxRestartStart() {
	if(ShouldProfile()) {
		last_event_cycles = get_clock_count();
		current_profiling_cost += cost_tx_restart_start;
	}
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::AdaptiveDynamicProfiling<STATS, STATS_TYPE>::TxRestartEnd() {
	if(ShouldProfile()) {
		uint64_t restart_end_cycles = get_clock_count();
		uint64_t tx_duration = restart_end_cycles - tx_start_cycles;
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_DURATION_RESTARTED, tx_duration);
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_DURATION_RESTART, restart_end_cycles - last_event_cycles);
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_PROFILED_RESTARTS);

		current_profiling_cost += cost_tx_restart_end;
		EndProfilingTx(tx_duration);
	}	
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::AdaptiveDynamicProfiling<STATS, STATS_TYPE>::TxAbortStart() {
	if(ShouldProfile()) {
		last_event_cycles = get_clock_count();
		current_profiling_cost += cost_tx_abort_start;
	}	
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::AdaptiveDynamicProfiling<STATS, STATS_TYPE>::TxAbortEnd() {
	if(ShouldProfile()) {
		uint64_t abort_end_cycles = get_clock_count();
		uint64_t tx_duration = abort_end_cycles - tx_start_cycles;
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_DURATION_ABORTED, tx_duration);
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_DURATION_ABORT, abort_end_cycles - last_event_cycles);
		stats->IncrementStatisticsPerTxType(STATS_TYPE::TX_PROFILED_ABORTS);

		current_profiling_cost += cost_tx_abort_end;
		EndProfilingTx(tx_duration);
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
inline void wlpdstm::AdaptiveDynamicProfiling<STATS, STATS_TYPE>::EstimateProfilingCosts() {
	// turn on profiling so something actually happens
	profiling_this_tx = true;

	// estimate costs for all profiling functions
	ESTIMATE_FUN_PROFILING_COST(TxStartStartForEstimatingCosts, cost_tx_start_start);
	ESTIMATE_FUN_PROFILING_COST(TxStartEnd, cost_tx_start_end);
	ESTIMATE_FUN_PROFILING_COST(TxCommitStart, cost_tx_commit_start);
	ESTIMATE_FUN_PROFILING_COST(TxCommitEnd, cost_tx_commit_end);
	ESTIMATE_FUN_PROFILING_COST(ReadWordStartForEstimatingCosts, cost_read_word_start);
	ESTIMATE_FUN_PROFILING_COST(ReadWordEndForEstimatingCosts, cost_read_word_end);
	ESTIMATE_FUN_PROFILING_COST(WriteWordStartForEstimatingCosts, cost_write_word_start);
	ESTIMATE_FUN_PROFILING_COST(WriteWordEnd, cost_write_word_end);
	ESTIMATE_FUN_PROFILING_COST(TxMallocStartForEstimatingCosts, cost_tx_malloc_start);
	ESTIMATE_FUN_PROFILING_COST(TxMallocEndForEstimatingCosts, cost_tx_malloc_end);
	ESTIMATE_FUN_PROFILING_COST(TxFreeStartForEstimatingCosts, cost_tx_free_start);
	ESTIMATE_FUN_PROFILING_COST(TxFreeEnd, cost_tx_free_end);
	ESTIMATE_FUN_PROFILING_COST(TxRestartStart, cost_tx_restart_start);
	ESTIMATE_FUN_PROFILING_COST(TxRestartEnd, cost_tx_restart_end);
	ESTIMATE_FUN_PROFILING_COST(TxAbortStart, cost_tx_abort_start);
	ESTIMATE_FUN_PROFILING_COST(TxAbortEnd, cost_tx_abort_end);

	// revert stats updated during cost estimation
	REVERT_STAT(STATS_TYPE::TX_PROFILED_ALL);
	REVERT_STAT(STATS_TYPE::TX_PROFILED_COMMITS);
	REVERT_STAT(STATS_TYPE::TX_PROFILED_RESTARTS);
	REVERT_STAT(STATS_TYPE::TX_PROFILED_ABORTS);
	REVERT_STAT(STATS_TYPE::TX_PROFILED_READS);
	REVERT_STAT(STATS_TYPE::TX_PROFILED_WRITES);
	REVERT_STAT(STATS_TYPE::TX_PROFILED_ALLOCS);
	REVERT_STAT(STATS_TYPE::TX_PROFILED_FREES);
	REVERT_STAT(STATS_TYPE::TX_DURATION_ALL);
	REVERT_STAT(STATS_TYPE::TX_DURATION_COMMITTED);
	REVERT_STAT(STATS_TYPE::TX_DURATION_RESTARTED);
	REVERT_STAT(STATS_TYPE::TX_DURATION_ABORTED);
	REVERT_STAT(STATS_TYPE::TX_DURATION_START);
	REVERT_STAT(STATS_TYPE::TX_DURATION_COMMIT);
	REVERT_STAT(STATS_TYPE::TX_DURATION_RESTART);
	REVERT_STAT(STATS_TYPE::TX_DURATION_ABORT);
	REVERT_STAT(STATS_TYPE::TX_DURATION_READ);
	REVERT_STAT(STATS_TYPE::TX_DURATION_WRITE);
	REVERT_STAT(STATS_TYPE::TX_DURATION_ALLOC);
	REVERT_STAT(STATS_TYPE::TX_DURATION_FREE);

#ifdef PRINT_ESTIMATED_PROFILING_COSTS
	printf("Estimated profiling costs:\n");
	printf("==========================\n");
	print_indent(stdout, 1);
	printf("cost_tx_start_start: %" PRIu64 "\n", cost_tx_start_start);
	print_indent(stdout, 1);
	printf("cost_tx_start_end: %" PRIu64 "\n", cost_tx_start_end);
	print_indent(stdout, 1);
	printf("cost_tx_commit_start: %" PRIu64 "\n", cost_tx_commit_start);
	print_indent(stdout, 1);
	printf("cost_tx_commit_end: %" PRIu64 "\n", cost_tx_commit_end);
	print_indent(stdout, 1);
	printf("cost_read_word_start: %" PRIu64 "\n", cost_read_word_start);
	print_indent(stdout, 1);
	printf("cost_read_word_end: %" PRIu64 "\n", cost_read_word_end);
	print_indent(stdout, 1);
	printf("cost_write_word_start: %" PRIu64 "\n", cost_write_word_start);
	print_indent(stdout, 1);
	printf("cost_write_word_end: %" PRIu64 "\n", cost_write_word_end);
	print_indent(stdout, 1);
	printf("cost_tx_malloc_start: %" PRIu64 "\n", cost_tx_malloc_start);
	print_indent(stdout, 1);
	printf("cost_tx_malloc_end: %" PRIu64 "\n", cost_tx_malloc_end);
	print_indent(stdout, 1);
	printf("cost_tx_free_start: %" PRIu64 "\n", cost_tx_free_start);
	print_indent(stdout, 1);
	printf("cost_tx_free_end: %" PRIu64 "\n", cost_tx_free_end);
	print_indent(stdout, 1);
	printf("cost_tx_restart_start: %" PRIu64 "\n", cost_tx_restart_start);
	print_indent(stdout, 1);
	printf("cost_tx_restart_end: %" PRIu64 "\n", cost_tx_restart_end);
	print_indent(stdout, 1);
	printf("cost_tx_abort_start: %" PRIu64 "\n", cost_tx_abort_start);
	print_indent(stdout, 1);
	printf("cost_tx_abort_end: %" PRIu64 "\n", cost_tx_abort_end);
#endif /* PRINT_ESTIMATED_PROFILING_COSTS */
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::AdaptiveDynamicProfiling<STATS, STATS_TYPE>::TxStartStartForEstimatingCosts() {
	TxStartStart(PROFILING_COSTS_ESTIMATION_LEXICAL_TX_ID);
}

template<class STATS, class STATS_TYPE>				
inline void wlpdstm::AdaptiveDynamicProfiling<STATS, STATS_TYPE>::ReadWordStartForEstimatingCosts() {
	ReadWordStart(NULL);
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::AdaptiveDynamicProfiling<STATS, STATS_TYPE>::ReadWordEndForEstimatingCosts() {
	ReadWordEnd(0);
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::AdaptiveDynamicProfiling<STATS, STATS_TYPE>::WriteWordStartForEstimatingCosts() {
	WriteWordStart(NULL, 0);
}

template<class STATS, class STATS_TYPE>				
inline void wlpdstm::AdaptiveDynamicProfiling<STATS, STATS_TYPE>::TxMallocStartForEstimatingCosts() {
	TxMallocStart(0);
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::AdaptiveDynamicProfiling<STATS, STATS_TYPE>::TxMallocEndForEstimatingCosts() {
	TxMallocEnd(NULL);
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::AdaptiveDynamicProfiling<STATS, STATS_TYPE>::TxFreeStartForEstimatingCosts() {
	TxFreeStart(NULL, 0);
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::AdaptiveDynamicProfiling<STATS, STATS_TYPE>::ThreadStart() {
	// empty
}

template<class STATS, class STATS_TYPE>
inline void wlpdstm::AdaptiveDynamicProfiling<STATS, STATS_TYPE>::ThreadEnd() {
	// empty
}

#endif /* WLPDSTM_DYNAMIC_PROFILING_ADAPTIVE_DYNAMICH_ */
