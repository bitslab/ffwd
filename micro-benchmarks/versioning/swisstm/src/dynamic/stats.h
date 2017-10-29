/**
 *  Collect simple execution statistics.
 *
 *  @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#ifndef WLPDSTM_STATS_H_
#define WLPDSTM_STATS_H_

#include "../common/stats_base.h"
#include "memory.h"

namespace wlpdstm {

	class TxMixinvStats {
	public:
		enum Type {
			THREAD_COUNT = 0,
			COMMIT,
			COMMIT_READ_ONLY,
			ABORT,
			ABORT_WRITE_LOCKED,
			ABORT_WRITE_VALIDATE,
			ABORT_READ_VALIDATE,
			ABORT_READ_LOCKED,
			ABORT_COMMIT_VALIDATE,
			USER_ABORT,
			CLOCK_OVERFLOWS,
#ifdef INFLUENCE_DIAGRAM_STATS
			READ_LOG_SIZE,
			READ_LOG_SIZE_ABORT_READ,
			READ_LOG_SIZE_ABORT_WRITE,
			WRITE_LOG_SIZE,
			WRITE_LOG_SIZE_ABORT_READ,
			WRITE_LOG_SIZE_ABORT_WRITE,
			FALSE_ABORTS_READ,
			FALSE_ABORTS_WRITE,
#endif /* INFLUENCE_DIAGRAM_STATS */
#ifdef TS_EXTEND_STATS
			EXTEND_SUCCESS,
			EXTEND_FAILURE,
#endif /* TS_EXTEND_STATS */
#ifdef DETAILED_STATS
			FREE_DOMINATED,
			UPDATE_MINIMUM_TS,
			SWITCH_TO_SECOND_CM_PHASE,
			CM_DECIDE,
			LARGER_TABLE_WRITE_HITS,
			LARGER_TABLE_READ_HITS,
			WRITES,
			NEW_WRITES,
			READS,
			NEW_READS,
			FALSE_CONFLICTS,
			LOCK_TABLE_RECONFIGURATIONS,
			MEMORY_DEALLOC_COUNT,
			MEMORY_DEALLOC_SIZE,
			MEMORY_DEALLOC_LOCKS,
			WAIT_ON_ABORT,
#endif /* DETAILED_STATS */
#ifdef PERFORMANCE_COUNTING
			CYCLES,
			RETIRED_INSTRUCTIONS,
			CACHE_MISSES,
#endif /* PERFORMANCE_COUNTING */
			TX_PROFILED_ALL,
			TX_PROFILED_COMMITS,
			TX_PROFILED_RESTARTS,
			TX_PROFILED_ABORTS,
			TX_PROFILED_READS,
			TX_PROFILED_WRITES,
			TX_PROFILED_ALLOCS,
			TX_PROFILED_FREES,
			NON_TX_PROFILED,
			TX_DURATION_ALL,
			TX_DURATION_COMMITTED,
			TX_DURATION_RESTARTED,
			TX_DURATION_ABORTED,
			TX_DURATION_START,
			TX_DURATION_COMMIT,
			TX_DURATION_RESTART,
			TX_DURATION_ABORT,
			TX_DURATION_READ,
			TX_DURATION_WRITE,
			TX_DURATION_ALLOC,
			TX_DURATION_FREE,
			NON_TX_DURATION,
			PROFILING_COST_START,
			PROFILING_COST_COMMIT,
			PROFILING_COST_RESTART,
			PROFILING_COST_ABORT,
			PROFILING_COST_NON_TX,
            TX_CACHE_MISSES_COMMITTED,
            TX_CACHE_MISSES_RESTARTED,
            TX_CYCLES_COMMITTED,
            TX_CYCLES_RESTARTED,
#ifdef GREEN_CM
			CONSUMED_ENERGY,
#endif /* GREEN_CM */			
			COUNT
		};

		static const char *GetTypeName(Type type) {
			const char *type_names[] = {
				"ThreadCount",
				"Commit",
				"CommitReadOnly",
				"Abort",
				"AbortWriteLocked",
				"AbortWriteValidate",
				"AbortReadValidate",
				"AbortReadLocked",
				"AbortCommitValidate",
				"UserAbort",
				"ClockOverflows",
#ifdef INFLUENCE_DIAGRAM_STATS
				"ReadLogSize",
				"ReadLogSizeAbortRead",
				"ReadLogSizeAbortWrite",
				"WriteLogSize",
				"WriteLogSizeAbortRead",
				"WriteLogSizeAbortWrite",
				"FalseAbortsRead",
				"FalseAbortsWrite",
#endif /* INFLUENCE_DIAGRAM_STATS */
#ifdef TS_EXTEND_STATS
				"ExtendSuccess",
				"ExtendFailure",
#endif /* TS_EXTEND_STATS */
#ifdef DETAILED_STATS
				"FreeDominated",
				"UpdateMinimumTs",
				"SwitchToSecondCMPhase",
				"CMDecide",
				"LargerTableWriteHits",
				"LargerTableReadHits",
				"Writes",
				"NewWrites",
				"Reads",
				"NewReads",
				"FalseConflicts",
				"LockTableReconfigurations",
				"MemoryDeallocCount",
				"MemoryDeallocSize",
				"MemoryDeallocLocks",
				"WaitOnAbort",
#endif /* DETAILED_STATS */
#ifdef PERFORMANCE_COUNTING
				"Cycles",
				"RetiredInstructions",
				"CacheMisses",
#endif /* PERFORMANCE_COUNTING */
				"TxProfiledAll",
				"TxProfiledCommits",
				"TxProfiledRestarts",
				"TxProfiledAborts",
				"TxProfiledReads",
				"TxProfiledWrites",
				"TxProfiledAllocs",
				"TxProfiledFrees",
				"NonTxProfiled",
				"TxDurationAll",
				"TxDurationCommitted",
				"TxDurationRestarted",
				"TxDurationAborted",
				"TxDurationStart",
				"TxDurationCommit",
				"TxDurationRestart",
				"TxDurationAbort",
				"TxDurationRead",
				"TxDurationWrite",
				"TxDurationAlloc",
				"TxDurationFree",
				"NonTxDuration",
				"ProfilingCostStart",
				"ProfilingCostCommit",
				"ProfilingCostRestart",
				"ProfilingCostAbort"
#ifdef GREEN_CM
				"ConsumedEnergy"
#endif /* GREEN_CM */							
			};

			return type_names[type];
		}

		// average stats
		static const unsigned AVERAGE_STAT_COUNT = 20;
		
		static const AverageStat *GetAverageStats() {
			static const AverageStat average_stats[AVERAGE_STAT_COUNT] = {
				{ "AverageTransactionDuration", TX_DURATION_COMMITTED, TX_PROFILED_COMMITS},
				{ "AverageRestartedTransactionDuration", TX_DURATION_RESTARTED, TX_PROFILED_RESTARTS},
				{ "AverageAbortedTransactionDuration", TX_DURATION_ABORTED, TX_PROFILED_ABORTS},
				{ "AverageWrites", TX_PROFILED_WRITES, TX_PROFILED_ALL},
				{ "AverageWriteDuration", TX_DURATION_WRITE, TX_PROFILED_WRITES},
				{ "AverageReadDuration", TX_DURATION_READ, TX_PROFILED_READS},
				{ "AverageReads", TX_PROFILED_READS, TX_PROFILED_ALL},
				{ "AverageStartDuration", TX_DURATION_START, TX_PROFILED_ALL},
				{ "AverageCommitDuration", TX_DURATION_COMMIT, TX_PROFILED_COMMITS},
				{ "AverageRestartDuration", TX_DURATION_RESTART, TX_PROFILED_RESTARTS},
				{ "AverageAbortDuration", TX_DURATION_ABORT, TX_PROFILED_ABORTS},
				{ "AverageAllocs", TX_PROFILED_ALLOCS, TX_PROFILED_ALL},
				{ "AverageAllocDuration", TX_DURATION_ALLOC, TX_PROFILED_ALLOCS},
				{ "AverageFrees", TX_PROFILED_FREES, TX_PROFILED_ALL},
				{ "AverageFreeDuration", TX_DURATION_FREE, TX_PROFILED_FREES},
				{ "AverageNonTxDuration", NON_TX_DURATION, NON_TX_PROFILED },
				{ "AverageProfilingCostStart", PROFILING_COST_START, THREAD_COUNT },
				{ "AverageProfilingCostCommit", PROFILING_COST_COMMIT, THREAD_COUNT },
				{ "AverageProfilingCostRestart", PROFILING_COST_RESTART, THREAD_COUNT },
				{ "AverageProfilingCostAbort", PROFILING_COST_ABORT, THREAD_COUNT }
			};

			return average_stats;
		}		
	};

	typedef TxMixinvStats StatisticsType;
	typedef StatsBase<TxMixinvStats, CacheAlignedAlloc, TxMixinvStats::COUNT> Statistics;
	typedef ThreadStatsBase<TxMixinvStats, CacheAlignedAlloc, TxMixinvStats::COUNT> ThreadStatistics;
	typedef ThreadStatsCollectionBase<TxMixinvStats, CacheAlignedAlloc, TxMixinvStats::COUNT> ThreadStatisticsCollection;
}

#endif /* WLPDSTM_STATS_H_ */
