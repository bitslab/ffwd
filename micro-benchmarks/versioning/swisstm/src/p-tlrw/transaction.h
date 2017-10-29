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

#ifdef WLPDSTM_NIAGARA2
#include <sys/types.h>
#include <sys/processor.h>
#include <sys/procset.h>
#endif /* WLPDSTM_NIAGARA2 */

#include "../common/word.h"
#include "../common/jmp.h"
#include "../common/cache_aligned_alloc.h"
#include "../common/tid.h"
#include "../common/padded.h"
#include "../common/log.h"
#include "../common/random.h"
#include "../common/timing.h"
#include "../common/sampling.h"
#include "../common/spin_loop_backoff.h"

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

//#define THREAD_READING (ReadLock)0xff
#define THREAD_NOT_READING (ReadLock)(0)
#define NO_WRITER ((WriteLock)~0)

//#define MAX_TRANSACTION_COUNT 0xff
#define MAX_TRANSACTION_COUNT 0x20
#define MIN_TRANSACTION_COUNT 1

#define CREATE_WRITE_LOCK(thr_id, tx_cnt)    (((WriteLock)tx_cnt << 8) | (Word)thr_id)
#define WRITE_LOCK_GET_THREAD_ID(write_lock) (write_lock & (WriteLock)0xff)
#define WRITE_LOCK_GET_TX_CNT(write_lock)    (write_lock >> 8)

#define CLEANUP_NO_CAS

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

		//typedef Log<WriteSetEntry> WriteSet;
		//typedef Log<ReadSetEntry> ReadSet;

		class ReadSet : public Log<ReadSetEntry> {
			public:
				void SetThreadId(uint8_t thread_id_) {
					thread_id = thread_id_;
				}

				uint8_t GetThreadId() const {
					return thread_id;
				}

			protected:
				uint8_t thread_id;
		};

		class WriteSet : public Log<WriteSetEntry> {
			public:
				void SetThreadId(uint8_t thread_id_) {
					thread_id = thread_id_;
				}
				
				uint8_t GetThreadId() const {
					return thread_id;
				}
				
			protected:
				uint8_t thread_id;
		};
		
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

		typedef uint8_t TxCounter;

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
#ifdef WLPDSTM_NIAGARA2
			static int GetCpuMap(unsigned thread_id);
			static void BindToCPU(int cpu_map);
#endif /* WLPDSTM_NIAGARA2 */
		
			static void PrintStatistics();

			static void PrintConfiguration(FILE *out_file);
		
			static unsigned map_address_to_index(Word *address);

			void AbortJump();
			void AbortCleanup();

			void RestartJump();
			void RestartCleanup(RestartCause cause);

			void Rollback();

			//static bool CleanupWriter(OwnershipRecord *orec, WriteLock old_value);

#ifdef WAIT_ON_SUCC_ABORTS
			void WaitOnAbort();
#endif /* WAIT_ON_SUCC_ABORTS */

			void LockMemoryStripe(Word *addr, unsigned orec_idx);

			static Word MaskWord(Word old, Word val, Word mask);

			void AddToWriteLog(Word *addr, Word val);
			void RollbackWriteLog();
			void CommitWriteLog();

			void AddToWriteSet(OwnershipRecord *orec);
			void AddToReadSet(OwnershipRecord *orec);

			bool CheckForConcurrentReaders(OwnershipRecord *orec);

			// returns true when STM is shutdown
			//static bool IsSTMStopped();

			void IncrementCurrentTx();
			static TxCounter GetNextTxCounter(TxCounter cnt);
			void GetNextSetPointers();

			// cleanup
			void StartCleanupThread();
			void StopCleanupThread();
			void CleanupThread();
			static void *CleanupThreadStatic(void *tx);
			void EnableThreadCancelation();
			bool IsThreadStopped();

			static void UnlockWriteSet(WriteSet *wr_set, TxCounter cleanup_cnt);
			static void UnlockReadSet(ReadSet *rd_set, TxCounter cleanup_cnt);
		

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

			// shared between app thread and background thread
			CACHE_LINE_ALIGNED union {
				struct {
					volatile TxCounter current_tx;
					volatile TxCounter cleanup_tx;
				};

				char padding_[CACHE_LINE_SIZE_BYTES];
			};

			// shared between app thread and background thread
			WriteSet write_set_array[MAX_TRANSACTION_COUNT + 1];
			ReadSet read_set_array[MAX_TRANSACTION_COUNT + 1];

			// local
			WriteSet *write_set;
			ReadSet *read_set;

			// local
			WriteLog write_log;

			// local
			WriteLock current_write_lock;

			// local
			pthread_t cleanup_thread;

			// local? memory manager
			MemoryManager mm;

			// local data that doesn't need to be aligned:

			// local
			Tid tid;

			// local
			ThreadStatistics stats;

			// local
			Random random;

			TxProfiling profiling;

#ifdef WAIT_ON_SUCC_ABORTS
			// local
			unsigned succ_aborts;
#endif /* WAIT_ON_SUCC_ABORTS */

#ifdef WLPDSTM_NIAGARA2
			int cpu_map;
#endif /* WLPDSTM_NIAGARA2 */

			SpinLoopBackoff spin_loop_backoff;

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

	// one time read and write set init
	for(unsigned i = 0;i < MAX_TRANSACTION_COUNT + 1;i++) {
		read_set_array[i].SetThreadId(tid.Get());
	}

	for(unsigned i = 0;i < MAX_TRANSACTION_COUNT + 1;i++) {
		write_set_array[i].SetThreadId(tid.Get());
	}
	
	// initialize counters
	current_tx = MIN_TRANSACTION_COUNT;
	cleanup_tx = MIN_TRANSACTION_COUNT;
	GetNextSetPointers();

#ifdef WLPDSTM_NIAGARA2
	cpu_map = GetCpuMap(tid.Get());
	BindToCPU(cpu_map);
#endif /* WLPDSTM_NIAGARA2 */
	
	StartCleanupThread();

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

inline void wlpdstm::TransactionTlrw::GetNextSetPointers() {
	write_set = write_set_array + current_tx;
	read_set = read_set_array + current_tx;
}

inline void wlpdstm::TransactionTlrw::TxStart(int lex_tx_id) {
	// initialize lexical tx id
	//stats.lexical_tx_id = lex_tx_id;

	current_write_lock = CREATE_WRITE_LOCK(tid.Get(), current_tx);

	// notify cm of tx start
	cm.TxStart();
	
	// start mm transaction
	mm.TxStart();
}

inline void wlpdstm::TransactionTlrw::TxCommit() {
	// all loads and stores must complete before the next store
	membar_load_store_membar_store_store();

	CommitWriteLog();
	IncrementCurrentTx();

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

	// this is left for the background thread
	IncrementCurrentTx();
	
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
	// TODO: check whether this helps
	if(*read_lock == current_tx) {
		return *addr;
	}

#ifdef CLEANUP_NO_CAS
	// this is needed to avoid races with the cleanup thread
	while(*read_lock != THREAD_NOT_READING) {
		// do nothing
		// TODO: maybe some backoff here
		// TODO: count stats here
	}
#endif /* CLEANUP_NO_CAS */

	// next set myself as the reader
	*read_lock = current_tx;
	AddToReadSet(orec);

	// setting this tx as the reader and reading the write lock must not be reordered
	membar_store_load();

	// read the lock
	WriteLock write_lock = orec->write_lock;

	if(current_write_lock == write_lock) {
		return *addr;
	}

	// TODO: seems there is no need to wait here at all
//	// wait for cleanup of this writer if it is in progress
//	while(write_lock != NO_WRITER && WRITE_LOCK_GET_THREAD_ID(write_lock) == tid.Get()) {
//		write_lock = orec->write_lock;
//		// TODO: some backoff maybe
//		// TODO: collect stats
//	}

	// loop for contention management
	while(true) {
		if(write_lock == NO_WRITER) {
			return *addr;
		}

		uint8_t write_lock_tid = WRITE_LOCK_GET_THREAD_ID(write_lock);

		if(write_lock_tid == tid.Get()) {
			return *addr;
		}

		TransactionTlrw *other_tx = transactions[write_lock_tid];

		// if here, there is a conflict
		if(cm.ShouldRestartReadWrite(&other_tx->cm)) {
			stats.IncrementStatistics(StatisticsType::ABORT_READ_WRITE);
			TxRestart(RESTART_READ_WRITE);
		}

		write_lock = orec->write_lock;
//		spin_loop_backoff.backoff();
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

	// if already written by this transaction return
	if(write_lock == current_write_lock) {
		return;
	}

#ifdef CLEANUP_NO_CAS
//	// wait for cleanup of this writer if it is in progress
//	while(write_lock != NO_WRITER && WRITE_LOCK_GET_THREAD_ID(write_lock) == tid.Get()) {
//		write_lock = orec->write_lock;
//		// TODO: some backoff maybe
//		// TODO: collect stats
//	}
#endif /* CLEANUP_NO_CAS */

	while(true) {
		// if unlocked
//#ifdef CLEANUP_NO_CAS
//		if(write_lock == NO_WRITER) {
//			// try to lock
//			if(atomic_cas_no_barrier(&orec->write_lock, NO_WRITER, current_write_lock)) {				
//#else
		if(write_lock == NO_WRITER || WRITE_LOCK_GET_THREAD_ID(write_lock) == tid.Get()) {
			// try to lock
			if(atomic_cas_no_barrier(&orec->write_lock, write_lock, current_write_lock)) {				
//#endif /* CLEANUP_NO_CAS */
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
//			spin_loop_backoff.backoff();
			continue;			
		}

		// this is a conflict with a running transaction
		TransactionTlrw *other_tx = transactions[WRITE_LOCK_GET_THREAD_ID(write_lock)];

		if(cm.ShouldRestartWriteWrite(&other_tx->cm)) {
			stats.IncrementStatistics(StatisticsType::ABORT_WRITE_WRITE);
			TxRestart(RESTART_WRITE_WRITE);
		}

		write_lock = orec->write_lock;
//		spin_loop_backoff.backoff();
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

			if(read_locks_buffer.read_locks[i] != THREAD_NOT_READING) {
				if(cm.ShouldRestartWriteRead(&transactions[next_read_lock_idx]->cm)) {
					stats.IncrementStatistics(StatisticsType::ABORT_WRITE_READ);
					TxRestart(RESTART_WRITE_READ);
				}
			}

			while(orec->read_locks[next_read_lock_idx] != THREAD_NOT_READING) {
				if(cm.ShouldRestart()) {
					stats.IncrementStatistics(StatisticsType::ABORT_WRITE_READ);
					TxRestart(RESTART_WRITE_READ);					
				}

//				spin_loop_backoff.backoff();
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
	write_set->insert(orec);
}

inline void wlpdstm::TransactionTlrw::AddToReadSet(OwnershipRecord *orec) {
	read_set->insert(orec);
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
	StopCleanupThread();
}

inline void wlpdstm::TransactionTlrw::GlobalShutdown() {
	PrintStatistics();
}

inline void wlpdstm::TransactionTlrw::IncrementCurrentTx() {
	TxCounter next_tx = GetNextTxCounter(current_tx);
	bool first_time = true;

	while(next_tx == cleanup_tx) {
		if(first_time) {
			stats.IncrementStatistics(StatisticsType::NO_READY_RW_SET);
			first_time = false;
		}

		// do nothing
		// TODO: maybe backoff here
		// TODO: increment some stats about exhausted counters
	}

	current_tx = next_tx;
	GetNextSetPointers();
}

inline wlpdstm::TransactionTlrw::TxCounter wlpdstm::TransactionTlrw::GetNextTxCounter(TxCounter cnt) {
	return (cnt == MAX_TRANSACTION_COUNT ? MIN_TRANSACTION_COUNT : cnt + 1);
}

// cleanup
inline void wlpdstm::TransactionTlrw::StartCleanupThread() {
	pthread_create(&cleanup_thread, NULL, TransactionTlrw::CleanupThreadStatic, (void *)this);
}

inline void wlpdstm::TransactionTlrw::StopCleanupThread() {
	pthread_cancel(cleanup_thread);
}

#define CLEANUP_BIND_SAME_CORE

inline void wlpdstm::TransactionTlrw::CleanupThread() {
#ifdef WLPDSTM_NIAGARA2
#ifdef CLEANUP_BIND_SAME_EXE
	BindToCPU(cpu_map + 1);
#elif defined CLEANUP_BIND_SAME_CORE
	BindToCPU(cpu_map | 4);
#elif defined CLEANUP_BIND_DIFF_CORE
	BindToCPU(cpu_map | 32);
#endif /* bind cleanup thread */
#endif /* WLPDSTM_NIAGARA2 */
	EnableThreadCancelation();

	while(true) {
		// wait for something to do
		while(current_tx == cleanup_tx) {
			if(IsThreadStopped()) {
				break;
			}

			spin_loop_backoff.backoff();
		}

		WriteSet *cleanup_write_set = write_set_array + cleanup_tx;
		UnlockWriteSet(cleanup_write_set, cleanup_tx);
		ReadSet *cleanup_read_set = read_set_array + cleanup_tx;
		UnlockReadSet(cleanup_read_set, cleanup_tx);
		membar_store_store();
		cleanup_tx = GetNextTxCounter(cleanup_tx);
	}
}

inline void wlpdstm::TransactionTlrw::UnlockWriteSet(WriteSet *wr_set, TxCounter cleanup_cnt) {
//#ifndef CLEANUP_NO_CAS
	WriteLock write_lock = CREATE_WRITE_LOCK(wr_set->GetThreadId(), cleanup_cnt);
//#endif /* CLEANUP_NO_CAS */
	
	for(WriteSet::iterator curr = wr_set->begin();curr.hasNext();curr.next()) {
		OwnershipRecord *orec = *curr;
//#ifdef CLEANUP_NO_CAS
//		orec->write_lock = NO_WRITER;
//#else
		// no need to check whether cas succeeds
		atomic_cas_no_barrier(&orec->write_lock, write_lock, NO_WRITER);
//#endif /* CLEANUP_NO_CAS */
	}
	
	wr_set->clear();
}

inline void wlpdstm::TransactionTlrw::UnlockReadSet(ReadSet *rd_set, TxCounter cleanup_cnt) {
	uint8_t read_set_thread_id = rd_set->GetThreadId();
	
	for(ReadSet::iterator curr = rd_set->begin();curr.hasNext();curr.next()) {
		OwnershipRecord *orec = *curr;
		volatile ReadLock *read_lock = orec->read_locks + read_set_thread_id;
#ifdef CLEANUP_NO_CAS
		*read_lock = THREAD_NOT_READING;
#else
		volatile Word *read_lock_word = get_word_address((void *)read_lock);
		unsigned read_lock_idx = get_byte_in_word_index((void *)read_lock);
		word_to_bytes old_value, new_value;

		while(true) {
			old_value.word = *read_lock_word;

			if(old_value.bytes[read_lock_idx] != cleanup_cnt) {
				break;
			}

			new_value.word = old_value.word;
			new_value.bytes[read_lock_idx] = THREAD_NOT_READING;
			atomic_cas_no_barrier(read_lock_word, old_value.word, new_value.word);
		}
#endif /* CLEANUP_NO_CAS */
	}
	
	rd_set->clear();	
}

inline bool wlpdstm::TransactionTlrw::IsThreadStopped() {
	pthread_testcancel();
	return false;
}

inline void wlpdstm::TransactionTlrw::EnableThreadCancelation() {
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
}

inline void *wlpdstm::TransactionTlrw::CleanupThreadStatic(void *data) {
	TransactionTlrw *tx = (TransactionTlrw *)data;
	tx->CleanupThread();
	return NULL;
}

#ifdef WLPDSTM_NIAGARA2
inline int wlpdstm::TransactionTlrw::GetCpuMap(unsigned thread_id) {
	unsigned core_id = thread_id & 0x7;
	unsigned subcore_id = (thread_id >> 3) & 0x3;
#ifdef CLEANUP_BIND_SAME_EXE
	return (core_id << 3) | (subcore_id << 1);
#elif defined CLEANUP_BIND_SAME_CORE
	return (core_id << 3) | (subcore_id);
#elif defined CLEANUP_BIND_DIFF_CORE
	return (core_id << 2) | (subcore_id);
#endif /* bind cleanup thread */
}

inline void wlpdstm::TransactionTlrw::BindToCPU(int cpu_map) {
	int res = processor_bind(P_LWPID, P_MYID, cpu_map, NULL) ;

    if(res != 0) {
		printf("processor_bind %d failed with error code %d\n", cpu_map, res);
	}
}
#endif /* WLPDSTM_NIAGARA2 */


#endif /* WLPDSTM_TLRW_TRANSACTION_H_ */
