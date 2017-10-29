/**
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

// TODO: An idea to avoid using tid.Get() all the time. Maybe not needed, but it seems
//       to be interesting to try at least.
//
//       Add tid as a template parameter and then initialize the array of transaction
//       objects with appropriate TID values. In thread initialization (which would now
//       be a static function) when tid is generated the appropriate version of transaction
//       is returned and used subsequently. This would avoid all calculations for epochs index
//       that are based on tid.Get().
//
//       Another possibility is to cache the address of current epoch and use it when needed. ???

#ifndef WLPDSTM_EPOCHSTM_TRANSACTION_H_
#define WLPDSTM_EPOCHSTM_TRANSACTION_H_

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

#define TRANSACTION_COUNT_SIZE_BITS 24
#define MAX_TRANSACTION_COUNT 0xffffff
#define NO_TRANSACTION_COUNT  0
#define MIN_TRANSACTION_COUNT 1
#define NO_THREAD_TID         MAX_THREADS

// txid macros
#define CREATE_TX_ID(thr, cnt)   (((uint32_t)thr << TRANSACTION_COUNT_SIZE_BITS) | (uint32_t)cnt)
#define GET_TX_ID_THREAD(tx_id)  (((uint32_t)tx_id) >> TRANSACTION_COUNT_SIZE_BITS)
#define GET_TX_ID_CNT(tx_id)     ((uint32_t)tx_id & MAX_TRANSACTION_COUNT)
#define NO_THREAD_TX_ID          CREATE_TX_ID(NO_THREAD_TID, NO_TRANSACTION_COUNT)

// this is the size of the cleanup work queue
//#define CLEANUP_WORK_ITEMS  (1 << 8)
//#define CLEANUP_MEMORY_WORK_ITEMS (MAX_THREADS - 1)
//#define DEFAULT_CLEANUP_THREAD_COUNT 2

#define READERS_EXTRA_ARRAY_EMPTY ((Reader *)NULL)

namespace wlpdstm {

	class TransactionEpoch : public CacheAlignedAlloc {
		protected:
			/////////////////
			// types start //
			/////////////////

			// this consists of thread id and transaction count in the thread
			// one byte is reserved for the thread id and three for the transaction id
			// I limit this to 32 in order to limit the size of the reader arrays in the
			// orecs
			typedef uint32_t TxId;

			// the per-thread transaction counter
			typedef Word TxCounter;
			
			union PaddedTxCounter {
				volatile TxCounter tx_counter;
				char padding[CACHE_LINE_SIZE_BYTES];
			};
			
			typedef Word WriteLock;

			struct WriteLogEntry {
				Word *log_address;
				Word log_value;
			};

			typedef Log<WriteLogEntry> WriteLog;

			// reader should be 4 bytes to limit the size of the orec tabel and
			// all associated reader arrays
			typedef TxId Reader;

#define OWNERSHIP_RECORD_SIZE (CACHE_LINE_SIZE_BYTES)
#define READERS_EXTRA_ARRAYS 2
#ifdef USE_MINIMUM_READER_VERSION
#define READERS_IN_OREC ((OWNERSHIP_RECORD_SIZE - sizeof(WriteLock) - sizeof(Word) \
		- READERS_EXTRA_ARRAYS * sizeof(Reader *) - sizeof(TxCounter)) / sizeof(Reader))
#else
#define READERS_IN_OREC ((OWNERSHIP_RECORD_SIZE - sizeof(WriteLock) - sizeof(Word) \
		- READERS_EXTRA_ARRAYS * sizeof(Reader *)) / sizeof(Reader))
#endif /* USE_MINIMUM_READER_VERSION */
#define READERS_EXTRA_ARRAY_SIZE (2 * CACHE_LINE_SIZE_BYTES)
#define READERS_IN_EXTRA_ARRAY (READERS_EXTRA_ARRAY_SIZE / sizeof(Reader))
#define READERS_IN_WORD (sizeof(Word) / sizeof(Reader))

			// these will be aligned on cache line boundaries
			struct OwnershipRecord {
				volatile WriteLock lock;
				volatile Word reader_count;
#ifdef USE_MINIMUM_READER_VERSION
				volatile TxCounter max_orec_rver;
#endif /* USE_MINIMUM_READER_VERSION */

				union {
					Reader readers[READERS_IN_OREC];
					Word reader_epochs_word[READERS_IN_OREC / READERS_IN_WORD];
				};

				Reader *readers_extra_arrays[READERS_EXTRA_ARRAYS];
			};
			
			struct ReaderExtraArray : public CacheAlignedAlloc {
				union {
					Reader readers[READERS_IN_EXTRA_ARRAY];
					Word readers_word[READERS_IN_EXTRA_ARRAY / READERS_IN_WORD];
				};
			};

			typedef union {
				Reader readers[READERS_IN_WORD];
				Word word;
			} ReaderToWord;

			typedef struct {
				OwnershipRecord *start_orec;
#ifdef USE_TOUCHED_ORECS
				unsigned start_touched_orec_idx;
				unsigned end_touched_orec_idx; // one past the end
#else
				OwnershipRecord *end_orec; // one past the end
#endif /* USE_TOUCHED_ORECS */
			} CleanupWorkItem;

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
			void AbortCleanup();

			void RestartCleanup(RestartCause cause);		
		
			static void PrintStatistics();

			static void PrintConfiguration(FILE *out_file);
		
			static unsigned map_address_to_index(Word *address);
#ifdef USE_TOUCHED_ORECS
			static unsigned map_orec_idx_to_touched_orec_idx(unsigned orec_idx);
#endif /* USE_TOUCHED_ORECS */

			void AbortJump();

			void RestartJump();

			void Rollback();

			static bool CleanupWriter(OwnershipRecord *orec, WriteLock old_value);

#ifdef WAIT_ON_SUCC_ABORTS
			void WaitOnAbort();
#endif /* WAIT_ON_SUCC_ABORTS */

//			static bool CleanupWord(Word *word, int reader_count);
//			template<unsigned ARRAY_SIZE>
//			static void CleanupReadersArray(Reader *array, int &reader_count);
//			static void CleanupReaders(OwnershipRecord *orec);

//			void GetNextEpoch();

			void IncrementCounter();
#ifdef USE_MINIMUM_READER_VERSION
			void CalculateMinimumCounter();
#endif /* USE_MINIMUM_READER_VERSION */
			void InitializeCurrentWriteLock(uint32_t counter);
			bool IsNotCurrentTx(uint32_t thread, uint32_t cnt);

			void LockMemoryStripe(Word *addr, unsigned orec_idx);

			static Word MaskWord(Word old, Word val, Word mask);

			void AddToWriteLog(Word *addr, Word val);
			void RollbackWriteLog();
			void CommitWriteLog();

			Reader *GetReader(unsigned orc_idx);
			static Reader *CreateReaderArray();
			static void FreeReaderArray(Reader *first_in_array);

			bool CheckForConcurrentReaders(OwnershipRecord *orec);
			bool CheckReader(Reader reader);

			// returns true when STM is shutdown
			static bool IsSTMStopped();

//			// gc related:
//			static void CleanupOrec(OwnershipRecord *orec);
//
//			// remember the state of epochs before starting the cleanup
//			static void MakeEpochSnapshot();
//
//			// update epoch state after cleanup
//			static void UpdateEpochFromSnapshot();
//
//			static void InitializeCleanupQueue();
//
//			static bool CleanupLoopIter();
//#ifdef MM_EPOCH
//			static bool CleanupMemoryLoopIter();
//#endif /* MM_EPOCH */

#ifdef USE_TOUCHED_ORECS
			static void MakeTouchedOrecsSnapshot();
			static void MergeTouchedOrecsArrays(uint8_t *result, unsigned threads);
			static unsigned GetTouchedOrecsCount();
			static void PrintTouchedOrecsStats(FILE *out_file);
#endif /* USE_TOUCHED_ORECS */

//			static void *MainCleanupThread(void *nothing);
//
//			static void *WorkerCleanupThread(void *nothing);
//
//			static void StartupCleanupThreads(unsigned thread_count);
//
//			static void EnableThreadCancelation();

			// gather and print statistics about orec table
#define STATS_ARRAY_COLS 2
#define READ_STATS_ARRAY_ROWS (MAX_THREADS + 1)
#define WRITE_STATS_ARRAY_ROWS 16

			template<unsigned ROWS>
			struct OrecTableStatsArray {
				void Init();
				void Sort();
				void Print(const char *msg, FILE *out_file);

				unsigned stats[ROWS][STATS_ARRAY_COLS];
			};

			typedef OrecTableStatsArray<READ_STATS_ARRAY_ROWS> OrecTableReadStats;

			static void PrintOwnershipTableStats(FILE* out_file);
			static void GatherOwnershipTableStats(OrecTableReadStats &read_stats);
		
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

#ifdef TLCNT
			// these are the latest values of thread counters
			// observed by this thread
			TxCounter observed_counters[MAX_THREADS];
#endif /* TLCNT */

			// cache the lock value at the start of a transaction
			Word current_cnt;
			TxId current_tx_id;
			WriteLock current_lock;

			// local
			Tid tid;

			// local
		protected:
			ThreadStatistics stats;

			// this is pretty large
			// TODO: think about replacing it with a hashtable maybe?
			Reader *reader_mapping[OWNERSHIP_TABLE_SIZE];

			// local
			WriteLog write_log;

			// local
			Random random;

			// profiling
			TxProfiling profiling;
//PROF			uint64_t tx_start_cycles;

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

		// aligned thread counters
		CACHE_LINE_ALIGNED static PaddedTxCounter counters[MAX_THREADS];

#ifdef USE_MINIMUM_READER_VERSION
		CACHE_LINE_ALIGNED static PaddedTxCounter min_counter;
#endif /* USE_MINIMUM_READER_VERSION */

#ifdef USE_TOUCHED_ORECS
		CACHE_LINE_ALIGNED static volatile uint8_t touched_orecs[MAX_THREADS][TOUCHED_ORECS_ARRAY_SIZE];
#endif /* USE_TOUCHED_ORECS */
		
		CACHE_LINE_ALIGNED static TransactionEpoch *transactions[MAX_THREADS];

		// number of application threads running
		CACHE_LINE_ALIGNED static PaddedWord thread_count;

//		CACHE_LINE_ALIGNED static CleanupWorkItem cleanup_queue[CLEANUP_WORK_ITEMS];
//
//		CACHE_LINE_ALIGNED static PaddedWord cleanup_counter;
//
//		CACHE_LINE_ALIGNED static PaddedWord cleanup_done_counter;
//
//#ifdef MM_EPOCH
//		CACHE_LINE_ALIGNED static PaddedWord cleanup_memory_counter;
//		
//		CACHE_LINE_ALIGNED static PaddedWord cleanup_memory_done_counter;
//#endif /* MM_EPOCH */
//
//		// this is the number of active threads when cleanup snapshot was taken
//		static Word cleanup_snapshot_thread_count;
//
//		// cleanup threads
//		static pthread_t cleanup_threads[MAX_CLEANUP_THREADS];
//
//		// number of background threads
//		static unsigned cleanup_thread_count;
//
//#ifdef USE_TOUCHED_ORECS
//		CACHE_LINE_ALIGNED static uint8_t cleanup_touched_orecs[TOUCHED_ORECS_ARRAY_SIZE];
//#endif /* USE_TOUCHED_ORECS */

		/////////////////////
		// shared data end //
		/////////////////////
	};

	typedef TransactionEpoch TransactionImpl;
}

inline void wlpdstm::TransactionEpoch::GlobalInit() {
	PrintConfiguration(stdout);

	// initialize global ownership record table
	for(unsigned i = 0;i < OWNERSHIP_TABLE_SIZE;i++) {
		orec_table[i].lock = NO_THREAD_TX_ID;

		orec_table[i].reader_count = 0;
#ifdef USE_MINIMUM_READER_VERSION
		orec_table[i].max_orec_rver = NO_TRANSACTION_COUNT;
#endif /* USE_MINIMUM_READER_VERSION */

		for(unsigned j = 0;j < READERS_IN_OREC;j++) {
			orec_table[i].readers[j] = NO_THREAD_TX_ID;
		}

		for(unsigned j = 0;j < READERS_EXTRA_ARRAYS;j++) {
			orec_table[i].readers_extra_arrays[j] = READERS_EXTRA_ARRAY_EMPTY;
		}
	}

	// initialize counters
	for(unsigned i = 0;i < MAX_THREADS;i++) {
		counters[i].tx_counter = NO_TRANSACTION_COUNT;
	}

#ifdef USE_TOUCHED_ORECS
	for(unsigned i = 0;i < MAX_THREADS;i++) {
		// initialize touched orecs
		for(unsigned j = 0;j < TOUCHED_ORECS_ARRAY_SIZE;j++) {
			touched_orecs[i][j] = TOUCHED_ORECS_NO;
		}
	}
#endif /* USE_TOUCHED_ORECS */

	// initialize memory manager
	MemoryManager::GlobalInit();

	// initialize contention manager
	ContentionManager::GlobalInit();

	// init total thread count
	thread_count.val = 0;

	// initialize gc
//	InitializeCleanupQueue();
//	cleanup_counter.val = CLEANUP_WORK_ITEMS;
//	cleanup_done_counter.val = 0;
//#ifdef MM_EPOCH
//	cleanup_memory_counter.val = CLEANUP_MEMORY_WORK_ITEMS;
//	cleanup_memory_done_counter.val = 0;
//#endif /* MM_EPOCH */
//	StartupCleanupThreads(DEFAULT_CLEANUP_THREAD_COUNT);
}

inline void wlpdstm::TransactionEpoch::PrintConfiguration(FILE *out_file) {
	fprintf(out_file, "\nConfiguration:\n");
	fprintf(out_file, "\tLockExtent: %d\n", LOCK_EXTENT);
	fprintf(out_file, "\tOwnershipTableSize: %d\n", OWNERSHIP_TABLE_SIZE);
	fprintf(out_file, "\tMaxThreads: %d\n", MAX_THREADS);
//	fprintf(out_file, "\tMaxCleanupThreads: %d\n", MAX_CLEANUP_THREADS);
//	fprintf(out_file, "\tEpochSize: %u\n", (unsigned)sizeof(Epoch));
	fprintf(out_file, "\tMaxTransactionId: %d\n", MAX_TRANSACTION_COUNT);
//	fprintf(out_file, "\tCleanupThreads: %d\n", DEFAULT_CLEANUP_THREAD_COUNT);
//	fprintf(out_file, "\tCleanupWorkItems: %d\n", CLEANUP_WORK_ITEMS);
//	fprintf(out_file, "\tCleanupMemoryWorkItems: %d\n", CLEANUP_MEMORY_WORK_ITEMS);
	fprintf(out_file, "\tReadersInOrec: %u\n", (unsigned)READERS_IN_OREC);
	fprintf(out_file, "\tReadersInExtraArray: %u\n", (unsigned)READERS_IN_EXTRA_ARRAY);
	fprintf(out_file, "\tSizeOfOrec: %u\n", (unsigned)sizeof(OwnershipRecord));
	fprintf(out_file, "\tNoThreadTxId: 0x%x\n", NO_THREAD_TX_ID);
#ifdef USE_TOUCHED_ORECS
	fprintf(out_file, "\tTouchedOrecsArraySize: %u\n", TOUCHED_ORECS_ARRAY_SIZE);
#endif /* USE_TOUCHED_ORECS */
}

inline void wlpdstm::TransactionEpoch::ThreadInit() {
	// add itself to the transaction array
	transactions[tid.Get()] = this;

#ifdef TLCNT
	// intialize current epoch
	for(unsigned i = 0;i < MAX_THREADS;i++) {
		observed_counters[i] = NO_TRANSACTION_COUNT;
	}
#endif /* TLCNT */

	IncrementCounter();

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

	// initialize reader epoch mapping
	for(unsigned i = 0;i < OWNERSHIP_TABLE_SIZE;i++) {
		reader_mapping[i] = NULL;
	}

	profiling.ThreadInit(&random);

#ifdef WAIT_ON_SUCC_ABORTS
	succ_aborts = 0;
#endif /* WAIT_ON_SUCC_ABORTS */
}

inline void wlpdstm::TransactionEpoch::TxStart(int lex_tx_id) {
	// initialize lexical tx id
//PROF	stats.lexical_tx_id = lex_tx_id;
//
//	profiling.TxStart();
//
//	if(profiling.IsSampling()) {
//		stats.IncrementStatistics(StatisticsType::TX_PROFILED_ALL);
//		tx_start_cycles = get_clock_count();
//	}

	// notify cm of tx start
	cm.TxStart();
	
	// start mm transaction
	mm.TxStart();

//	GetNextEpoch();

	// initialize the current lock
//	current_lock = CREATE_WRITE_LOCK(tid.Get(), current_epoch);

	// start the transaction
//	epochs[tid.Get()][current_epoch] = CURRENT_EPOCH;

	// the previous write must finish before any further reads and writes
//	membar_store_load_membar_store_store();
}

//inline void wlpdstm::TransactionEpoch::GetNextEpoch() {
//	Epoch start_epoch = current_epoch;
//
//	while(true) {
//		do {
//			if(current_epoch == MAX_EPOCH) {
//				current_epoch = MIN_EPOCH;
//			} else {
//				current_epoch += 1;
//			}
//
//			if(epochs[tid.Get()][current_epoch] == UNUSED_EPOCH) {
//				return;
//			}
//		} while (current_epoch != start_epoch);
//
//#ifdef APP_THREAD_CLEANUP
//		while(CleanupLoopIter()) {
//			stats.IncrementStatistics(StatisticsType::APP_THREAD_CLEANUP_HELP);
//		}
//#endif /* APP_THREAD_CLEANUP */
//			
//		stats.IncrementStatistics(StatisticsType::NO_FREE_NEXT_EPOCH);
//	}
//}

inline void wlpdstm::TransactionEpoch::IncrementCounter() {
	uint32_t current_cnt = ++counters[tid.Get()].tx_counter;

	// TODO - this needs to take cleanup in account
	if(current_cnt == MAX_TRANSACTION_COUNT) {
		printf("Exhausted the transaction id space. Exiting...\n");
		exit(1);
	}

	InitializeCurrentWriteLock(current_cnt);
#ifdef USE_MINIMUM_READER_VERSION
	CalculateMinimumCounter();
#endif /* USE_MINIMUM_READER_VERSION */
}

inline void wlpdstm::TransactionEpoch::InitializeCurrentWriteLock(uint32_t cnt) {
	current_cnt = cnt;
	current_tx_id = CREATE_TX_ID(tid.Get(), cnt);
	current_lock = current_tx_id;
}

inline bool wlpdstm::TransactionEpoch::IsNotCurrentTx(uint32_t thread, uint32_t cnt) {
#ifdef TLCNT
	// first try to read from last observed state
	if(cnt < observed_counters[thread]) {
		return true;
	}

	// update observed state from shared state
	observed_counters[thread] = counters[thread].tx_counter;
	return cnt < observed_counters[thread];
#else
	return cnt != counters[thread].tx_counter;
#endif /* TLCNT */
}

inline void wlpdstm::TransactionEpoch::TxCommit() {
	// all loads and stores must complete before the next store
	membar_load_store_membar_store_store();

	CommitWriteLog();

	// mark transaction committed
	IncrementCounter();

	// notify cm of tx commit
	cm.TxCommit();

	// commit mm transaction
	mm.TxCommit();

	stats.IncrementStatistics(StatisticsType::COMMIT);

#ifdef WAIT_ON_SUCC_ABORTS
	succ_aborts = 0;
#endif /* WAIT_ON_SUCC_ABORTS */
}

inline void wlpdstm::TransactionEpoch::AbortCleanup() {
	Rollback();
	cm.TxAbort();
}

inline void wlpdstm::TransactionEpoch::TxAbort() {
	AbortCleanup();
	AbortJump();
}

inline void wlpdstm::TransactionEpoch::RestartCleanup(RestartCause cause) {
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

inline void wlpdstm::TransactionEpoch::TxRestart(RestartCause cause) {
	RestartCleanup(cause);
	RestartJump();	
}

#ifdef WAIT_ON_SUCC_ABORTS
inline void wlpdstm::TransactionEpoch::WaitOnAbort() {
	uint64_t cycles_to_wait = random.Get() % (succ_aborts * WAIT_CYCLES_MULTIPLICATOR);
	wait_cycles(cycles_to_wait);
	stats.IncrementStatistics(StatisticsType::WAIT_ON_ABORT);
}
#endif /* WAIT_ON_SUCC_ABORTS */


inline void wlpdstm::TransactionEpoch::Rollback() {
	// have to rollback write log here
	RollbackWriteLog();

	// all loads and stores must complete before the next store
	membar_load_store_membar_store_store();
	
	// mark transaction aborted
	IncrementCounter();

	// commit mm transaction
	mm.TxAbort();

	stats.IncrementStatistics(StatisticsType::ABORT);
}

inline void wlpdstm::TransactionEpoch::AbortJump() {
#ifdef WLPDSTM_ICC
	jmp_to_begin_transaction(&start_buf);
#else	
	siglongjmp(start_buf, LONG_JMP_ABORT_FLAG);
#endif /* WLPDSTM_ICC */
}

inline void wlpdstm::TransactionEpoch::RestartJump() {
#ifdef WLPDSTM_ICC
	jmp_to_begin_transaction(&start_buf);
#else
	siglongjmp(start_buf, LONG_JMP_RESTART_FLAG);
#endif /* WLPDSTM_ICC */
}

inline wlpdstm::TransactionEpoch::Reader *wlpdstm::TransactionEpoch::GetReader(unsigned orec_idx) {
	// this is common and fast path
	Reader *reader = reader_mapping[orec_idx];

	if(reader != NULL) {
		return reader;
	}

	// if never read the orec before, need to acquire it
	OwnershipRecord *orec = orec_table + orec_idx;
	Word reader_idx = fetch_and_inc_no_barrier(&orec->reader_count);

	// use inlined reader_epoch if possible
	if(reader_idx < READERS_IN_OREC) {
		reader = (Reader *)&orec->readers[reader_idx];
	} else {
		// if not get it from extra array
		reader_idx -= READERS_IN_OREC;
		unsigned reader_array_idx = reader_idx / READERS_IN_EXTRA_ARRAY;
		reader_idx %= READERS_IN_EXTRA_ARRAY;
		Reader *array = (Reader *)atomic_load_no_barrier(
			&orec->readers_extra_arrays[reader_array_idx]);

		// if needed allocate extra array
		if(array == READERS_EXTRA_ARRAY_EMPTY) {
			array = CreateReaderArray();

			// finish initialization of epoch array before cas
			membar_store_store();

			if(!atomic_cas_no_barrier(&orec->readers_extra_arrays[reader_array_idx],
									  READERS_EXTRA_ARRAY_EMPTY, array)) {
				FreeReaderArray(array);
				array = orec->readers_extra_arrays[reader_array_idx];
			}
		}

		reader = &array[reader_idx];
	}

	// initialize epoch, initialize mapping and return
	reader_mapping[orec_idx] = reader;

#ifdef USE_TOUCHED_ORECS
	// annonuce touching the orec 
	unsigned touched_orecs_idx = map_orec_idx_to_touched_orec_idx(orec_idx);

	if(touched_orecs[tid.Get()][touched_orecs_idx] == TOUCHED_ORECS_NO) {
		touched_orecs[tid.Get()][touched_orecs_idx] = TOUCHED_ORECS_YES;
	}
#endif /* USE_TOUCHED_ORECS */

	return reader;
}

inline wlpdstm::TransactionEpoch::Reader *wlpdstm::TransactionEpoch::CreateReaderArray() {
	ReaderExtraArray *extra_array = new ReaderExtraArray();
	Reader *array = extra_array->readers;
	
	for(unsigned i = 0;i < READERS_IN_EXTRA_ARRAY;i++) {
		array[i] = NO_THREAD_TX_ID;
	}

	return array;
}

inline void wlpdstm::TransactionEpoch::FreeReaderArray(Reader *first_in_array) {
	ReaderExtraArray *extra_array = (ReaderExtraArray *)first_in_array;
	delete(extra_array);
}

inline Word wlpdstm::TransactionEpoch::ReadWord(Word *addr) {
//PROF	if(profiling.IsSampling()) {
//		stats.IncrementStatistics(StatisticsType::TX_PROFILED_READS);
//	}

	unsigned orec_idx = map_address_to_index(addr);
	Reader *reader = GetReader(orec_idx);

	// if orec is currently read by this tx, just return
	if(*reader == current_tx_id) {
		return *addr;
	}

	OwnershipRecord *orec = orec_table + orec_idx;

	// next set myself as the reader
	*reader = current_tx_id;

#ifdef USE_MINIMUM_READER_VERSION
	Word old_max_cnt = orec->max_orec_rver;

	while(old_max_cnt < current_cnt) {
		if(atomic_cas_no_barrier(&orec->max_orec_rver, old_max_cnt, current_cnt)) {
			break;
		}

		old_max_cnt = orec->max_orec_rver;
	}
#endif /* USE_MINIMUM_READER_VERSION */

	// setting this tx as the reader and reading the write lock must not be reordered
	membar_store_load();

	// read the lock
	WriteLock write_lock = orec->lock;

	// if locked by this transaction
//	uint8_t write_lock_thr = GET_TX_ID_THREAD(write_lock);
//
//	if(write_lock_thr == tid.Get()) {
	if(write_lock == current_lock) {
		return *addr;
	}

	// loop for contention management
	while(true) {
		if(write_lock == NO_THREAD_TX_ID) {
			return *addr;
		}

		uint8_t write_lock_thr = GET_TX_ID_THREAD(write_lock);
		TxCounter write_lock_cnt = GET_TX_ID_CNT(write_lock);

		if(IsNotCurrentTx(write_lock_thr, write_lock_cnt)) {
			//CleanupWriter(orec, write_lock);
			return *addr;
		}

		// if here, there is a conflict
		if(cm.ShouldRestartReadWrite(&transactions[write_lock_thr]->cm)) {
			// TODO: here maybe cleanup the reader state
			stats.IncrementStatistics(StatisticsType::ABORT_READ_WRITE);
			TxRestart(RESTART_READ_WRITE);
		}

		write_lock = orec->lock;
		write_lock_thr = GET_TX_ID_THREAD(write_lock);
		continue;
	}

	// this cannot happen
	assert(0);
	return NULL;
}

inline bool wlpdstm::TransactionEpoch::CleanupWriter(OwnershipRecord *orec, WriteLock old_value) {
	return atomic_cas_no_barrier(&orec->lock, old_value, NO_THREAD_TX_ID);
}

inline void wlpdstm::TransactionEpoch::WriteWord(Word *addr, Word val, Word mask) {
//PROF	uint64_t write_start_cycles;
//
//	if(profiling.IsSampling()) {
//		stats.IncrementStatistics(StatisticsType::TX_PROFILED_WRITES);
//		write_start_cycles = get_clock_count();
//	}
	
	unsigned orec_idx = map_address_to_index(addr);

	// LockMemoryStripe will restart if it cannot lock
	LockMemoryStripe(addr, orec_idx);
	AddToWriteLog(addr, *addr);

	// then update the value
	*addr = MaskWord(*addr, val, mask);

//	if(profiling.IsSampling()) {
//		stats.IncrementStatistics(StatisticsType::TX_DURATION_WRITE,
//								  get_clock_count() - write_start_cycles);
//	}	
}

inline void wlpdstm::TransactionEpoch::LockMemoryStripe(Word *addr, unsigned orec_idx) {
	OwnershipRecord *orec = orec_table + orec_idx;
	WriteLock write_lock = orec->lock;

	// if already written by this transaction return
	if(write_lock == current_lock) {
		return;
	}

#ifdef USE_TOUCHED_ORECS
	// annonuce touching the orec
	// it is safe to do it here
	unsigned touched_orecs_idx = map_orec_idx_to_touched_orec_idx(orec_idx);

	if(touched_orecs[tid.Get()][touched_orecs_idx] == TOUCHED_ORECS_NO) {
		touched_orecs[tid.Get()][touched_orecs_idx] = TOUCHED_ORECS_YES;
	}
#endif /* USE_TOUCHED_ORECS */
	
	while(true) {
		// if unlocked
		if(write_lock == NO_THREAD_TX_ID) {
			// try to lock
			if(atomic_cas_no_barrier(&orec->lock, write_lock, current_lock)) {
				// if no readers done
				if(CheckForConcurrentReaders(orec)) {
#ifdef USE_WRITE_SET
					AddToWriteSet(orec);
#endif /* USE_WRITE_SET */
					return;
				}

				// if there are some readers that won contention management
				orec->lock = (WriteLock)NO_THREAD_TX_ID;
				stats.IncrementStatistics(StatisticsType::ABORT_WRITE_READ);
				TxRestart(RESTART_WRITE_READ);
			}

			write_lock = orec->lock;
			continue;			
		}

		// locked by some thread that is not this one
		uint32_t write_lock_thr = GET_TX_ID_THREAD(write_lock);
		uint32_t write_lock_cnt = GET_TX_ID_CNT(write_lock);

		// if transaction already committed do the same as for unlocked
		// TODO chekc whether this first check helps speed things up
		if(write_lock_thr == tid.Get() || IsNotCurrentTx(write_lock_thr, write_lock_cnt)) {
			// try to lock
			if(atomic_cas_no_barrier(&orec->lock, write_lock, current_lock)) {
				// if no readers return
				if(CheckForConcurrentReaders(orec)) {
					return;
				}

				// if there are some readers that won contention management
				orec->lock = (WriteLock)NO_THREAD_TX_ID;
				stats.IncrementStatistics(StatisticsType::ABORT_WRITE_READ);
				TxRestart(RESTART_WRITE_READ);
			}

			write_lock = orec->lock;
			continue;
		}

		// this is a conflict with a running transaction
		if(cm.ShouldRestartWriteWrite(&transactions[write_lock_thr]->cm)) {
			stats.IncrementStatistics(StatisticsType::ABORT_WRITE_WRITE);
			TxRestart(RESTART_WRITE_WRITE);
		}

		write_lock = orec->lock;
		continue;
	}

	// this shouldn't happen
	assert(0);
}

inline bool wlpdstm::TransactionEpoch::CheckReader(Reader reader) {
	// if epoch is clean
	if(reader == NO_THREAD_TX_ID) {
		return true;
	}

	uint32_t reader_thr = GET_TX_ID_THREAD(reader);

	// if this thread is the reader
	if(reader_thr == tid.Get()) {
		return true;
	}

	// otherwise check for conflict
	TxCounter reader_cnt = GET_TX_ID_CNT(reader);

	if(IsNotCurrentTx(reader_thr, reader_cnt)) {
		return true;
	}

	// conflict here with the current reader
	if(cm.ShouldRestartWriteRead(&transactions[reader_thr]->cm)) {
		return false;
	}
	
	// spin here until the reader gets aborted
	// TODO maybe do this faster
	while(!IsNotCurrentTx(reader_thr, reader_cnt)) {
		if(cm.ShouldRestart()) {
			return false;
		}
	}

	return true;
}

inline bool wlpdstm::TransactionEpoch::CheckForConcurrentReaders(OwnershipRecord *orec) {
//	stats.IncrementStatistics(StatisticsType::CHECK_FOR_CONCURRENT_READER);

	int reader_count = (int)orec->reader_count;

//	if(reader_count == 0) {
//		stats.IncrementStatistics(StatisticsType::CHECK_FOR_CONCURRENT_READER_NO_READER);
//		return true;
//	}

#ifdef USE_MINIMUM_READER_VERSION
	if(orec->max_orec_rver < min_counter.tx_counter) {
		stats.IncrementStatistics(StatisticsType::CHECK_FOR_CONCURRENT_READER_MIN_VERSION_HIT);
		return true;
	}
#endif /* USE_MINIMUM_READER_VERSION */

	// first check inlined reader epochs
	for(unsigned idx = 0;reader_count > 0 && idx < READERS_IN_OREC;idx++, reader_count--) {		
		if(!CheckReader(orec->readers[idx])) {
			return false;
		}
	}

	// next check arrays
	for(unsigned array_idx = 0;reader_count > 0 && array_idx < READERS_EXTRA_ARRAYS;array_idx++) {
		Reader *array = orec->readers_extra_arrays[array_idx];

		if(array == READERS_EXTRA_ARRAY_EMPTY) {
			reader_count -= READERS_IN_EXTRA_ARRAY;
			continue;
		}

		for(unsigned idx = 0;reader_count > 0 && idx < READERS_IN_EXTRA_ARRAY;idx++, reader_count--) {			
			if(!CheckReader(array[idx])) {
				return false;
			}
		}
	}

	return true;
}

// mask contains ones where value bits are valid
inline Word wlpdstm::TransactionEpoch::MaskWord(Word old, Word val, Word mask) {
	if(mask == LOG_ENTRY_UNMASKED) {
		return val;
	}
	
	return (old & ~mask) | (val & mask);
}

inline void wlpdstm::TransactionEpoch::AddToWriteLog(Word *addr, Word val) {
	WriteLogEntry *log_entry = write_log.get_next();
	log_entry->log_address = addr;
	log_entry->log_value = val;
}

inline void wlpdstm::TransactionEpoch::RollbackWriteLog() {
	for(WriteLog::rev_iterator curr = write_log.rbegin();curr.hasPrev();curr.prev()) {
		WriteLogEntry &log_entry = *curr;
		*log_entry.log_address = log_entry.log_value;
	}

	write_log.clear();
}

inline void wlpdstm::TransactionEpoch::CommitWriteLog() {
	if(write_log.empty()) {
		stats.IncrementStatistics(StatisticsType::COMMIT_READ_ONLY);
	}

	write_log.clear();
}

inline void *wlpdstm::TransactionEpoch::TxMalloc(size_t size) {
//PROF	if(profiling.IsSampling()) {
//		stats.IncrementStatistics(StatisticsType::TX_PROFILED_ALLOCS);
//	}
	
	return mm.TxMalloc(size);
}

inline void wlpdstm::TransactionEpoch::TxFree(void *ptr, size_t size) {
//PROF	if(profiling.IsSampling()) {
//		stats.IncrementStatistics(StatisticsType::TX_PROFILED_FREES);
//	}
	
	LockMemoryBlock(ptr, size);
	mm.TxFree(ptr);
}

inline void wlpdstm::TransactionEpoch::LockMemoryBlock(void *address, size_t size) {
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
inline void wlpdstm::TransactionEpoch::PrintStatistics() {
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

	PrintOwnershipTableStats(out_file);
#ifdef USE_TOUCHED_ORECS
	PrintTouchedOrecsStats(out_file);
#endif /* USE_TOUCHED_ORECS */
#endif /* COLLECT_STATS */
}

inline unsigned wlpdstm::TransactionEpoch::map_address_to_index(Word *address) {
	return ((uintptr_t)address >> (LOCK_EXTENT)) & (OWNERSHIP_TABLE_SIZE - 1);
}

#ifdef USE_TOUCHED_ORECS
inline unsigned wlpdstm::TransactionEpoch::map_orec_idx_to_touched_orec_idx(unsigned orec_idx) {
	return orec_idx >> TOUCHED_ORECS_ENTRY_LOG_SIZE_ORECS;
}
#endif /* USE_TOUCHED_ORECS */


// cleanup for gc threads
//inline void wlpdstm::TransactionEpoch::CleanupOrec(OwnershipRecord *orec) {
//	WriteLock orec_write_lock = orec->lock;
//
//	// cleanup writers
//	if(orec_write_lock != NO_WRITER_LOCK) {
//		uint8_t orec_write_tid = GET_WRITE_LOCK_TID(orec_write_lock);
//		Epoch orec_epoch = GET_WRITE_LOCK_EPOCH(orec_write_lock);
//		EpochState orec_epoch_state = epochs_cleanup_snapshot[orec_write_tid][orec_epoch];
//
//		if(orec_epoch_state == COMMITTED_EPOCH || orec_epoch_state == ABORTED_EPOCH) {
//			CleanupWriter(orec, orec_write_lock);
//		}
//	}
//
//	CleanupReaders(orec);
//}
//
//inline bool wlpdstm::TransactionEpoch::CleanupWord(Word *word, int reader_count) {
//	ReaderEpochToWord old_word, new_word;
//	old_word.word = new_word.word = atomic_load_no_barrier(word);
//
//	for(unsigned idx = 0;idx < READER_EPOCHS_IN_WORD && reader_count > 0;idx++, reader_count--) {
//		Epoch epoch = old_word.reader_epochs[idx].epoch;
//
//		if(epoch == NO_EPOCH) {
//			continue;
//		}
//
//		uint16_t tid = old_word.reader_epochs[idx].tid;
//		EpochState epoch_state = epochs_cleanup_snapshot[tid][epoch];
//
//		if(epoch_state == ABORTED_EPOCH || epoch_state == COMMITTED_EPOCH) {
//			new_word.reader_epochs[idx].epoch = NO_EPOCH;
//		}
//	}
//
//	if(old_word.word == new_word.word) {
//		return true;
//	}
//
//	return atomic_cas_no_barrier(word, old_word.word, new_word.word);
//}
//
//template<unsigned ARRAY_SIZE>
//inline void wlpdstm::TransactionEpoch::CleanupReadersArray(ReaderEpoch *array, int &reader_count) {
//	Word *word_array = (Word *)array;
//
//	for(unsigned i = 0;i < ARRAY_SIZE / READER_EPOCHS_IN_WORD && reader_count > 0;
//			i++, reader_count -= READER_EPOCHS_IN_WORD) {
//		while(!CleanupWord(word_array + i, reader_count)) {
//			// do nothing
//		}
//	}
//}
//
//inline void wlpdstm::TransactionEpoch::CleanupReaders(OwnershipRecord *orec) {
//	int reader_count = (int)orec->reader_count;
//
//	if(reader_count == 0) {
//		return;
//	}
//
//	// first cleanup inlined entries
//	CleanupReadersArray<READER_EPOCHS_IN_OREC>(orec->reader_epochs, reader_count);
//
//	// next cleanup linked entries
//	for(unsigned i = 0;reader_count > 0 && i < READER_EPOCHS_EXTRA_ARRAYS;i++) {
//		ReaderEpoch *array = orec->reader_epochs_extra_arrays[i];
//
//		if(array == READER_EXTRA_ARRAY_EMPTY) {
//			reader_count -= READER_EPOCHS_IN_EXTRA_ARRAY;
//			continue;
//		}
//
//		CleanupReadersArray<READER_EPOCHS_IN_EXTRA_ARRAY>(array, reader_count);
//	}
//}
//
//inline void wlpdstm::TransactionEpoch::MakeEpochSnapshot() {
//	for(unsigned i = 0;i < cleanup_snapshot_thread_count;i++) {
//		for(unsigned j = 0;j < MAX_EPOCH + 1;j++) {
//			epochs_cleanup_snapshot[i][j] = epochs[i][j];
//		}
//	}
//}
//
//inline void wlpdstm::TransactionEpoch::UpdateEpochFromSnapshot() {
//	for(unsigned i = 0;i < cleanup_snapshot_thread_count;i++) {
//		for(unsigned j = 0;j < MAX_EPOCH + 1;j++) {
//			EpochState epoch_state = epochs_cleanup_snapshot[i][j];
//
//			if(epoch_state == COMMITTED_EPOCH || epoch_state == ABORTED_EPOCH) {
//				epochs[i][j] = UNUSED_EPOCH;
//			}
//		}
//	}
//}
//
//inline void wlpdstm::TransactionEpoch::InitializeCleanupQueue() {
//	unsigned idx = 0;
//	unsigned work_item_size = OWNERSHIP_TABLE_SIZE / CLEANUP_WORK_ITEMS;
//
//	for(unsigned i = 0;i < CLEANUP_WORK_ITEMS;i++) {
//		cleanup_queue[i].start_orec = orec_table + idx;
//#ifdef USE_TOUCHED_ORECS
//		cleanup_queue[i].start_touched_orec_idx = map_orec_idx_to_touched_orec_idx(idx);
//#endif /* USE_TOUCHED_ORECS */		
//		idx += work_item_size;
//#ifdef USE_TOUCHED_ORECS
//		cleanup_queue[i].end_touched_orec_idx = map_orec_idx_to_touched_orec_idx(idx);
//#else
//		cleanup_queue[i].end_orec = orec_table + idx;
//#endif /* USE_TOUCHED_ORECS */				
//	}
//
//	// make sure all elements are covered
//#ifdef USE_TOUCHED_ORECS
//	cleanup_queue[CLEANUP_WORK_ITEMS - 1].end_touched_orec_idx = map_orec_idx_to_touched_orec_idx(OWNERSHIP_TABLE_SIZE);
//#else
//	cleanup_queue[CLEANUP_WORK_ITEMS - 1].end_orec =  orec_table + OWNERSHIP_TABLE_SIZE;
//#endif /* USE_TOUCHED_ORECS */					
//}
//
//inline bool wlpdstm::TransactionEpoch::CleanupLoopIter() {
//	Word item_idx = fetch_and_inc_no_barrier(&cleanup_counter.val);
//
//	if(item_idx < CLEANUP_WORK_ITEMS) {
//#ifdef USE_TOUCHED_ORECS
//		const unsigned start_touched_orec_idx = cleanup_queue[item_idx].start_touched_orec_idx;
//		const unsigned end_touched_orec_idx = cleanup_queue[item_idx].end_touched_orec_idx;
//		OwnershipRecord *orec = cleanup_queue[item_idx].start_orec;
//		unsigned touched_orec_idx = start_touched_orec_idx;
//
//		while(touched_orec_idx < end_touched_orec_idx) {
//			if(cleanup_touched_orecs[touched_orec_idx] == TOUCHED_ORECS_YES) {
//				OwnershipRecord *end_orec = orec + TOUCHED_ORECS_ENTRY_SIZE_ORECS;
//
//				while(orec < end_orec) {
//					CleanupOrec(orec++);
//				}
//			} else {
//				orec += TOUCHED_ORECS_ENTRY_SIZE_ORECS;
//			}
//
//			touched_orec_idx++;
//		}
//#else
//		OwnershipRecord *start_orec = cleanup_queue[item_idx].start_orec;
//		OwnershipRecord *end_orec = cleanup_queue[item_idx].end_orec;
//		
//		for(OwnershipRecord *orec = start_orec;orec < end_orec;orec++) {
//			CleanupOrec(orec);
//		}
//#endif /* USE_TOUCHED_ORECS */
//
//		fetch_and_inc_no_barrier(&cleanup_done_counter.val);
//		return true;
//	} else {
//		return false;
//	}
//}
//
//#ifdef MM_EPOCH
//inline bool wlpdstm::TransactionEpoch::CleanupMemoryLoopIter() {
//	Word thread_idx = fetch_and_inc_no_barrier(&cleanup_memory_counter.val);
//	
//	if(thread_idx < MAX_THREADS) {
//		if(transactions[thread_idx] != NULL) {
//			for(unsigned i = 0;i < MAX_EPOCH;i++) {
//				EpochState epoch_state = epochs_cleanup_snapshot[thread_idx][i];
//
//				if(epoch_state == COMMITTED_EPOCH || epoch_state == ABORTED_EPOCH) {
//					transactions[thread_idx]->mm.CleanupEpoch(i);
//				}
//			}
//		}
//		
//		fetch_and_inc_no_barrier(&cleanup_memory_done_counter.val);
//		return true;
//	} else {
//		return false;
//	}
//}
//#endif /* MM_EPOCH */

#ifdef USE_TOUCHED_ORECS
inline void wlpdstm::TransactionEpoch::MergeTouchedOrecsArrays(uint8_t *result, unsigned threads) {
	for(unsigned i = 0;i < TOUCHED_ORECS_ARRAY_SIZE;i++) {
		result[i] = TOUCHED_ORECS_NO;
	}

	for(unsigned i = 0;i < threads;i++) {
		for(unsigned j = 0;j < TOUCHED_ORECS_ARRAY_SIZE;j++) {
			result[j] |= touched_orecs[i][j];
		}
	}
}

//inline void wlpdstm::TransactionEpoch::MakeTouchedOrecsSnapshot() {
//	MergeTouchedOrecsArrays(cleanup_touched_orecs, cleanup_snapshot_thread_count);
//}

inline unsigned wlpdstm::TransactionEpoch::GetTouchedOrecsCount() {
	uint8_t merged_touched_orecs[TOUCHED_ORECS_ARRAY_SIZE];
	MergeTouchedOrecsArrays(merged_touched_orecs, thread_count);
	unsigned ret = 0;

	for(unsigned i = 0;i < TOUCHED_ORECS_ARRAY_SIZE;i++) {
		if(merged_touched_orecs[i] == TOUCHED_ORECS_YES) {
			ret++;
		}
	}

	return ret;
}

#endif /* USE_TOUCHED_ORECS */
			
// TODO think about postponing the start of the cleanup so at least there
// is something to be cleaned up
//inline void *wlpdstm::TransactionEpoch::MainCleanupThread(void *nothing) {
//	EnableThreadCancelation();
//
//	while(!IsSTMStopped()) {
//		cleanup_snapshot_thread_count = thread_count;
//
//		// start the cleanup
//		MakeEpochSnapshot();
//#ifdef USE_TOUCHED_ORECS
//		// finish the whole epoch snapshot before starting the touched orecs snapshot
//		membar_store_load();
//		MakeTouchedOrecsSnapshot();
//#endif /* USE_TOUCHED_ORECS */
//		cleanup_done_counter.val = 0;
//		cleanup_counter.val = 0;
//
//		// execute while there are items in the queue
//		while(CleanupLoopIter()) {
//			// do nothing
//		}
//
//		// wait for all workers to finish
//		while(cleanup_done_counter.val < CLEANUP_WORK_ITEMS) {
//			// do nothing
//		}
//
//#ifdef MM_EPOCH
//		// now cleanup memory
//		cleanup_memory_done_counter.val = 0;
//		cleanup_memory_counter.val = 0;
//
//		while(CleanupMemoryLoopIter()) {
//			// do nothing
//		}
//		
//		// wait for all workers to finish
//		while(cleanup_memory_done_counter.val < CLEANUP_MEMORY_WORK_ITEMS) {
//			// do nothing
//		}
//#endif /* MM_EPOCH */
//
//		membar_load_store_membar_store_store();
//
//		// update epochs
//		UpdateEpochFromSnapshot();
//	}
//
//	return NULL;
//}
//
//inline void *wlpdstm::TransactionEpoch::WorkerCleanupThread(void *nothing) {
//	EnableThreadCancelation();
//
//	while(!IsSTMStopped()) {
//		// wait for start
//		while(cleanup_counter.val >= CLEANUP_WORK_ITEMS) {
//			// do nothing
//		}
//
//		// execute while there are items in the queue
//		while(CleanupLoopIter()) {
//			// do nothing
//		}
//
//#ifdef MM_EPOCH
//		// wait for start
//		while(cleanup_memory_counter.val >= CLEANUP_MEMORY_WORK_ITEMS) {
//			// do nothing
//		}
//
//		// execute while there are items in the queue
//		while(CleanupMemoryLoopIter()) {
//			// do nothing
//		}
//#endif /* MM_EPOCH */
//	}
//
//	return NULL;
//}
//
//inline bool wlpdstm::TransactionEpoch::IsSTMStopped() {
//	pthread_testcancel();
//	return false;
//}
//
//inline void wlpdstm::TransactionEpoch::StartupCleanupThreads(unsigned thread_count) {
//	cleanup_thread_count = thread_count;
//
//	// startup the main cleanup thread
//	pthread_create(&cleanup_threads[0], NULL, TransactionEpoch::MainCleanupThread, NULL);
//
//	// startup worker cleanup threads
//	for(unsigned i = 1;i < cleanup_thread_count;i++) {
//		pthread_create(&cleanup_threads[i], NULL, TransactionEpoch::WorkerCleanupThread, NULL);
//	}
//}

inline void wlpdstm::TransactionEpoch::ThreadShutdown() {
	// nothing
}

inline void wlpdstm::TransactionEpoch::GlobalShutdown() {
//	// cancel all background threads
//	for(unsigned i = 0;i < cleanup_thread_count;i++) {
//		pthread_cancel(cleanup_threads[i]);
//	}

	PrintStatistics();
}

//inline void wlpdstm::TransactionEpoch::EnableThreadCancelation() {
//	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
//	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
//}

// stats about STM
inline void wlpdstm::TransactionEpoch::PrintOwnershipTableStats(FILE *out_file) {
	OrecTableReadStats read_stats;
	GatherOwnershipTableStats(read_stats);
	read_stats.Print("read", out_file);
}

inline void wlpdstm::TransactionEpoch::GatherOwnershipTableStats(OrecTableReadStats &read_stats) {
	read_stats.Init();

	for(OwnershipRecord *orec = orec_table;orec < orec_table + OWNERSHIP_TABLE_SIZE;orec++) {
		read_stats.stats[orec->reader_count][1]++;
	}

	read_stats.Sort();
}

template<unsigned ROWS>
inline void wlpdstm::TransactionEpoch::OrecTableStatsArray<ROWS>::Init() {
	for(unsigned i = 0;i < ROWS;i++) {
		stats[i][0] = i;
		stats[i][1] = 0;
	}
}

template<unsigned ROWS>
inline void wlpdstm::TransactionEpoch::OrecTableStatsArray<ROWS>::Sort() {
	for(unsigned i = 0;i < ROWS - 1;i++) {
		for(unsigned j = i + 1;j < ROWS;j++) {
			if(stats[i][1] < stats[j][1] || (stats[i][1] == stats[j][1] && stats[i][0] > stats[j][0])) {
				unsigned first = stats[j][0];
				unsigned second = stats[j][1];
				stats[j][0] = stats[i][0];
				stats[j][1] = stats[i][1];
				stats[i][0] = first;
				stats[i][1] = second;
			}
		}
	}		
}

template<unsigned ROWS>
inline void wlpdstm::TransactionEpoch::OrecTableStatsArray<ROWS>::Print(const char *msg, FILE *out_file) {
	fprintf(out_file, "Ownership table %s stats:\n", msg);
	fprintf(out_file, "===========================\n");

	for(unsigned i = 0;i < ROWS;i++) {
		if(stats[i][1] != 0) {
			fprintf(out_file, "\t %u: %u\n", stats[i][0], stats[i][1]);
		}
	}
}

#ifdef USE_TOUCHED_ORECS
inline void wlpdstm::TransactionEpoch::PrintTouchedOrecsStats(FILE *out_file) {
	fprintf(out_file, "Total touched orecs: %u\n", GetTouchedOrecsCount());
}
#endif /* USE_TOUCHED_ORECS */

#ifdef USE_MINIMUM_READER_VERSION
inline void wlpdstm::TransactionEpoch::CalculateMinimumCounter() {
	TxCounter min = MAX_TRANSACTION_COUNT;

	for(unsigned i = 0;i < thread_count.val;i++) {
		if(min > counters[i].tx_counter) {
			min = counters[i].tx_counter;
		}
	}

	min_counter.tx_counter = min;
}
#endif /* USE_MINIMUM_READER_VERSION */


#endif /* WLPDSTM_EPOCHSTM_TRANSACTION_H_ */
