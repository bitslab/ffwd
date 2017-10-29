/**
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 */

#ifndef WLPDSTM_STATS_H_
#define WLPDSTM_STATS_H_

#include "memory.h"
#include "../common/stats_base.h"

namespace wlpdstm {
	
	class TlrwStats {
	public:
		enum Type {
			COMMIT = 0,
			COMMIT_READ_ONLY,
			ABORT,
			ABORT_READ_WRITE,
			ABORT_WRITE_READ,
			ABORT_WRITE_WRITE,
			WAIT_ON_ABORT,
			CHECK_FOR_CONCURRENT_READER,
			CHECK_FOR_CONCURRENT_READER_NO_READER,
#ifdef PERFORMANCE_COUNTING
			CYCLES,
			RETIRED_INSTRUCTIONS,
			CACHE_MISSES,
#endif /* PERFORMANCE_COUNTING */
			TX_PROFILED_ALL,
			TX_PROFILED_COMMITS,
			TX_PROFILED_ABORTS,
			TX_PROFILED_READS,
			TX_PROFILED_WRITES,
			TX_PROFILED_ALLOCS,
			TX_PROFILED_FREES,
			TX_DURATION_ALL,
			TX_DURATION_COMMIT_TX,
			TX_DURATION_ABORT_TX,
			TX_DURATION_START,
			TX_DURATION_COMMIT,
			TX_DURATION_ABORT,
			TX_DURATION_READ,
			TX_DURATION_WRITE,
			TX_DURATION_ALLOC,
			TX_DURATION_FREE,
			COUNT
		};
		
		static const char *GetTypeName(Type type) {
			const char *type_names[] = {
				"Commit",
				"CommitReadOnly",
				"Abort",
				"AbortReadWrite",
				"AbortWriteRead",
				"AbortWriteWrite",
				"WaitOnAbort",
				"CheckForConcurrentReader",
				"CheckForConcurrentReaderNoReader",
#ifdef PERFORMANCE_COUNTING
				"Cycles",
				"RetiredInstructions",
				"CacheMisses",
#endif /* PERFORMANCE_COUNTING */
				"TxProfiledAll",
				"TxProfiledCommits",
				"TxProfiledAborts",
				"TxProfiledReads",
				"TxProfiledWrites",
				"TxProfiledAllocs",
				"TxProfiledFrees",
				"TxDurationAll",
				"TxDurationCommitTx",
				"TxDurationAbortTx",
				"TxDurationStart",
				"TxDurationCommit",
				"TxDurationAbort",
				"TxDurationRead",
				"TxDurationWrite",
				"TxDurationAlloc",
				"TxDurationFree"
			};
			
			return type_names[type];
		}

		// average stats
		static const unsigned AVERAGE_STAT_COUNT = 8;
		
		static const AverageStat *GetAverageStats() {
			static const AverageStat average_stats[AVERAGE_STAT_COUNT] = {
				{ "AverageTransactionDuration", TX_DURATION_ALL, TX_PROFILED_ALL},
				{ "AverageWriteDuration", TX_DURATION_WRITE, TX_PROFILED_WRITES},
				{ "AverageReadDuration", TX_DURATION_READ, TX_PROFILED_READS},
				{ "AverageStartDuration", TX_DURATION_START, TX_PROFILED_ALL},
				{ "AverageCommitDuration", TX_DURATION_COMMIT, TX_PROFILED_COMMITS},
				{ "AverageAbortDuration", TX_DURATION_ABORT, TX_PROFILED_ABORTS},
				{ "AverageAllocDuration", TX_DURATION_ALLOC, TX_PROFILED_ALLOCS},
				{ "AverageAllocDuration", TX_DURATION_FREE, TX_PROFILED_FREES},
			};
			
			return average_stats;
		}
	};
	
	typedef TlrwStats StatisticsType;
	typedef StatsBase<StatisticsType, CacheAlignedAlloc, StatisticsType::COUNT> Statistics;
	typedef ThreadStatsBase<StatisticsType, CacheAlignedAlloc, StatisticsType::COUNT> ThreadStatistics;
	typedef ThreadStatsCollectionBase<StatisticsType, CacheAlignedAlloc, StatisticsType::COUNT> ThreadStatisticsCollection;
}

#endif /* WLPDSTM_STATS_H_ */
