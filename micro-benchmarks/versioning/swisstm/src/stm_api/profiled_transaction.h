/**
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#ifndef WLPDSTM_PROFILED_TRANSACTION_H_
#define WLPDSTM_PROFILED_TRANSACTION_H_

#include "../common/timing.h"
#include "../common/sampling.h"
#include "../common/word.h"

namespace wlpdstm {

	template<class TX, class STATS_TYPE>
	class ProfiledTransaction : public TX {
		public:
			void TxStart(int lex_tx_id = NO_LEXICAL_TX);

			void TxCommit();

			Word ReadWord(Word *addr);

			void WriteWord(Word *addr, Word val, Word mask = LOG_ENTRY_UNMASKED);

			void *TxMalloc(size_t size);

			void TxFree(void *ptr, size_t size);

		private:
			uint64_t tx_start_cycles;
	};
}

template<class TX, class STATS_TYPE>
inline void wlpdstm::ProfiledTransaction<TX, STATS_TYPE>::TxStart(int lex_tx_id) {
	TX::stats.lexical_tx_id = lex_tx_id;
	TX::profiling.TxStart();

	if(TX::profiling.IsSampling()) {
		TX::stats.IncrementStatistics(STATS_TYPE::TX_PROFILED_ALL);
		tx_start_cycles = get_clock_count();
	}

	TX::TxStart(lex_tx_id);

	if(TX::profiling.IsSampling()) {
		TX::stats.IncrementStatistics(STATS_TYPE::TX_DURATION_START, get_clock_count() - tx_start_cycles);
	}
}

template<class TX, class STATS_TYPE>
inline void wlpdstm::ProfiledTransaction<TX, STATS_TYPE>::TxCommit() {
	uint64_t commit_start_cycles;
	uint64_t commit_end_cycles;

	if(TX::profiling.IsSampling()) {
		commit_start_cycles = get_clock_count();
	}

	TX::TxCommit();

	if(TX::profiling.IsSampling()) {
		commit_end_cycles = get_clock_count();
		TX::stats.IncrementStatistics(STATS_TYPE::TX_DURATION_ALL, commit_end_cycles - tx_start_cycles);
		TX::stats.IncrementStatistics(STATS_TYPE::TX_DURATION_COMMIT, commit_end_cycles - commit_start_cycles);
		TX::stats.IncrementStatistics(STATS_TYPE::TX_PROFILED_COMMITS);
	}
}

template<class TX, class STATS_TYPE>
inline Word wlpdstm::ProfiledTransaction<TX, STATS_TYPE>::ReadWord(Word *addr) {
	uint64_t read_start_cycles;

	if(TX::profiling.IsSampling()) {
		read_start_cycles = get_clock_count();
	}

	Word ret = TX::ReadWord(addr);

	if(TX::profiling.IsSampling()) {
		TX::stats.IncrementStatistics(STATS_TYPE::TX_DURATION_READ,
									  get_clock_count() - read_start_cycles);
		TX::stats.IncrementStatistics(STATS_TYPE::TX_PROFILED_READS);
	}

	return ret;
}

template<class TX, class STATS_TYPE>
inline void wlpdstm::ProfiledTransaction<TX, STATS_TYPE>::WriteWord(Word *addr, Word val, Word mask) {
	uint64_t write_start_cycles;

	if(TX::profiling.IsSampling()) {
		write_start_cycles = get_clock_count();
	}

	TX::WriteWord(addr, val, mask);

	if(TX::profiling.IsSampling()) {
		TX::stats.IncrementStatistics(STATS_TYPE::TX_DURATION_WRITE,
									  get_clock_count() - write_start_cycles);
		TX::stats.IncrementStatistics(STATS_TYPE::TX_PROFILED_WRITES);
	}
}

template<class TX, class STATS_TYPE>
inline void *wlpdstm::ProfiledTransaction<TX, STATS_TYPE>::TxMalloc(size_t size) {
	if(TX::profiling.IsSampling()) {
		TX::stats.IncrementStatistics(STATS_TYPE::TX_PROFILED_ALLOCS);
	}
	
	return TX::TxMalloc(size);
}

template<class TX, class STATS_TYPE>
inline void wlpdstm::ProfiledTransaction<TX, STATS_TYPE>::TxFree(void *ptr, size_t size) {
	if(TX::profiling.IsSampling()) {
		TX::stats.IncrementStatistics(STATS_TYPE::TX_PROFILED_FREES);
	}

	TX::TxFree(ptr, size);
}

#ifdef SWISSTM
#include "../stm/transaction.h"
#include "../stm/memory.h"
#include "../stm/stats.h"
#elif defined EPOCHSTM
#include "../epochstm/transaction.h"
#include "../epochstm/memory.h"
#include "../epochstm/stats.h"
#elif defined TLRW
#include "../tlrw/transaction.h"
#include "../tlrw/memory.h"
#include "../tlrw/stats.h"
#elif defined RW
#include "../rw/transaction.h"
#include "../rw/memory.h"
#include "../rw/stats.h"
#elif defined P_TLRW
#include "../p-tlrw/transaction.h"
#include "../p-tlrw/memory.h"
#include "../p-tlrw/stats.h"
#elif defined DYNAMIC
#include "../dynamic/transaction.h"
#include "../dynamic/memory.h"
#include "../dynamic/stats.h"
#endif /* SWISSTM */

namespace wlpdstm {
	typedef ProfiledTransaction<TransactionImpl, StatisticsType> Transaction;
}

#endif /* WLPDSTM_PROFILED_TRANSACTION_H_ */
