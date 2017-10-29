/**
 * This is a simple implementation of STM using read-write locks.
 *
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#ifndef WLPDSTM_TLRW_TRANSACTION_H_
#define WLPDSTM_TLRW_TRANSACTION_H_

#include <assert.h>
#include <stdio.h>
#include <pthread.h>

#include "../common/word.h"
#include "../common/jmp.h"
#include "../common/cache_aligned_alloc.h"
#include "../common/tid.h"
#include "../common/padded.h"
#include "../common/log.h"
#include "../common/random.h"
#include "../common/timing.h"
#include "../common/sampling.h"

#include "constants.h"
#include "cm.h"
#include "memory.h"
#include "stats.h"

// default is cache line size
// how many successive locations (segment size)
// TODO: seems that epochstm is very very sensitive to this parameter
// it would be interesting to figure this out
// TODO: try increasing it for SwissTM base algorithm
#define LOCK_EXTENT          LOG_CACHE_LINE_SIZE_BYTES
//#define LOCK_EXTENT            (LOG_BYTES_IN_WORD + 2)
#define OWNERSHIP_TABLE_LOG_SIZE 20
#define OWNERSHIP_TABLE_SIZE (1 << OWNERSHIP_TABLE_LOG_SIZE)

#define THREAD_READING (ReadLock)0xff
#define THREAD_NOT_READING (ReadLock)(~THREAD_READING)
#define NO_WRITER NULL

namespace wlpdstm {

	class TransactionTlrw : public CacheAlignedAlloc {
	protected:
		/////////////////
		// types start //
		/////////////////

		typedef Word WriteLock;
		typedef uint8_t ReadLock;

		struct WriteLogEntry {
			Word *log_address;
			Word log_value;
		};

		typedef Log<WriteLogEntry> WriteLog;

#define OWNERSHIP_RECORD_SIZE (CACHE_LINE_SIZE_BYTES)
#define READ_LOCKS_IN_OREC ((OWNERSHIP_RECORD_SIZE - sizeof(WriteLock)) / sizeof(ReadLock))
#define READ_LOCKS_IN_WORD (sizeof(Word) / sizeof(ReadLock))

		// these will be aligned on cache line boundaries
		struct OwnershipRecord {
			volatile WriteLock write_lock;
			volatile ReadLock read_locks[READ_LOCKS_IN_OREC];
		};

		typedef OwnershipRecord *WriteSetEntry;
		typedef OwnershipRecord *ReadSetEntry;

		typedef Log<WriteSetEntry> WriteSet;
		typedef Log<ReadSetEntry> ReadSet;
		
		typedef union {
			ReadLock read_locks[READ_LOCKS_IN_WORD];
			Word word;
		} ReadLocksToWord;

		enum RestartCause {
			NO_RESTART = 0,
			RESTART_EXTERNAL,
			RESTART_READ_WRITE,
			RESTART_WRITE_READ,
			RESTART_WRITE_WRITE
		};

#ifdef WAIT_ON_SUCC_ABORTS
		static const unsigned SUCC_ABORTS_THRESHOLD = 1;
		static const unsigned SUCC_ABORTS_MAX = 10;		
		static const unsigned WAIT_CYCLES_MULTIPLICATOR = 8000;
#endif /* WAIT_ON_SUCC_ABORTS */
		
		///////////////
		// types end //
		///////////////
		
		public:
			static void GlobalInit();

			void ThreadInit();

			void TxStart(int lex_tx_id = NO_LEXICAL_TX);

			void TxCommit();

			void TxAbort();

			void TxRestart(RestartCause cause = RESTART_EXTERNAL);		

			Word ReadWord(Word *addr);

			void WriteWord(Word *address, Word val, Word mask = LOG_ENTRY_UNMASKED);

			void *TxMalloc(size_t size);

			void TxFree(void *ptr, size_t size);

			void LockMemoryBlock(void *address, size_t size);

			void ThreadShutdown();

			static void GlobalShutdown();

		protected:
			static void PrintStatistics();

			static void PrintConfiguration(FILE *out_file);
		
			static unsigned map_address_to_index(Word *address);

			void AbortJump();
			void AbortCleanup();

			void RestartJump();
			void RestartCleanup(RestartCause cause);

			void Rollback();

			static bool CleanupWriter(OwnershipRecord *orec, WriteLock old_value);

#ifdef WAIT_ON_SUCC_ABORTS
			void WaitOnAbort();
#endif /* WAIT_ON_SUCC_ABORTS */

			void LockMemoryStripe(Word *addr, unsigned orec_idx);

			static Word MaskWord(Word old, Word val, Word mask);

			void AddToWriteLog(Word *addr, Word val);
			void RollbackWriteLog();
			void CommitWriteLog();

			void AddToWriteSet(OwnershipRecord *orec);
			void UnlockWriteSet();
			void AddToReadSet(OwnershipRecord *orec);
			void UnlockReadSet();

			bool CheckForConcurrentReaders(OwnershipRecord *orec);

			// returns true when STM is shutdown
			static bool IsSTMStopped();

		/////////////////////////////
		// thread local data start //
		/////////////////////////////

		public:
			// Local, but should be at the start of the descriptor as this is what assembly jump expects.
			CACHE_LINE_ALIGNED union {
				LONG_JMP_BUF start_buf;
				char padding_start_buf[CACHE_LINE_SIZE_BYTES * START_BUF_PADDING_SIZE];
			};

		protected:
			// shared contention manager data
			CACHE_LINE_ALIGNED ContentionManager cm;

			// local? memory manager
			MemoryManager mm;

			// local data that doesn't need to be aligned:

			// local
			Tid tid;

			// local
			ThreadStatistics stats;

			// local
			WriteLog write_log;
			WriteSet write_set;
			ReadSet read_set;

			// local
			Random random;

			TxProfiling profiling;

#ifdef WAIT_ON_SUCC_ABORTS
			// local
			unsigned succ_aborts;
#endif /* WAIT_ON_SUCC_ABORTS */

		///////////////////////////
		// thread local data end //
		///////////////////////////

		///////////////////////
		// shared data start //
		///////////////////////

		CACHE_LINE_ALIGNED static OwnershipRecord orec_table[OWNERSHIP_TABLE_SIZE];

		CACHE_LINE_ALIGNED static TransactionTlrw *transactions[MAX_THREADS];

		// number of application threads running
		CACHE_LINE_ALIGNED static PaddedWord thread_count;

		/////////////////////
		// shared data end //
		/////////////////////
	};

	typedef TransactionTlrw TransactionImpl;
}

inline void wlpdstm::TransactionTlrw::GlobalInit() {
	PrintConfiguration(stdout);

	// initialize global ownership record table
	for(unsigned i = 0;i < OWNERSHIP_TABLE_SIZE;i++) {
		orec_table[i].write_lock = NO_WRITER;

		for(unsigned j = 0;j < READ_LOCKS_IN_OREC;j++) {
			orec_table[i].read_locks[j] = THREAD_NOT_READING;
		}
	}

	// initialize memory manager
	MemoryManager::GlobalInit();

	// initialize contention manager
	ContentionManager::GlobalInit();

	// init total thread count
	thread_count.val = 0;
}

inline void wlpdstm::TransactionTlrw::PrintConfiguration(FILE *out_file) {
	fprintf(out_file, "\nConfiguration:\n");
	fprintf(out_file, "\tLockExtent: %d\n", LOCK_EXTENT);
	fprintf(out_file, "\tOwnershipTableSize: %d\n", OWNERSHIP_TABLE_SIZE);
	fprintf(out_file, "\tMaxThreads: %d\n", MAX_THREADS);
	fprintf(out_file, "\tReadersInOrec: %u\n", (unsigned)READ_LOCKS_IN_OREC);
	fprintf(out_file, "\tSizeOfOrec: %u\n", (unsigned)sizeof(OwnershipRecord));
}

inline void wlpdstm::TransactionTlrw::ThreadInit() {
	// add itself to the transaction array
	transactions[tid.Get()] = this;

	// initialize memory manager
	mm.ThreadInit(tid.Get());

	// initialize contention manager
	cm.ThreadInit(tid.Get());

	// increment count of running threads
	// not safe to use fetch_and_increment on thread_count
	Word my_count = tid.Get() + 1;
	Word curr_count = thread_count.val;
	
	while(my_count > curr_count) {
		if(atomic_cas_no_barrier(&thread_count.val, curr_count, my_count)) {
			break;
		}
		
		curr_count = thread_count.val;
	}

#ifdef WAIT_ON_SUCC_ABORTS
	succ_aborts = 0;
#endif /* WAIT_ON_SUCC_ABORTS */

	profiling.ThreadInit(&random);
}

inline void wlpdstm::TransactionTlrw::TxStart(int lex_tx_id) {
	// initialize lexical tx id
	stats.lexical_tx_id = lex_tx_id;

	// notify cm of tx start
	cm.TxStart();
	
	// start mm transaction
	mm.TxStart();
}

inline void wlpdstm::TransactionTlrw::TxCommit() {
	// all loads and stores must complete before the next store
	membar_load_store_membar_store_store();

	CommitWriteLog();
	UnlockWriteSet();
	UnlockReadSet();

	// notify cm of tx commit
	cm.TxCommit();

	// commit mm transaction
	mm.TxCommit();

	stats.IncrementStatistics(StatisticsType::COMMIT);

#ifdef WAIT_ON_SUCC_ABORTS
	succ_aborts = 0;
#endif /* WAIT_ON_SUCC_ABORTS */
}

inline void wlpdstm::TransactionTlrw::AbortCleanup() {
	Rollback();
	cm.TxAbort();
}

inline void wlpdstm::TransactionTlrw::TxAbort() {
	AbortCleanup();
	AbortJump();
}

inline void wlpdstm::TransactionTlrw::RestartCleanup(RestartCause cause) {
	Rollback();
	cm.TxRestart();
	
#ifdef WAIT_ON_SUCC_ABORTS
	if(cause != RESTART_EXTERNAL) {
		if(++succ_aborts > SUCC_ABORTS_MAX) {
			succ_aborts = SUCC_ABORTS_MAX;
		}
		
		if(succ_aborts >= SUCC_ABORTS_THRESHOLD) {
			WaitOnAbort();
		}
	}
#endif /* WAIT_ON_SUCC_ABORTS */		
}

inline void wlpdstm::TransactionTlrw::TxRestart(RestartCause cause) {
	RestartCleanup(cause);
	RestartJump();	
}

#ifdef WAIT_ON_SUCC_ABORTS
inline void wlpdstm::TransactionTlrw::WaitOnAbort() {
	uint64_t cycles_to_wait = random.Get() % (succ_aborts * WAIT_CYCLES_MULTIPLICATOR);
	wait_cycles(cycles_to_wait);
	stats.IncrementStatistics(StatisticsType::WAIT_ON_ABORT);
}
#endif /* WAIT_ON_SUCC_ABORTS */


inline void wlpdstm::TransactionTlrw::Rollback() {
	// have to rollback write log here
	RollbackWriteLog();

	// all loads and stores must complete before the next store
	membar_load_store_membar_store_store();

	UnlockWriteSet();
	UnlockReadSet();
	
	// commit mm transaction
	mm.TxAbort();

	stats.IncrementStatistics(StatisticsType::ABORT);
}

inline void wlpdstm::TransactionTlrw::AbortJump() {
#ifdef WLPDSTM_ICC
	jmp_to_begin_transaction(&start_buf);
#else	
	siglongjmp(start_buf, LONG_JMP_ABORT_FLAG);
#endif /* WLPDSTM_ICC */
}

inline void wlpdstm::TransactionTlrw::RestartJump() {
#ifdef WLPDSTM_ICC
	jmp_to_begin_transaction(&start_buf);
#else
	siglongjmp(start_buf, LONG_JMP_RESTART_FLAG);
#endif /* WLPDSTM_ICC */
}

inline Word wlpdstm::TransactionTlrw::ReadWord(Word *addr) {
	unsigned orec_idx = map_address_to_index(addr);
	OwnershipRecord *orec = orec_table + orec_idx;
	volatile ReadLock *read_lock = orec->read_locks + tid.Get();

	// if orec is currently read by this tx, just return
	if(*read_lock == THREAD_READING) {
		return *addr;
	}

	// next set myself as the reader
	*read_lock = THREAD_READING;
	AddToReadSet(orec);

	// setting this tx as the reader and reading the write lock must not be reordered
	membar_store_load();

	// read the lock
	WriteLock write_lock = orec->write_lock;
	TransactionTlrw *write_lock_tx = (TransactionTlrw *)write_lock;

	// if locked by this transaction
	if(write_lock_tx == this) {
		return *addr;
	}

	// loop for contention management
	while(true) {
		if(write_lock == (uintptr_t)NO_WRITER) {
			return *addr;
		}

		// if here, there is a conflict
		if(cm.ShouldRestartReadWrite(&write_lock_tx->cm)) {
			stats.IncrementStatistics(StatisticsType::ABORT_READ_WRITE);
			TxRestart(RESTART_READ_WRITE);
		}

		write_lock = orec->write_lock;
		write_lock_tx = (TransactionTlrw *)write_lock;
		continue;
	}

	// this cannot happen
	assert(0);
	return NULL;
}

inline void wlpdstm::TransactionTlrw::WriteWord(Word *addr, Word val, Word mask) {
	unsigned orec_idx = map_address_to_index(addr);

	// LockMemoryStripe will restart if it cannot lock
	LockMemoryStripe(addr, orec_idx);
	AddToWriteLog(addr, *addr);

	// then update the value
	*addr = MaskWord(*addr, val, mask);
}

inline void wlpdstm::TransactionTlrw::LockMemoryStripe(Word *addr, unsigned orec_idx) {
	OwnershipRecord *orec = orec_table + orec_idx;
	WriteLock write_lock = orec->write_lock;
	TransactionTlrw *write_lock_tx = (TransactionTlrw *)write_lock;

	// if already written by this transaction return
	if(write_lock_tx == this) {
		return;
	}

	while(true) {
		// if unlocked
		if(write_lock_tx == NO_WRITER) {
			// try to lock
			if(atomic_cas_no_barrier(&orec->write_lock, NO_WRITER, (Word)this)) {
				AddToWriteSet(orec);

				// if no readers done
				if(CheckForConcurrentReaders(orec)) {
					return;
				}

				// if there are some readers that won contention management
				stats.IncrementStatistics(StatisticsType::ABORT_WRITE_READ);
				TxRestart(RESTART_WRITE_READ);
			}

			write_lock = orec->write_lock;
			write_lock_tx = (TransactionTlrw *)write_lock;
			continue;			
		}

		// this is a conflict with a running transaction
		if(cm.ShouldRestartWriteWrite(&write_lock_tx->cm)) {
			stats.IncrementStatistics(StatisticsType::ABORT_WRITE_WRITE);
			TxRestart(RESTART_WRITE_WRITE);
		}

		write_lock = orec->write_lock;
		write_lock_tx = (TransactionTlrw *)write_lock;
		continue;
	}

	// this shouldn't happen
	assert(0);
}

inline bool wlpdstm::TransactionTlrw::CheckForConcurrentReaders(OwnershipRecord *orec) {
	unsigned next_read_lock_idx = 0;
	unsigned next_word_idx = 0;
	ReadLocksToWord read_locks_buffer;
	Word *read_locks_words = (Word *)orec->read_locks;

	while(next_read_lock_idx < thread_count.val) {
		read_locks_buffer.word = read_locks_words[next_word_idx];

		if(read_locks_buffer.word == (Word)THREAD_NOT_READING) {
			next_word_idx++;
			next_read_lock_idx += READ_LOCKS_IN_WORD;
			continue;
		}

		for(unsigned i = 0;i < READ_LOCKS_IN_WORD;i++,next_read_lock_idx++) {
			if(next_read_lock_idx == tid.Get()) {
				continue;
			}

			if(read_locks_buffer.read_locks[i] == THREAD_READING) {
				if(cm.ShouldRestartWriteRead(&transactions[next_read_lock_idx]->cm)) {
					stats.IncrementStatistics(StatisticsType::ABORT_WRITE_READ);
					TxRestart(RESTART_WRITE_READ);
				}
			}

			while(orec->read_locks[next_read_lock_idx] == THREAD_READING) {
				if(cm.ShouldRestart()) {
					stats.IncrementStatistics(StatisticsType::ABORT_WRITE_READ);
					TxRestart(RESTART_WRITE_READ);					
				}
			}
		}

		next_word_idx++;
	}

	return true;
}

// mask contains ones where value bits are valid
inline Word wlpdstm::TransactionTlrw::MaskWord(Word old, Word val, Word mask) {
	if(mask == LOG_ENTRY_UNMASKED) {
		return val;
	}
	
	return (old & ~mask) | (val & mask);
}

inline void wlpdstm::TransactionTlrw::AddToWriteLog(Word *addr, Word val) {
	WriteLogEntry *log_entry = write_log.get_next();
	log_entry->log_address = addr;
	log_entry->log_value = val;
}

inline void wlpdstm::TransactionTlrw::RollbackWriteLog() {
	for(WriteLog::rev_iterator curr = write_log.rbegin();curr.hasPrev();curr.prev()) {
		WriteLogEntry &log_entry = *curr;
		*log_entry.log_address = log_entry.log_value;
	}

	write_log.clear();
}

inline void wlpdstm::TransactionTlrw::CommitWriteLog() {
	if(write_log.empty()) {
		stats.IncrementStatistics(StatisticsType::COMMIT_READ_ONLY);
	}

	write_log.clear();
}

inline void wlpdstm::TransactionTlrw::AddToWriteSet(OwnershipRecord *orec) {
	write_set.insert(orec);
}

inline void wlpdstm::TransactionTlrw::AddToReadSet(OwnershipRecord *orec) {
	read_set.insert(orec);
}

inline void wlpdstm::TransactionTlrw::UnlockWriteSet() {
	for(WriteSet::iterator curr = write_set.begin();curr.hasNext();curr.next()) {
		OwnershipRecord *orec = *curr;
		orec->write_lock = NO_WRITER;
	}
	
	write_set.clear();
}

inline void wlpdstm::TransactionTlrw::UnlockReadSet() {
	for(ReadSet::iterator curr = read_set.begin();curr.hasNext();curr.next()) {
		OwnershipRecord *orec = *curr;
		orec->read_locks[tid.Get()] = THREAD_NOT_READING;
	}
	
	read_set.clear();	
}

inline void *wlpdstm::TransactionTlrw::TxMalloc(size_t size) {
	return mm.TxMalloc(size);
}

inline void wlpdstm::TransactionTlrw::TxFree(void *ptr, size_t size) {
	LockMemoryBlock(ptr, size);
	mm.TxFree(ptr);
}

inline void wlpdstm::TransactionTlrw::LockMemoryBlock(void *address, size_t size) {
	uintptr_t start = (uintptr_t)address;
	uintptr_t end = start + size;
	int old = -1;
	int curr;
	
	for(uintptr_t address = start;address < end;address++) {
		curr = map_address_to_index((Word *)address);
		
		if(curr != old) {
			LockMemoryStripe((Word *)address, curr);
			old = curr;
		}
	}	
}

// TODO think about moving this out of particular STM implementation
inline void wlpdstm::TransactionTlrw::PrintStatistics() {
#ifdef COLLECT_STATS
	FILE *out_file = stdout;
	fprintf(out_file, "\n");
	fprintf(out_file, "STM internal statistics: \n");
	fprintf(out_file, "========================\n");
	
	// collect stats in a single collection
	ThreadStatisticsCollection stat_collection;
	
	for(unsigned i = 0;i < thread_count.val;i++) {
		// these should all be initialized at this point
		stat_collection.Add(&(transactions[i]->stats));
		fprintf(out_file, "Thread %d: \n", i + 1);
		transactions[i]->stats.Print(out_file, 1);
		fprintf(out_file, "\n");
	}
	
	fprintf(out_file, "Total stats: \n");
	ThreadStatistics total_stats = stat_collection.MergeAll();
	total_stats.Print(out_file, 1);
#endif /* COLLECT_STATS */
}

inline unsigned wlpdstm::TransactionTlrw::map_address_to_index(Word *address) {
	return ((uintptr_t)address >> (LOCK_EXTENT)) & (OWNERSHIP_TABLE_SIZE - 1);
}

inline void wlpdstm::TransactionTlrw::ThreadShutdown() {
	// nothing
}

inline void wlpdstm::TransactionTlrw::GlobalShutdown() {
	PrintStatistics();
}

#endif /* WLPDSTM_TLRW_TRANSACTION_H_ */
