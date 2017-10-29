/**
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#ifndef WLPDSTM_STATS_BASE_H_
#define WLPDSTM_STATS_BASE_H_

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#include "cache_aligned_alloc.h"
#include "constants.h"
#include "print_utils.h"

namespace wlpdstm {

	struct AverageStat {
		const char *name;
		unsigned total_idx;
		unsigned count_idx;
	};

	template<class STATS_TYPES, class ALLOCATOR, unsigned COUNT>
	class StatsBase : public ALLOCATOR {
	public:
		typedef typename STATS_TYPES::Type Type;

	public:
		StatsBase() {
			Reset();
		}
		
		void Reset() {
			memset(&stat_counts, 0, sizeof(stat_counts));
		}
		
		void Increment(Type stat, uint64_t inc = 1) {
			stat_counts[stat] += inc;
		}
		
		void Merge(StatsBase<STATS_TYPES, ALLOCATOR, COUNT> *other) {
			for(unsigned i = 0;i < COUNT;i++) {
				Increment((Type)i, other->Get((Type)i));
			}
		}
		
		uint64_t Get(Type stat) {
			return stat_counts[stat];
		}

	private:
		uint64_t stat_counts[COUNT];
	};
	
	// hold statistics for a single thread and for lexical transactions
	template<class STATS_TYPES, class ALLOCATOR, unsigned COUNT>
	struct ThreadStatsBase {
		typedef typename STATS_TYPES::Type Type;
		typedef StatsBase<STATS_TYPES, ALLOCATOR, COUNT> Statistics;

		static const int MAX_LEXICAL_TX = 64;

		enum {
			TOTAL = 0,
			COMMITTED,
			RESTARTED,
			ABORTED,
			TX_TYPES
		};

		ThreadStatsBase() : next(NULL), lexical_tx_id(NO_LEXICAL_TX) {
			for(int i = 0;i < TX_TYPES;i++) {
				stats[i] = new Statistics();
			}

			for(int i = 0;i < MAX_LEXICAL_TX;i++) {
				for(int j = 0;j < TX_TYPES;j++) {
					lexical_tx_stats[j][i] = new Statistics();
				}

				lexical_tx_stat_used[i] = false;
			}
		}
		
		void Merge(const ThreadStatsBase<STATS_TYPES, ALLOCATOR, COUNT> *tstat) {
			for(int i = 0;i < TX_TYPES;i++) {
				stats[i]->Merge(tstat->stats[i]);
			}

			for(int i = 0;i < MAX_LEXICAL_TX;i++) {
				for(int j = 0;j < TX_TYPES;j++) {
					lexical_tx_stats[j][i]->Merge(tstat->lexical_tx_stats[j][i]);
				}

				lexical_tx_stat_used[i] |= tstat->lexical_tx_stat_used[i];
			}
		}

		inline void IncrementStatistics(Type type, uint64_t inc = 1) {
#ifdef COLLECT_STATS
			IncrementStatisticsInner(type, inc);
#endif /* COLLECT_STATS */
		}

		inline void IncrementStatisticsPerTxType(Type type, uint64_t inc = 1) {
#ifdef COLLECT_STATS
			IncrementStatisticsPerTxTypeInner(type, inc);
#endif /* COLLECT_STATS */
		}

		inline uint64_t GetStatForLexicalTx(Type type) {
			if(lexical_tx_id != -1) {
				return lexical_tx_stats[TOTAL][lexical_tx_id]->Get(type);
			} else {
				return stats[TOTAL]->Get(type);
			}

			return 0;
		}

	protected:
		inline void IncrementStatisticsInner(Type type, uint64_t inc = 1) {
			stats[TOTAL]->Increment(type, inc);
			
			if(lexical_tx_id != -1) {
				lexical_tx_stats[TOTAL][lexical_tx_id]->Increment(type, inc);
				lexical_tx_stat_used[lexical_tx_id] = true;
			}
		}

		inline void IncrementStatisticsPerTxTypeInner(Type type, uint64_t inc = 1) {
#ifdef PER_ABORT_COMMIT_STATS
			current_tx_stats.Increment(type, inc);
#endif /* PER_ABORT_COMMIT_STATS */
			IncrementStatisticsInner(type, inc);
		}

	public:
		inline void TxStart(int lex_tx_id) {
			lexical_tx_id = lex_tx_id;
#ifdef PER_ABORT_COMMIT_STATS
			current_tx_stats.Reset();
#endif /* PER_ABORT_COMMIT_STATS */
		}

		inline void TxCommit() {
#ifdef PER_ABORT_COMMIT_STATS
			stats[COMMITTED]->Merge(&current_tx_stats);

			if(lexical_tx_id != -1) {
				lexical_tx_stats[COMMITTED][lexical_tx_id]->Merge(&current_tx_stats);
				lexical_tx_stat_used[lexical_tx_id] = true;
			}			
#endif /* PER_ABORT_COMMIT_STATS */
		}

		inline void TxRestart() {
#ifdef PER_ABORT_COMMIT_STATS
			stats[RESTARTED]->Merge(&current_tx_stats);
			
			if(lexical_tx_id != -1) {
				lexical_tx_stats[RESTARTED][lexical_tx_id]->Merge(&current_tx_stats);
				lexical_tx_stat_used[lexical_tx_id] = true;
			}						
#endif /* PER_ABORT_COMMIT_STATS */			
		}

		inline void TxAbort() {
#ifdef PER_ABORT_COMMIT_STATS
			stats[ABORTED]->Merge(&current_tx_stats);
			
			if(lexical_tx_id != -1) {
				lexical_tx_stats[ABORTED][lexical_tx_id]->Merge(&current_tx_stats);
				lexical_tx_stat_used[lexical_tx_id] = true;
			}									
#endif /* PER_ABORT_COMMIT_STATS */			
		}

		void Print(FILE *out_file, unsigned indent = 1);
		void PrintStats(Statistics *stats, FILE *out_file, unsigned indent = 1);
		
		Statistics *stats[TX_TYPES];
		Statistics *lexical_tx_stats[TX_TYPES][MAX_LEXICAL_TX];
		bool lexical_tx_stat_used[MAX_LEXICAL_TX];

		// current tx stats, needed for more precise stats
		Statistics current_tx_stats;

		// this is used just for finalization
		ThreadStatsBase<STATS_TYPES, ALLOCATOR, COUNT> *next;

		int lexical_tx_id;
	};
	
	// class for manipulating group of statistics
	template<class STATS_TYPES, class ALLOCATOR, unsigned COUNT>
	class ThreadStatsCollectionBase {
		typedef ThreadStatsBase<STATS_TYPES, ALLOCATOR, COUNT> ThreadStatistics;

		public:
			ThreadStatsCollectionBase() : head(NULL), last(NULL) {
				// empty
			}
			
			void Add(ThreadStatistics *stat) {
				if(last == NULL) {
					head = last = stat;
				} else {
					last->next = stat;
					last = stat;
				}
			}
			
			ThreadStatistics MergeAll() {
				ThreadStatistics ret;
				ThreadStatistics *curr;
				
				for(curr = head;curr != NULL;curr = curr->next) {
					ret.Merge(curr);
				}
				
				return ret;
			}

		private:
			ThreadStatistics *head;
			ThreadStatistics *last;
	};
}

template<class STATS_TYPES, class ALLOCATOR, unsigned COUNT>
inline void wlpdstm::ThreadStatsBase<STATS_TYPES, ALLOCATOR, COUNT>::PrintStats(
		Statistics *s, FILE *out_file, unsigned indent) {
	for(unsigned i = 0;i < COUNT;i++) {
		uint64_t value = s->Get((Type)i);
		
		if(value != 0) {
			print_indent(out_file, indent);
			fprintf(out_file, "%s: %" PRIu64 "\n", STATS_TYPES::GetTypeName((Type)i),
					value);
		}
	}
	
	const AverageStat *average_stats = STATS_TYPES::GetAverageStats();
	
	for(unsigned i = 0;i < STATS_TYPES::AVERAGE_STAT_COUNT;i++) {
		float total = (float)s->Get((Type)average_stats[i].total_idx);
		uint64_t count = s->Get((Type)average_stats[i].count_idx);
		float average = (count == 0) ? 0 : total / count;
		
		if(average != 0) {
			print_indent(out_file, indent);
			fprintf(out_file, "%s: %.2f\n", average_stats[i].name, average);
		}
	}	
}

template<class STATS_TYPES, class ALLOCATOR, unsigned COUNT>
inline void wlpdstm::ThreadStatsBase<STATS_TYPES, ALLOCATOR, COUNT>::Print(FILE *out_file, unsigned indent) {
	PrintStats(stats[TOTAL], out_file, indent);

#ifdef PER_ABORT_COMMIT_STATS
	print_indent(out_file, indent);
	fprintf(out_file, "Committed:\n");
	PrintStats(stats[COMMITTED], out_file, indent + 1);
	print_indent(out_file, indent);
	fprintf(out_file, "Restarted:\n");
	PrintStats(stats[RESTARTED], out_file, indent + 1);	
	print_indent(out_file, indent);
	fprintf(out_file, "Aborted:\n");
	PrintStats(stats[ABORTED], out_file, indent + 1);
#endif /* PER_ABORT_COMMIT_STATS */
	
	for(int ls = 0;ls < MAX_LEXICAL_TX;ls++) {
		if(lexical_tx_stat_used[ls]) {
			print_indent(out_file, indent);
			fprintf(out_file, "Tx %d:\n", ls);
			PrintStats(lexical_tx_stats[TOTAL][ls], out_file, indent + 1);
#ifdef PER_ABORT_COMMIT_STATS
			print_indent(out_file, indent + 1);
			fprintf(out_file, "Committed:\n");
			PrintStats(lexical_tx_stats[COMMITTED][ls], out_file, indent + 2);
			print_indent(out_file, indent + 1);
			fprintf(out_file, "Restarted:\n");
			PrintStats(lexical_tx_stats[RESTARTED][ls], out_file, indent + 2);			
			print_indent(out_file, indent + 1);
			fprintf(out_file, "Aborted:\n");
			PrintStats(lexical_tx_stats[ABORTED][ls], out_file, indent + 2);
#endif /* PER_ABORT_COMMIT_STATS */			
		}
	}
}

#endif /* WLPDSTM_STATS_H_ */
