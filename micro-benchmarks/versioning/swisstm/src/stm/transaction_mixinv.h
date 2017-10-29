/**
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 * TODO: move privatization quiescence to a separate class
 * TODO: move contention management to a separate class
 * TODO: perhaps move definition of logs into a separate file
 * TODO: seems that there might be some more things that could be done to speed up
 *       writes and improve performance in cases where there seem to be many writes
 *       e.g. kmeans workloads
 * TODO: There is some code repetition in rollback / restart. Maybe figure out how to
 *       to deal with that.
 */


// TODO: writing masked values doesn't really work with lazy conflict detection

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "../common/word.h"
#include "../common/tid.h"
#include "../common/tls.h"
#include "../common/cache_aligned_alloc.h"
#include "../common/jmp.h"
#include "../common/timing.h"
#include "../common/random.h"
#include "../common/padded.h"
#include "../common/log.h"

#include "version_lock.h"
#include "constants.h"
#include "timestamp.h"
#include "memory.h"
#include "stats.h"
#include "large_lock_set.h"

#ifdef USE_PREEMPTIVE_WAITING
#include "../common/preemptive_utils.h"
#endif // USE_PREEMPTIVE_WAITING

#ifdef STACK_PROTECT
#include "../common/arch.h"
#endif /* STACK_PROTECT */

#ifdef PRIVATIZATION_QUIESCENCE_TREE
#include "privatization_tree.h"
#endif /* PRIVATIZATION_QUIESCENCE_TREE */

#include "perfcnt/perf_cnt.h"
#include "../common/sampling.h"

#include "profiling.h"

// default is 2
// how many successive locations (segment size)
#define LOG_DEFAULT_LOCK_EXTENT_SIZE_WORDS 2
#define DEFAULT_LOCK_EXTENT_SIZE (LOG_DEFAULT_LOCK_EXTENT_SIZE_WORDS + LOG_BYTES_IN_WORD)
#define MIN_LOCK_EXTENT_SIZE LOG_BYTES_IN_WORD
#define MAX_LOCK_EXTENT_SIZE 10

#define VERSION_LOCK_TABLE_SIZE (1 << 22)
// two locks are used - write/write and read/write 
#define FULL_VERSION_LOCK_TABLE_SIZE (VERSION_LOCK_TABLE_SIZE << 1)

// rbtree
//     20 no tx go to second phase
//     10 several tx go to second phase
//      5 significant number of tx go to second phase
// default is 10
#define CM_ACCESS_THRESHOLD 10

// rbtree
//     20 number of FreeDominated calls is small and this parameter does not make difference
// default is 20
#define UPDATE_LOCAL_LAST_OBSERVED_TS_FREQUENCY 20

namespace wlpdstm {
	
	typedef VersionLock WriteLock;
	
	class TxMixinv : public CacheAlignedAlloc {
		protected:

		//////////////////////////////////
		// log related structures start //
		//////////////////////////////////
		
		struct ReadLogEntry {
			VersionLock *read_lock;
			VersionLock version;
		};

		// mask is needed here to avoid overwriting non-transactional
		// data with their stale values upon commit 
		struct WriteWordLogEntry {
			Word *address;
			Word value;
			Word mask;
			WriteWordLogEntry *next;
		};
		
		struct WriteLogEntry {
			VersionLock *read_lock;
			WriteLock *write_lock;
			
			VersionLock old_version;

			TxMixinv *owner;

#ifdef CONFLICT_DETECTION_LAZY
			WriteLogEntry *next;
#endif /* CONFLICT_DETECTION_LAZY */
			
			WriteWordLogEntry *head;

			// methods
			void InsertWordLogEntry(Word *address, Word value, Word mask);

			WriteWordLogEntry *FindWordLogEntry(Word *address);

			void ClearWordLogEntries();
		};
		
		typedef Log<ReadLogEntry> ReadLog;
		typedef Log<WriteLogEntry> WriteLog;
		typedef Log<WriteWordLogEntry> WriteWordLogMemPool;
		
		////////////////////////////////
		// log related structures end //
		////////////////////////////////
		
	public:
		// possible CM phases
		enum CmPhase {
			CM_PHASE_INITIAL,
			CM_PHASE_GREEDY
		};
		
//		enum RestartCause {
//			NO_RESTART = 0,
//			RESTART_EXTERNAL,
//			RESTART_LOCK,
//			RESTART_READ_LOCK,
//			RESTART_VALIDATION,
//			RESTART_CLOCK_OVERFLOW
//		};

		// possible statuses of aborted transactions
		enum TxStatus {
			TX_IDLE,
			TX_EXECUTING,
			TX_ABORTED,
			TX_RESTARTED,
			TX_COMMITTED
		};

#ifdef SIGNALING
		enum TxSignals {
			SIGNAL_EMPTY = 0,
			SIGNAL_PRIVATIZATION_VALIDATE = 0x1
		};
#endif /* SIGNALING */

#ifdef WAIT_ON_SUCC_ABORTS
		static const unsigned SUCC_ABORTS_THRESHOLD = 1;
		static const unsigned SUCC_ABORTS_MAX = 10;
		
		static const unsigned WAIT_CYCLES_MULTIPLICATOR = 8000;
#endif /* WAIT_ON_SUCC_ABORTS */
		
	public:
		static void GlobalInit();

		static void InitializeReadLocks();

		static void InitializeWriteLocks();

#ifdef PRIVATIZATION_QUIESCENCE		
		static void InitializeQuiescenceTimestamps();
#endif /* PRIVATIZATION_QUIESCENCE */

#ifdef SIGNALING
		static void InitializeSignaling();
#endif /* SIGNALING */

		void ThreadInit();

		static void GlobalShutdown();

		void ThreadShutdown();

		/**
		 * Start a transaction.
		 */
		void TxStart(int lex_tx_id = NO_LEXICAL_TX);

		enum TryCommitResult {
			COMMIT = 0,
			JUMP_RESTART,
			RESTART_RUNNING,
			RESTART_COMMITTING
		};

		/**
		 * Try to commit a transaction. Return 0 when commit is successful, reason for not succeeding otherwise.
		 */		
		TryCommitResult TxTryCommit();

		/**
		 * Commit a transaction. If commit is not possible, rollback and restart.
		 */		
		void TxCommit();

		/**
		 * Rollback transaction's effects and jump to the beginning with flag specifying abort.
		 */		
		void TxAbort();

		/**
		 * Rollback transaction's effects and jump to the beginning with flag specifying restart.
		 * This is external restart.
		 */				
		void TxRestart();

		void WriteWord(Word *address, Word val, Word mask = LOG_ENTRY_UNMASKED);

		Word ReadWord(Word *address);

		void *TxMalloc(size_t size);

		void TxFree(void *ptr, size_t size);

		void StartThreadProfiling();
		void EndThreadProfiling();

		// maybe this should be organized slightly different
		void LockMemoryBlock(void *address, size_t size);

		bool IsExecuting() const;

		TxStatus GetTxStatus() const;

		int GetTransactionId() const;

		int GetThreadId();

	protected:
		/**
		 * Rollback transaction's effects.
		 */
		void RollbackRunning();
		void RollbackCommitting();

		void RestartRunning();
		void RestartCommitting();
		
		void WriteWordInner(Word *address, Word val, Word mask);

		Word ReadWordInner(Word *address);
		
		static Word MaskWord(Word old, Word val, Word mask);

		static Word MaskWord(WriteWordLogEntry *entry);
		
		void RestartJump();
		
		void AbortJump();
		
		unsigned map_address_to_index(Word *address, unsigned le);
		unsigned map_address_to_index(Word *address);
		VersionLock *map_address_to_read_lock(Word *address);
		WriteLock *map_address_to_write_lock(Word *address);
		WriteLock *map_read_lock_to_write_lock(VersionLock *lock_address);
		VersionLock *map_write_lock_to_read_lock(WriteLock *lock_address);
#ifdef CONFLICT_DETECTION_LAZY
		unsigned map_address_to_hash_set_idx(Word *address);
#endif /* CONFLICT_DETECTION_LAZY */
		
		WriteLogEntry *LockMemoryStripe(WriteLock *write_lock, Word *address);

		// only check for version in read locks
		bool Validate();

		// if version not in read lock, check whether it was
		// locked by this transaction too
		bool ValidateWithReadLocks();
		
		bool Extend();

		bool ShouldExtend(VersionLock version);
		
		bool LockedByMe(WriteLogEntry *log_entry);
		
		Word IncrementCommitTs();
		
		///////////////////////////
		// contention management //
		///////////////////////////
		
		bool ShouldAbortWrite(WriteLock *write_lock);
		
		// TODO: isolate CM in a separate class
		bool CMStrongerThan(TxMixinv *other);
		
		void CmStartTx();
		
		void CmOnAccess();
		
#ifdef WAIT_ON_SUCC_ABORTS
		void WaitOnAbort();
#endif /* WAIT_ON_SUCC_ABORTS */
		
		void Register();
		
		void YieldCPU();
		
#ifdef MM_EPOCH
		void InitLastObservedTs();
		
		void UpdateLastObservedTs(Word ts);
		
	public:
		static Word UpdateMinimumObservedTs();
		
		static Word GetMinimumObservedTs();
#endif /* MM_EPOCH */
		
		////////////////////////////////////////
		// synchronize all transactions start //
		////////////////////////////////////////
		
		bool StartSynchronization();
		
		void EndSynchronization();
		
		bool Synchronize();
		
		void RestartCommitTS();
		
		//////////////////////////////////////
		// synchronize all transactions end //
		//////////////////////////////////////

		static void PrintStatistics();
		
		/////////////////////////
		// protect stack start //
		/////////////////////////
		
#ifdef STACK_PROTECT
#if defined STACK_PROTECT_TANGER_BOUND || defined STACK_PROTECT_ICC_BOUND
	public:
		void SetStackHigh(uintptr_t addr);
#elif defined STACK_PROTECT_WLPDSTM_BOUND
	private:
		void SetStackHigh();
#endif /* STACK_PROTECT_TANGER_BOUND */
		
	protected:
		bool OnStack(uintptr_t addr);
		
		bool OnStack(uintptr_t addr, uintptr_t current_sp);
#endif /* STACK_PROTECT */
		
#ifdef PRIVATIZATION_QUIESCENCE
		void PrivatizationQuiescenceWait(Word ts);
#endif /* PRIVATIZATION_QUIESCENCE */

#ifdef SIGNALING
		void SetSignal(unsigned tid, TxSignals signal);
		void ClearSignals();
		bool IsSignalSet(TxSignals signal);
#endif /* SIGNALING */
		
		///////////////////////
		// protect stack end //
		///////////////////////
		
		////////////////
		// data start //
		////////////////

	public:
		// Local, but should be at the start of the descriptor as this is what
		// assembly jump expects.
		CACHE_LINE_ALIGNED union {
			LONG_JMP_BUF start_buf;
			char padding_start_buf[CACHE_LINE_SIZE_BYTES * START_BUF_PADDING_SIZE];
		};
		
	private:
		// shared data aligned as needed
		// assumption here is that the whole descriptor is already aligned
		
		// data accessed by other tx
		// r shared
		CACHE_LINE_ALIGNED union {
			volatile Word greedy_ts;
			char padding_greedy_ts[CACHE_LINE_SIZE_BYTES];
		};
		
		// w shared
		CACHE_LINE_ALIGNED union {
			volatile bool aborted_externally;
			char padding_aborted_extrenally[CACHE_LINE_SIZE_BYTES];
		};
		
#ifdef MM_EPOCH
		// r shared
		CACHE_LINE_ALIGNED union {
			// used for memory management
			volatile Word last_observed_ts;
			char padding_last_observed_ts[CACHE_LINE_SIZE_BYTES];
		};
#endif /* MM_EPOCH */
		
		CACHE_LINE_ALIGNED union {
			volatile Word tx_status;
			char padding_tx_status[CACHE_LINE_SIZE_BYTES];
		};
		
		// tx local data next
		// its alignment is not important, as it is not shared
		
		Word valid_ts;
		
#ifdef PRIVATIZATION_QUIESCENCE
		Word *quiescence_ts;
#elif defined PRIVATIZATION_QUIESCENCE_TREE
		PrivatizationTree privatization_tree;
#endif /* PRIVATIZATION_QUIESCENCE */

#ifdef SIGNALING
		volatile Word *signals;
#endif /* SIGNALING */

	protected:
#ifdef STACK_PROTECT
		// local
		uintptr_t stack_high;
#endif /* STACK_PROTECT */
		
		// local
		ReadLog read_log;
		
		// local
		WriteLog write_log;
		
		// local
		WriteWordLogMemPool write_word_log_mem_pool;
		
		// contention management
		// local
		unsigned locations_accessed;
		
		// local
		CmPhase cm_phase;
		
		// local
		unsigned succ_aborts;
		
		// memory management support
		// local
		unsigned start_count;
		
	public:
		// local
		Tid tid;
		
		// local
		MemoryManager mm;

	protected:
		// local
		ThreadStatistics stats;

		// local
		Random random;

		Profiling profiling;

		// local
		// used to prevent multiple rollbacks
		bool rolled_back;

#ifdef PERFORMANCE_COUNTING
	protected:
		Sampling perf_cnt_sampling;
		PerfCnt perf_cnt;

#endif /* PERFORMANCE_COUNTING */

#ifdef CONFLICT_DETECTION_LAZY
		static const unsigned HASH_WRITE_SET_SIZE = 256;
		static const unsigned HASH_WRITE_SET_MARKERS_SIZE_WORDS = HASH_WRITE_SET_SIZE / BYTES_IN_WORD;
		static const unsigned HASH_WRITE_SET_MASK = HASH_WRITE_SET_SIZE - 1;

		WriteLogEntry *hash_write_set[HASH_WRITE_SET_SIZE];
		uint8_t hash_write_set_markers[HASH_WRITE_SET_SIZE];

		void ClearHashWriteSet();
#endif /* CONFLICT_DETECTION_LAZY */

		//////////////////////
		// shared variables //
		//////////////////////

	protected:
		static VersionLock version_lock_table[FULL_VERSION_LOCK_TABLE_SIZE];
		
		static GlobalTimestamp commit_ts;
		
		static GlobalTimestamp cm_ts;
		
		static PaddedWord minimum_observed_ts;
		
		static PaddedSpinTryLock minimum_observed_ts_lock;
		
		static TxMixinv *transactions[MAX_THREADS];
		
		static Word thread_count;
		
		static PaddedBool synchronization_in_progress;
		
#ifdef PRIVATIZATION_QUIESCENCE
		static volatile Word quiescence_timestamp_array[MAX_THREADS];
#endif /* PRIVATIZATION_QUIESCENCE */

#ifdef SIGNALING
		static volatile PaddedWord signaling_array[MAX_THREADS];
#endif /* SIGNALING */		

	} __attribute__ ((aligned(CACHE_LINE_SIZE_BYTES)));
}

#define LOCK_EXTENT DEFAULT_LOCK_EXTENT_SIZE

//////////////////////////
// initialization start //
//////////////////////////

inline void wlpdstm::TxMixinv::GlobalInit() {
	InitializeReadLocks();
	
	InitializeWriteLocks();
	
#ifdef PRIVATIZATION_QUIESCENCE
	InitializeQuiescenceTimestamps();
#elif defined PRIVATIZATION_QUIESCENCE_TREE
	PrivatizationTree::GlobalInit();
#endif /* PRIVATIZATION_QUIESCENCE */

#ifdef SIGNALING
	InitializeSignaling();
#endif /* SIGNALING */
	
	// initialize shared data
	minimum_observed_ts.val = MINIMUM_TS;
	
	// initialize memory manager
	MemoryManager::GlobalInit();
	
	synchronization_in_progress.val = false;
	
#ifdef PERFORMANCE_COUNTING
	PerfCnt::GlobalInit();
#endif /* PERFORMANCE_COUNTING */

	Profiling::GlobalInit();
}

inline void wlpdstm::TxMixinv::InitializeReadLocks() {
	VersionLock initial_version = get_version_lock(MINIMUM_TS + 1);
	
	for(int i = 0;i < FULL_VERSION_LOCK_TABLE_SIZE;i += 2) {
		version_lock_table[i] = initial_version;
	}	
}

inline void wlpdstm::TxMixinv::InitializeWriteLocks() {
	for(int i = 1;i < FULL_VERSION_LOCK_TABLE_SIZE;i += 2) {
		version_lock_table[i] = WRITE_LOCK_CLEAR;
	}
}

#ifdef PRIVATIZATION_QUIESCENCE
inline void wlpdstm::TxMixinv::InitializeQuiescenceTimestamps() {
	for(unsigned i = 0;i < MAX_THREADS;i++) {
		quiescence_timestamp_array[i] = MINIMUM_TS;
	}	
}
#endif /* PRIVATIZATION_QUIESCENCE */

#ifdef SIGNALING
inline void wlpdstm::TxMixinv::InitializeSignaling() {
	for(unsigned i = 0;i < MAX_THREADS;i++) {
		signaling_array[i].val = SIGNAL_EMPTY;
	}	
}
#endif /* SIGNALING */

inline void wlpdstm::TxMixinv::ThreadInit() {
	aborted_externally = false;
	
#ifdef MM_EPOCH
	InitLastObservedTs();
#endif /* MM_EPOCH */
	
	Register();
	
	// locally initialize memory manager
	mm.ThreadInit(tid.Get());
	
#ifdef COLLECT_STATS
	mm.InitStats(&stats);
#endif /* COLLECT_STATS */
	
	tx_status = (Word)TX_IDLE;

#ifdef PRIVATIZATION_QUIESCENCE_TREE
	privatization_tree.ThreadInit(tid.Get(), &thread_count);
#endif /* PRIVATIZATION_QUIESCENCE_TREE */
	
	succ_aborts = 0;

	assert((uintptr_t)this == (uintptr_t)(&start_buf));

#ifdef PERFORMANCE_COUNTING
	perf_cnt.ThreadInit();
#endif /* PERFORMANCE_COUNTING */

	profiling.ThreadInit(&stats);

#ifdef CONFLICT_DETECTION_LAZY
	ClearHashWriteSet();
#endif /* CONFLICT_DETECTION_LAZY */
}

#ifdef CONFLICT_DETECTION_LAZY
inline void wlpdstm::TxMixinv::ClearHashWriteSet() {
	Word *hash_write_set_markers_word = (Word *)hash_write_set_markers;
	
	for(unsigned i = 0;i < HASH_WRITE_SET_MARKERS_SIZE_WORDS;i++) {
		hash_write_set_markers_word[i] = 0;
	}	
}
#endif /* CONFLICT_DETECTION_LAZY */

////////////////////////
// initialization end //
////////////////////////

inline void wlpdstm::TxMixinv::GlobalShutdown() {
	PrintStatistics();
}

inline void wlpdstm::TxMixinv::ThreadShutdown() {
	profiling.ThreadShutdown();
	// nothing
}

inline wlpdstm::TxMixinv::TxStatus wlpdstm::TxMixinv::GetTxStatus() const {
	return (TxStatus)tx_status;
}

inline bool wlpdstm::TxMixinv::IsExecuting() const {
	return tx_status == TX_EXECUTING;
}

inline int wlpdstm::TxMixinv::GetTransactionId() const {
	return stats.lexical_tx_id;
}

inline int wlpdstm::TxMixinv::GetThreadId() {
	return tid.Get();
}


////////////////////////////
// mapping to locks start //
////////////////////////////

inline unsigned wlpdstm::TxMixinv::map_address_to_index(Word *address) {
	return map_address_to_index(address, LOCK_EXTENT) << 1;
}

inline unsigned wlpdstm::TxMixinv::map_address_to_index(Word *address, unsigned le) {
	return (((uintptr_t)address >> (le)) & (VERSION_LOCK_TABLE_SIZE - 1));
}

inline wlpdstm::VersionLock *wlpdstm::TxMixinv::map_address_to_read_lock(Word *address) {
	return version_lock_table + map_address_to_index(address);
}

inline wlpdstm::WriteLock *wlpdstm::TxMixinv::map_address_to_write_lock(Word *address) {
	return map_address_to_read_lock(address) + 1;
}

inline wlpdstm::WriteLock *wlpdstm::TxMixinv::map_read_lock_to_write_lock(VersionLock *lock_address) {
	return lock_address + 1;
}

inline wlpdstm::VersionLock *wlpdstm::TxMixinv::map_write_lock_to_read_lock(WriteLock *lock_address) {
	return lock_address - 1;
}

#ifdef CONFLICT_DETECTION_LAZY
inline unsigned wlpdstm::TxMixinv::map_address_to_hash_set_idx(Word *address) {
	return (((uintptr_t)address) >> LOCK_EXTENT) & HASH_WRITE_SET_MASK;
}
#endif /* CONFLICT_DETECTION_LAZY */

//////////////////////////
// mapping to locks end //
//////////////////////////


//////////////////////////
// main algorithm start //
//////////////////////////

inline void wlpdstm::TxMixinv::TxStart(int lex_tx_id) {
	stats.TxStart(lex_tx_id);
	profiling.TxStartStart(lex_tx_id);

#ifdef PERFORMANCE_COUNTING
	perf_cnt_sampling.tx_start();

	if(perf_cnt_sampling.should_sample()) {
		perf_cnt.TxStart();
	}
#endif /* PERFORMANCE_COUNTING */

	atomic_store_full(&tx_status, (Word)TX_EXECUTING);
	
	if(Synchronize()) {
		tx_status = (Word)TX_EXECUTING;
	}
	
#ifdef STACK_PROTECT_WLPDSTM_BOUND
	SetStackHigh();
#endif /* STACK_PROTECT_WLPDSTM_BOUND */
	
	mm.TxStart();
	
	// get validity timestamp
	valid_ts = commit_ts.readCurrentTsAcquire();
#ifdef PRIVATIZATION_QUIESCENCE
	*quiescence_ts = valid_ts;
#elif defined PRIVATIZATION_QUIESCENCE_TREE
	privatization_tree.setNonMinimumTs(valid_ts);
#endif /* PRIVATIZATION_QUIESCENCE */

#ifdef MM_EPOCH
	UpdateLastObservedTs(valid_ts);
#endif /* MM_EPOCH */
	
	CmStartTx();
	
	// reset aborted flag
	aborted_externally = false;
	
	// initialize lexical tx id
	stats.lexical_tx_id = lex_tx_id;

	// not rolled back yet
	rolled_back = false;
	
#ifdef SIGNALING
	ClearSignals();
#endif /* SIGNALING */

	profiling.TxStartEnd();
}

inline Word wlpdstm::TxMixinv::IncrementCommitTs() {
#ifdef COMMIT_TS_INC
	return commit_ts.getNextTsRelease() + 1;
#elif defined COMMIT_TS_GV4
	return commit_ts.GenerateTsGV4();
#endif /* commit_ts */
}

inline void wlpdstm::TxMixinv::TxCommit() {
	profiling.TxCommitStart();

	TryCommitResult result = TxTryCommit();

	if(result == JUMP_RESTART) {
		RestartJump();
	} else if(result == RESTART_RUNNING) {
		RestartRunning();
	} else if(result == RESTART_COMMITTING) {
		RestartCommitting();
	}
#ifdef PERFORMANCE_COUNTING
	if(perf_cnt_sampling.should_sample()) {
		// if tx is restarted, this code is not reached
		perf_cnt.TxEnd();
		stats.IncrementStatistics(StatisticsType::CYCLES, perf_cnt.GetElapsedCycles());
		stats.IncrementStatistics(StatisticsType::RETIRED_INSTRUCTIONS, perf_cnt.GetRetiredInstructions());
		stats.IncrementStatistics(StatisticsType::CACHE_MISSES, perf_cnt.GetCacheMisses());
	}
#endif /* PERFORMANCE_COUNTING */

	profiling.TxCommitEnd();
	stats.TxCommit();
}

inline wlpdstm::TxMixinv::TryCommitResult wlpdstm::TxMixinv::TxTryCommit() {
	Word ts = valid_ts;
	bool read_only = write_log.empty();
	
	if(!read_only) {
#ifdef CONFLICT_DETECTION_LAZY
		WriteLog::iterator iter;

		for(iter = write_log.begin();iter.hasNext();iter.next()) {
			WriteLogEntry &entry = *iter;
			WriteLock *write_lock = entry.write_lock;

			if(atomic_cas_release(write_lock, WRITE_LOCK_CLEAR, &entry)) {
				entry.old_version = *entry.read_lock;
				*entry.read_lock = READ_LOCK_SET;
			} else {
				// TODO: use proper conflict detection here
				// abort this immediately for now
				for(WriteLog::iterator clean_iter = write_log.begin();clean_iter != iter;clean_iter.next()) {
					WriteLogEntry &entry = *iter;
					*entry.read_lock = entry.old_version;
					*entry.write_lock = WRITE_LOCK_CLEAR;
				}

				return RESTART_RUNNING;
			}
		}
#elif defined CONFLICT_DETECTION_MIXED
		// first lock all read locks
		for(WriteLog::iterator iter = write_log.begin();iter.hasNext();iter.next()) {
			WriteLogEntry &entry = *iter;
			*(entry.read_lock) = READ_LOCK_SET;
		}
#endif /* CONFLICT_DETECTION_MIXED */
		
		// now get a commit timestamp
		ts = IncrementCommitTs();
		
		// if global time overflows restart
		if(ts >= MAXIMUM_TS) {
			//executing = false;
			tx_status = (Word)TX_ABORTED;
#ifdef PRIVATIZATION_QUIESCENCE
			*quiescence_ts = MINIMUM_TS;
#elif defined PRIVATIZATION_QUIESCENCE_TREE
			privatization_tree.setNonMinimumTs(MINIMUM_TS);
#endif /* PRIVATIZATION_QUIESCENCE */
			// this is a special case where no jump is required
			RollbackCommitting();
			
			if(StartSynchronization()) {
				RestartCommitTS();
				EndSynchronization();
				stats.IncrementStatistics(StatisticsType::CLOCK_OVERFLOWS);
			}

			return JUMP_RESTART;
		}

		// if there is no validation in GV4, these the read set of one transaction could
		// overlap with the write set of another and this would pass unnoticed
#ifdef COMMIT_TS_INC
		if(ts != valid_ts + 1 && !ValidateWithReadLocks()) {
#elif defined COMMIT_TS_GV4
		if(!ValidateWithReadLocks()) {
#endif /* commit_ts */
			stats.IncrementStatistics(StatisticsType::ABORT_COMMIT_VALIDATE);
			return RESTART_COMMITTING;
		}
		
		VersionLock commitVersion = get_version_lock(ts);
		
		// now update all written values
		for(WriteLog::iterator iter = write_log.begin();iter.hasNext();iter.next()) {
			WriteLogEntry &entry = *iter;
			
			// now update actual values
			WriteWordLogEntry *word_log_entry = entry.head;

			while(word_log_entry != NULL) {
				*word_log_entry->address = MaskWord(word_log_entry);
				word_log_entry = word_log_entry->next;
			}

			// release locks
			atomic_store_release(entry.read_lock, commitVersion);
			atomic_store_release(entry.write_lock, WRITE_LOCK_CLEAR);
		}

	} else {
		stats.IncrementStatistics(StatisticsType::COMMIT_READ_ONLY);
	}
	
	atomic_store_release(&tx_status, TX_COMMITTED);
		
#ifdef PRIVATIZATION_QUIESCENCE
	atomic_store_release(quiescence_ts, MINIMUM_TS);
	PrivatizationQuiescenceWait(ts);
#elif defined PRIVATIZATION_QUIESCENCE_TREE
	privatization_tree.setNonMinimumTs(MINIMUM_TS);
	privatization_tree.wait(ts);
#endif /* PRIVATIZATION_QUIESCENCE */

	if(!read_only) {
		write_log.clear();
		write_word_log_mem_pool.clear();
#ifdef CONFLICT_DETECTION_LAZY
		ClearHashWriteSet();
#endif /* CONFLICT_DETECTION_LAZY */		
	}

	read_log.clear();
	
	// commit mem
	mm.TxCommit<TxMixinv>(ts);
	
	stats.IncrementStatistics(StatisticsType::COMMIT);
	
	succ_aborts = 0;
	
	return COMMIT;
}

inline void wlpdstm::TxMixinv::RollbackRunning() {
	if(rolled_back) {
		return;
	}

	rolled_back = true;

#ifndef CONFLICT_DETECTION_LAZY
	for(WriteLog::iterator iter = write_log.begin();iter.hasNext();iter.next()) {
		WriteLogEntry &entry = *iter;
#ifdef CONFLICT_DETECTION_EAGER
		*entry.read_lock = entry.old_version;
#endif /* CONFLICT_DETECTION_EAGER */
		*entry.write_lock = WRITE_LOCK_CLEAR;
	}
#endif /* CONFLICT_DETECTION_LAZY */

	read_log.clear();
	write_log.clear();
	write_word_log_mem_pool.clear();
	
#ifdef CONFLICT_DETECTION_LAZY
	ClearHashWriteSet();
#endif /* CONFLICT_DETECTION_LAZY */
	
	YieldCPU();
	mm.TxAbort();
}

inline void wlpdstm::TxMixinv::RollbackCommitting() {
	if(rolled_back) {
		return;
	}
	
	rolled_back = true;	

	for(WriteLog::iterator iter = write_log.begin();iter.hasNext();iter.next()) {
		WriteLogEntry &entry = *iter;
		*entry.read_lock = entry.old_version;
		atomic_store_release(entry.write_lock, WRITE_LOCK_CLEAR);
	}

	read_log.clear();
	write_log.clear();
	write_word_log_mem_pool.clear();

#ifdef CONFLICT_DETECTION_LAZY
	ClearHashWriteSet();
#endif /* CONFLICT_DETECTION_LAZY */

	YieldCPU();
	mm.TxAbort();
}

inline void wlpdstm::TxMixinv::RestartRunning() {
	profiling.TxRestartStart();
#ifdef PRIVATIZATION_QUIESCENCE
	*quiescence_ts = MINIMUM_TS;
#elif defined PRIVATIZATION_QUIESCENCE_TREE
	privatization_tree.setNonMinimumTs(MINIMUM_TS);
#endif /* PRIVATIZATION_QUIESCENCE */

	RollbackRunning();
	atomic_store_release(&tx_status, (Word)TX_RESTARTED);

#ifdef WAIT_ON_SUCC_ABORTS
	if(++succ_aborts > SUCC_ABORTS_MAX) {
		succ_aborts = SUCC_ABORTS_MAX;
	}

	if(succ_aborts >= SUCC_ABORTS_THRESHOLD) {
		WaitOnAbort();
	}
#endif /* WAIT_ON_SUCC_ABORTS */	

#ifdef PERFORMANCE_COUNTING
	if(perf_cnt_sampling.should_sample()) {
		perf_cnt.TxEnd();
		stats.IncrementStatistics(StatisticsType::CYCLES, perf_cnt.GetElapsedCycles());
		stats.IncrementStatistics(StatisticsType::RETIRED_INSTRUCTIONS, perf_cnt.GetRetiredInstructions());
		stats.IncrementStatistics(StatisticsType::CACHE_MISSES, perf_cnt.GetCacheMisses());
	}
#endif /* PERFORMANCE_COUNTING */
	stats.IncrementStatistics(StatisticsType::ABORT);
	profiling.TxRestartEnd();
	stats.TxRestart();
	RestartJump();
}

inline void wlpdstm::TxMixinv::RestartCommitting() {
	profiling.TxRestartStart();
#ifdef PRIVATIZATION_QUIESCENCE
	*quiescence_ts = MINIMUM_TS;
#elif defined PRIVATIZATION_QUIESCENCE_TREE
	privatization_tree.setNonMinimumTs(MINIMUM_TS);
#endif /* PRIVATIZATION_QUIESCENCE */
	
	RollbackCommitting();
	atomic_store_release(&tx_status, (Word)TX_RESTARTED);
	
#ifdef WAIT_ON_SUCC_ABORTS
	if(++succ_aborts > SUCC_ABORTS_MAX) {
		succ_aborts = SUCC_ABORTS_MAX;
	}
	
	if(succ_aborts >= SUCC_ABORTS_THRESHOLD) {
		WaitOnAbort();
	}
#endif /* WAIT_ON_SUCC_ABORTS */	
	
#ifdef PERFORMANCE_COUNTING
	if(perf_cnt_sampling.should_sample()) {
		perf_cnt.TxEnd();
		stats.IncrementStatistics(StatisticsType::CYCLES, perf_cnt.GetElapsedCycles());
		stats.IncrementStatistics(StatisticsType::RETIRED_INSTRUCTIONS, perf_cnt.GetRetiredInstructions());
		stats.IncrementStatistics(StatisticsType::CACHE_MISSES, perf_cnt.GetCacheMisses());
	}
#endif /* PERFORMANCE_COUNTING */
	stats.IncrementStatistics(StatisticsType::ABORT);
	profiling.TxRestartEnd();
	stats.TxRestart();	
	RestartJump();	
}

inline wlpdstm::TxMixinv::WriteLogEntry *wlpdstm::TxMixinv::LockMemoryStripe(WriteLock *write_lock, Word *address) {
#ifdef CONFLICT_DETECTION_LAZY
	unsigned hash_write_set_idx = map_address_to_hash_set_idx(address);
	WriteLogEntry *first_log_entry, *log_entry;
	first_log_entry = log_entry = hash_write_set[hash_write_set_idx];

	if(hash_write_set_markers[hash_write_set_idx]) {
		while(log_entry != NULL) {
			if(log_entry->write_lock == write_lock) {
				return log_entry;
			}

			log_entry = log_entry->next;
		}
	}

	log_entry = write_log.get_next();
	log_entry->write_lock = write_lock;
	log_entry->ClearWordLogEntries(); // need this here TODO - maybe move this to commit/abort time
	log_entry->owner = this; // this is for CM - TODO: try to move it out of write path
	log_entry->read_lock = map_write_lock_to_read_lock(write_lock);
	
	if(hash_write_set_markers[hash_write_set_idx]) {
		log_entry->next = first_log_entry;
	} else {
		hash_write_set_markers[hash_write_set_idx] = 1;
		log_entry->next = NULL;
	}

	hash_write_set[hash_write_set_idx] = log_entry;
	return log_entry;
#else
#ifdef DETAILED_STATS
	stats.IncrementStatistics(StatisticsType::WRITES);
#endif /* DETAILED_STATS */
	
	// read lock value
	WriteLock lock_value = (WriteLock)atomic_load_no_barrier(write_lock);
	bool locked = is_write_locked(lock_value);
	
	if(locked) {
		WriteLogEntry *log_entry = (WriteLogEntry *)lock_value;
		
		if(LockedByMe(log_entry)) {
			return log_entry;
		}
	}
#ifdef DETAILED_STATS
	stats.IncrementStatistics(StatisticsType::NEW_WRITES);
#endif /* DETAILED_STATS */
	
	while(true) {		
		if(locked) {
			if(ShouldAbortWrite(write_lock)) {
				stats.IncrementStatistics(StatisticsType::ABORT_WRITE_LOCKED);
				RestartRunning();
			} else {
				lock_value = (WriteLock)atomic_load_acquire(write_lock);
				locked = is_write_locked(lock_value);
				YieldCPU();
			}
		}
		
		// prepare write log entry
		WriteLogEntry *log_entry = write_log.get_next();
		log_entry->write_lock = write_lock;
		log_entry->ClearWordLogEntries(); // need this here TODO - maybe move this to commit/abort time
		log_entry->owner = this; // this is for CM - TODO: try to move it out of write path
		
		// now try to lock it
		if(atomic_cas_release(write_lock, WRITE_LOCK_CLEAR, log_entry)) {
			// need to check read set validity if this address was read before
			// without this check, validity might not be ensured, as some other transaction
			// might have changed the version after it was read, but before it was locked by this
			// transaction and this will not be checked by validation at commit time (which might be
			// the only next validation)
			// we skip this read_before() check TODO: maybe do that
			VersionLock *read_lock = map_write_lock_to_read_lock(write_lock);
			VersionLock version = (VersionLock)atomic_load_acquire(read_lock);

			if(ShouldExtend(version)) {
				if(!Extend()) {
					stats.IncrementStatistics(StatisticsType::ABORT_WRITE_VALIDATE);
#ifdef CONFLICT_DETECTION_EAGER
					*write_lock = WRITE_LOCK_CLEAR;
					write_log.delete_last();
#endif /* CONFLICT_DETECTION_EAGER */
					RestartRunning();
				}
			}
			
			// success
			log_entry->read_lock = read_lock;
			log_entry->old_version = version;
#ifdef CONFLICT_DETECTION_EAGER
			*read_lock = READ_LOCK_SET;
#endif /* CONFLICT_DETECTION_EAGER */
			CmOnAccess();
			
			return log_entry;
		}
		
		// someone locked it in the meantime
		// return last element back to the log
		write_log.delete_last();
		
		// read version again
		lock_value = (WriteLock)atomic_load_acquire(write_lock);
		locked = is_write_locked(lock_value);
		YieldCPU();
	}
	
	// this can never happen
	return NULL;
#endif /* CONFLICT_DETECTION_LAZY */
}

inline void wlpdstm::TxMixinv::WriteWord(Word *address, Word value, Word mask) {
	profiling.WriteWordStart(address, value, mask);
#ifdef STACK_PROTECT_ON_WRITE
	if(OnStack((uintptr_t)address)) {
		*address = MaskWord(*address, value, mask);
	} else {
		WriteWordInner(address, value, mask);
	}
#else
	WriteWordInner(address, value, mask);
#endif /* STACK_PROTECT_ON_WRITE */
	profiling.WriteWordEnd();
}

inline void wlpdstm::TxMixinv::WriteWordInner(Word *address, Word value, Word mask) {
	// map address to the lock
	VersionLock *write_lock = map_address_to_write_lock(address);
	
	// try to lock the address - it will abort if address cannot be locked
	WriteLogEntry *log_entry = LockMemoryStripe(write_lock, address);
	
	// insert (address, value) pair into the log
	log_entry->InsertWordLogEntry(address, value, mask);
}

inline wlpdstm::TxMixinv::WriteWordLogEntry *wlpdstm::TxMixinv::WriteLogEntry::FindWordLogEntry(Word *address) {
	WriteWordLogEntry *curr = head;
	
	while(curr != NULL) {
		if(curr->address == address) {
			break;
		}
		
		curr = curr->next;
	}
	
	return curr;
}

inline void wlpdstm::TxMixinv::WriteLogEntry::InsertWordLogEntry(Word *address, Word value, Word mask) {
	WriteWordLogEntry *entry = FindWordLogEntry(address);

	// new entry
	if(entry == NULL) {
		entry = owner->write_word_log_mem_pool.get_next();
		entry->address = address;
		entry->next = head;
		entry->value = value;
		entry->mask = mask;
		head = entry;
	} else {
		entry->value = MaskWord(entry->value, value, mask);
		entry->mask |= mask;
	}
}

// mask contains ones where value bits are valid
inline Word wlpdstm::TxMixinv::MaskWord(Word old, Word val, Word mask) {
	if(mask == LOG_ENTRY_UNMASKED) {
		return val;
	}
	
	return (old & ~mask) | (val & mask);
}

inline Word wlpdstm::TxMixinv::MaskWord(WriteWordLogEntry *entry) {
	return MaskWord(*entry->address, entry->value, entry->mask);
}

inline void wlpdstm::TxMixinv::WriteLogEntry::ClearWordLogEntries() {
	head = NULL;
}

inline Word wlpdstm::TxMixinv::ReadWord(Word *address) {
	profiling.ReadWordStart(address);

	Word ret;
	
#ifdef STACK_PROTECT_ON_READ
	if(OnStack((uintptr_t)address)) {
		ret = *address;
	} else {
		ret = ReadWordInner(address);
	}
#else
	ret = ReadWordInner(address);
#endif /* STACK_PROTECT_ON_READ */

	profiling.ReadWordEnd(ret);
	return ret;
}

inline Word wlpdstm::TxMixinv::ReadWordInner(Word *address) {
	WriteLock *write_lock = map_address_to_write_lock(address);
#ifdef CONFLICT_DETECTION_LAZY
	unsigned hash_write_set_idx = map_address_to_hash_set_idx(address);
	WriteLogEntry *first_log_entry, *log_entry;
	first_log_entry = log_entry = hash_write_set[hash_write_set_idx];
	
	if(hash_write_set_markers[hash_write_set_idx]) {
		while(log_entry != NULL) {
			if(log_entry->write_lock == write_lock) {
				break;
			}
			
			log_entry = log_entry->next;
		}
	}

	if(log_entry != NULL) {
		WriteWordLogEntry *word_log_entry = log_entry->FindWordLogEntry(address);
		
		if(word_log_entry != NULL) {
			// if it was written return from log
			return MaskWord(word_log_entry);
		}			
	}
#else
	WriteLogEntry *log_entry = (WriteLogEntry *)atomic_load_no_barrier(write_lock);
	
	// if locked by me return quickly
	if(LockedByMe(log_entry)) {
		WriteWordLogEntry *word_log_entry = log_entry->FindWordLogEntry(address);
		
		if(word_log_entry != NULL) {
			// if it was written return from log
			return MaskWord(word_log_entry);
		} else {
			// if it was not written return from memory
			return (Word)atomic_load_no_barrier(address);
		}		
	}
#endif /* CONFLICT_DETECTION_LAZY */

	VersionLock *read_lock = map_write_lock_to_read_lock(write_lock);
	VersionLock version = (VersionLock)atomic_load_acquire(read_lock);
	Word value;

	while(true) {
		if(is_read_locked(version)) {
			// TODO: be careful here if ever making this support all three variants
			// correct strategy here depends on the strategy of the other transaction
			// not this one
#ifdef CONFLICT_DETECTION_MIXED
			// just wait for the other guy to finish its commit
			version = (VersionLock)atomic_load_acquire(read_lock);
			YieldCPU();
			continue;
#elif defined CONFLICT_DETECTION_LAZY
			// just wait for the other guy to finish its commit
			version = (VersionLock)atomic_load_acquire(read_lock);
			YieldCPU();
			continue;			
#elif defined CONFLICT_DETECTION_EAGER
			// TODO: investigate conflict detection schemes here
			// for now just restart
			RestartRunning();
#endif /* conflict detection choice */
		}

		value = (Word)atomic_load_acquire(address);
		VersionLock version_2 = (VersionLock)atomic_load_acquire(read_lock);

		if(version != version_2) {
			version = version_2;
			YieldCPU();
			continue;
		}

		ReadLogEntry *entry = read_log.get_next();
		entry->read_lock = read_lock;
		entry->version = version;		

		if(ShouldExtend(version)) {
			if(!Extend()) {
				// need to restart here
				stats.IncrementStatistics(StatisticsType::ABORT_READ_VALIDATE);
				RestartRunning();
			}
		}
		
		break;
	}
	
	return value;
}

inline bool wlpdstm::TxMixinv::ShouldExtend(VersionLock version) {
	if(get_value(version) > valid_ts) {
		return true;
	}

#ifdef SIGNALING
	if(IsSignalSet(SIGNAL_PRIVATIZATION_VALIDATE)) {
		ClearSignals();
		return true;
	}
#endif /* SIGNALING */

	return false;
}

// this is validate that occurs during reads and writes not during commit
inline bool wlpdstm::TxMixinv::Validate() {
	ReadLog::iterator iter;
	
	for(iter = read_log.begin();iter.hasNext();iter.next()) {
		ReadLogEntry &entry = *iter;
		VersionLock currentVersion = (VersionLock)atomic_load_no_barrier(entry.read_lock);
		
		if(currentVersion != entry.version) {
			return false;
		}
	}
	
	return true;
}

// validate invoked at commit time
inline bool wlpdstm::TxMixinv::ValidateWithReadLocks() {
	ReadLog::iterator iter;
	
	for(iter = read_log.begin();iter.hasNext();iter.next()) {
		ReadLogEntry &entry = *iter;
		VersionLock currentVersion = (VersionLock)atomic_load_no_barrier(entry.read_lock);
		
		if(currentVersion != entry.version) {
			if(is_read_locked(currentVersion)) {
				WriteLock *write_lock = map_read_lock_to_write_lock(entry.read_lock);
				WriteLogEntry *log_entry = (WriteLogEntry *)atomic_load_no_barrier(write_lock);
				
				if(LockedByMe(log_entry)) {
					continue;
				}
			}
			
			return false;
		}
	}
	
	return true;
}

inline bool wlpdstm::TxMixinv::Extend() {
	unsigned ts = commit_ts.readCurrentTsAcquire();

#ifdef CONFLICT_DETECTION_EAGER
	if(ValidateWithReadLocks()) {
#else
	if(Validate()) {
#endif /* CONFLICT_DETECTION_EAGER */
		valid_ts = ts;
#ifdef PRIVATIZATION_QUIESCENCE
		*quiescence_ts = ts;
#elif defined PRIVATIZATION_QUIESCENCE_TREE
		privatization_tree.setNonMinimumTs(ts);
#endif /* PRIVATIZATION_QUIESCENCE */
		
#ifdef TS_EXTEND_STATS
		stats.IncrementStatistics(StatisticsType::EXTEND_SUCCESS);
#endif /* TS_EXTEND_STATS */
		return true;
	}
	
#ifdef TS_EXTEND_STATS
	stats.IncrementStatistics(StatisticsType::EXTEND_FAILURE);
#endif /* TS_EXTEND_STATS */

	return false;
}

// this function knows maping from addresses to locks
inline void wlpdstm::TxMixinv::LockMemoryBlock(void *address, size_t size) {
	uintptr_t start = (uintptr_t)address;
	uintptr_t end = start + size;
	VersionLock *old = NULL;
	VersionLock *curr;
	
	for(uintptr_t address = start;address < end;address++) {
		curr = map_address_to_write_lock((Word *)address);
		
		if(curr != old) {
			LockMemoryStripe(curr, (Word *)address);
			old = curr;
		}
	}
}

inline void *wlpdstm::TxMixinv::TxMalloc(size_t size) {
	profiling.TxMallocStart(size);
	void *ret = mm.TxMalloc(size);
	profiling.TxMallocEnd(ret);
	return ret;
}

inline void wlpdstm::TxMixinv::TxFree(void *ptr, size_t size) {
	profiling.TxFreeStart(ptr, size);
#ifdef DETAILED_STATS
	stats.IncrementStatistics(StatisticsType::MEMORY_DEALLOC_COUNT);
	stats.IncrementStatistics(StatisticsType::MEMORY_DEALLOC_SIZE, size);
#endif /* DETAILED_STATS */
	LockMemoryBlock(ptr, size);
	mm.TxFree(ptr);
	profiling.TxFreeEnd();
}

inline bool wlpdstm::TxMixinv::LockedByMe(WriteLogEntry *log_entry) {
	// this is much faster than going through the log or checking address ranges
	return log_entry != NULL && log_entry->owner == this;
}

inline void wlpdstm::TxMixinv::TxRestart() {
	profiling.TxRestartStart();
#ifdef PRIVATIZATION_QUIESCENCE
	*quiescence_ts = MINIMUM_TS;
#elif defined PRIVATIZATION_QUIESCENCE_TREE
	privatization_tree.setNonMinimumTs(MINIMUM_TS);
#endif /* PRIVATIZATION_QUIESCENCE */
	
	RollbackRunning();
	atomic_store_release(&tx_status, (Word)TX_RESTARTED);
	
#ifdef PERFORMANCE_COUNTING
	if(perf_cnt_sampling.should_sample()) {
		perf_cnt.TxEnd();
		stats.IncrementStatistics(StatisticsType::CYCLES, perf_cnt.GetElapsedCycles());
		stats.IncrementStatistics(StatisticsType::RETIRED_INSTRUCTIONS, perf_cnt.GetRetiredInstructions());
		stats.IncrementStatistics(StatisticsType::CACHE_MISSES, perf_cnt.GetCacheMisses());
	}
#endif /* PERFORMANCE_COUNTING */
	stats.IncrementStatistics(StatisticsType::ABORT);
	profiling.TxRestartEnd();
	stats.TxRestart();
	RestartJump();
}

inline void wlpdstm::TxMixinv::RestartJump() {
#ifdef WLPDSTM_ICC
	jmp_to_begin_transaction(&start_buf);
#else
	siglongjmp(start_buf, LONG_JMP_RESTART_FLAG);
#endif /* WLPDSTM_ICC */
}

inline void wlpdstm::TxMixinv::TxAbort() {
	profiling.TxAbortStart();
#ifdef PRIVATIZATION_QUIESCENCE
	*quiescence_ts = MINIMUM_TS;
#elif defined PRIVATIZATION_QUIESCENCE_TREE
	privatization_tree.setNonMinimumTs(MINIMUM_TS);
#endif /* PRIVATIZATION_QUIESCENCE */
	
	RollbackRunning();
	atomic_store_release(&tx_status, (Word)TX_RESTARTED);
	
#ifdef PERFORMANCE_COUNTING
	if(perf_cnt_sampling.should_sample()) {
		perf_cnt.TxEnd();
		stats.IncrementStatistics(StatisticsType::CYCLES, perf_cnt.GetElapsedCycles());
		stats.IncrementStatistics(StatisticsType::RETIRED_INSTRUCTIONS, perf_cnt.GetRetiredInstructions());
		stats.IncrementStatistics(StatisticsType::CACHE_MISSES, perf_cnt.GetCacheMisses());
	}
#endif /* PERFORMANCE_COUNTING */
	profiling.TxAbortEnd();
	stats.TxAbort();
	AbortJump();
}

inline void wlpdstm::TxMixinv::AbortJump() {
#ifdef WLPDSTM_ICC
	jmp_to_begin_transaction(&start_buf);
#else	
	siglongjmp(start_buf, LONG_JMP_ABORT_FLAG);
#endif /* WLPDSTM_ICC */
}


inline bool wlpdstm::TxMixinv::ShouldAbortWrite(WriteLock *write_lock) {
	if(aborted_externally) {
		return true;
	}

	if(greedy_ts == MINIMUM_TS) {
		return true;
	}
	
	WriteLock lock_value = (WriteLock)atomic_load_no_barrier(write_lock);
	
	if(is_write_locked(lock_value)) {
		WriteLogEntry *log_entry = (WriteLogEntry *)lock_value;
		TxMixinv *owner = log_entry->owner;
		
		if(CMStrongerThan(owner)) {
			if(!owner->aborted_externally) {
				owner->aborted_externally = true;
#ifdef DETAILED_STATS
				stats.IncrementStatistics(StatisticsType::CM_DECIDE);
#endif /* DETAILED_STATS */
			}
			
			return false;
		}
		
		return true;
	}
	
	return false;
}

inline bool wlpdstm::TxMixinv::CMStrongerThan(TxMixinv *other) {
	if(greedy_ts == MINIMUM_TS) {
		return false;
	}
	
	unsigned other_ts = other->greedy_ts;
	return other_ts == MINIMUM_TS || greedy_ts < other_ts;
}

inline void wlpdstm::TxMixinv::CmStartTx() {
#ifdef SIMPLE_GREEDY
	if(succ_aborts == 0) {
		cm_phase = CM_PHASE_GREEDY;
		greedy_ts = cm_ts.getNextTs() + 1;
	}
#else /* two phase greedy */
	cm_phase = CM_PHASE_INITIAL;
	greedy_ts = MINIMUM_TS;
	locations_accessed = 0;
#endif /* SIMPLE_GREEDY */
}

inline void wlpdstm::TxMixinv::CmOnAccess() {
#ifndef SIMPLE_GREEDY
	if(cm_phase == CM_PHASE_INITIAL) {
		if(++locations_accessed > CM_ACCESS_THRESHOLD) {
#ifdef DETAILED_STATS
			stats.IncrementStatistics(StatisticsType::SWITCH_TO_SECOND_CM_PHASE);
#endif /* DETAILED_STATS */
			cm_phase = CM_PHASE_GREEDY;
			greedy_ts = cm_ts.getNextTs() + 1;
		}
	}
#endif /* SIMPLE_GREEDY */
}

#ifdef WAIT_ON_SUCC_ABORTS
inline void wlpdstm::TxMixinv::WaitOnAbort() {
	uint64_t cycles_to_wait = random.Get() % (succ_aborts * WAIT_CYCLES_MULTIPLICATOR);
	wait_cycles(cycles_to_wait);
#ifdef DETAILED_STATS
	stats.IncrementStatistics(StatisticsType::WAIT_ON_ABORT);
#endif /* DETAILED_STATS */
}
#endif /* WAIT_ON_SUCC_ABORTS */

#ifdef MM_EPOCH
inline void wlpdstm::TxMixinv::InitLastObservedTs() {
	last_observed_ts = MINIMUM_TS;
	start_count = 0;	
}

inline void wlpdstm::TxMixinv::UpdateLastObservedTs(Word ts) {
	if(++start_count >= UPDATE_LOCAL_LAST_OBSERVED_TS_FREQUENCY) {
		last_observed_ts = ts - 1;
		start_count = 0;
	}
}

inline Word wlpdstm::TxMixinv::UpdateMinimumObservedTs() {
	Word ret = MINIMUM_TS;
	
	if(minimum_observed_ts_lock.try_lock()) {
		if(transactions[0]) {
			unsigned minimum_ts = transactions[0]->last_observed_ts;
			
			for(unsigned i = 1;i < thread_count;i++) {
				if(transactions[i] != NULL) {
					unsigned ts = transactions[i]->last_observed_ts;
					
					if(ts < minimum_ts) {
						minimum_ts = ts;
					}
				}
			}
			
			minimum_observed_ts.val = minimum_ts;
			ret = minimum_ts;
		}
		
		minimum_observed_ts_lock.release();
	} else {
		ret = minimum_observed_ts.val;
	}
	
	return ret;
}

inline Word wlpdstm::TxMixinv::GetMinimumObservedTs() {
	return minimum_observed_ts.val;
}
#endif /* MM_EPOCH */

inline void wlpdstm::TxMixinv::Register() {
	// add itself to the transaction array
	transactions[tid.Get()] = this;

#ifdef PRIVATIZATION_QUIESCENCE
	quiescence_ts = (Word *)quiescence_timestamp_array + tid.Get();
#endif /* PRIVATIZATION_QUIESCENCE */

#ifdef SIGNALING
	signals = &(signaling_array[tid.Get()].val);
#endif /* SIGNALING */
	
	// update thread count
	Word my_count = tid.Get() + 1;
	Word curr_count = atomic_load_acquire(&thread_count);
	
	while(my_count > curr_count) {
		if(atomic_cas_full(&thread_count, curr_count, my_count)) {
			break;
		}
		
		curr_count = atomic_load_acquire(&thread_count);
	}
}

inline void wlpdstm::TxMixinv::YieldCPU() {
#ifdef USE_PREEMPTIVE_WAITING
	pre_yield();
#endif	
}

#ifdef STACK_PROTECT

#if defined STACK_PROTECT_TANGER_BOUND || defined STACK_PROTECT_ICC_BOUND
inline void wlpdstm::TxMixinv::SetStackHigh(uintptr_t addr) {
	stack_high = addr;
}
#elif defined STACK_PROTECT_WLPDSTM_BOUND
// This is safe to do because start_tx should have its own stack.
inline void wlpdstm::TxMixinv::SetStackHigh() {
	stack_high = read_bp();
}
#endif /* STACK_PROTECT_TANGER_BOUND */

inline bool wlpdstm::TxMixinv::OnStack(uintptr_t addr) {
	return OnStack(addr, read_sp());
}

inline bool wlpdstm::TxMixinv::OnStack(uintptr_t addr, uintptr_t current_sp) {
	if(addr < current_sp) {
		return false;
	}
	
	if(addr > stack_high) {
		return false;
	}
	
	return true;
}
#endif /* STACK_PROTECT */

inline void wlpdstm::TxMixinv::PrintStatistics() {
#ifdef COLLECT_STATS
	FILE *out_file = stdout;
	fprintf(out_file, "\n");
	fprintf(out_file, "STM internal statistics: \n");
	fprintf(out_file, "========================\n");
	
	// collect stats in a single collection
	ThreadStatisticsCollection stat_collection;
	
	for(unsigned i = 0;i < thread_count;i++) {
		// these should all be initialized at this point
		stat_collection.Add(&(transactions[i]->stats));
		fprintf(out_file, "Thread %d: \n", i + 1);
		transactions[i]->stats.Print(out_file, 1);
		fprintf(out_file, "\n");
	}

	fprintf(out_file, "Total stats: \n");
	ThreadStatistics total_stats = stat_collection.MergeAll();
	total_stats.Print(out_file, 1);
	fprintf(out_file, "\nConfiguration:\n");
	fprintf(out_file, "\tLockExtent: %d\n", LOCK_EXTENT);
#endif /* COLLECT_STATS */
}

inline bool wlpdstm::TxMixinv::StartSynchronization() {
	if(!atomic_cas_acquire(&synchronization_in_progress.val, false, true)) {
		Synchronize();
		return false;
	}
	
	for(unsigned i = 0;i < thread_count;i++) {
		//while(atomic_load_acquire(&transactions[i]->executing)) {
		while(transactions[i]->IsExecuting()) {
			// do nothing
		}
	}
	
	return true;
}

inline void wlpdstm::TxMixinv::EndSynchronization() {
	atomic_store_release(&synchronization_in_progress.val, false);
}

inline bool wlpdstm::TxMixinv::Synchronize() {
	bool ret = false;
	
	if(atomic_load_acquire(&synchronization_in_progress.val)) {
		tx_status = TX_IDLE;
		
		while(atomic_load_acquire(&synchronization_in_progress.val)) {
			// do nothing
		}
		
		ret = true;
	}
	
	return ret;
}

inline void wlpdstm::TxMixinv::RestartCommitTS() {
	commit_ts.restart();
	InitializeReadLocks();
}

inline void wlpdstm::TxMixinv::StartThreadProfiling() {
	// empty
}

inline void wlpdstm::TxMixinv::EndThreadProfiling() {
	// empty
}

#ifdef PRIVATIZATION_QUIESCENCE

inline void wlpdstm::TxMixinv::PrivatizationQuiescenceWait(Word ts) {
	unsigned tc = thread_count;

	for(unsigned i = 0;i < tc;i++) {
#ifdef SIGNALING
		if(quiescence_timestamp_array[i] != MINIMUM_TS && quiescence_timestamp_array[i] < ts) { 
			SetSignal(i, SIGNAL_PRIVATIZATION_VALIDATE);
		} else {
			continue;
		}
#endif /* SIGNALING */

		while(quiescence_timestamp_array[i] != MINIMUM_TS && quiescence_timestamp_array[i] < ts) {
			// do nothing
		}
	}
}

#endif /* PRIVATIZATION_QUIESCENCE */		

#ifdef SIGNALING
inline void wlpdstm::TxMixinv::SetSignal(unsigned tid, TxSignals signal) {
	signaling_array[tid].val |= (Word)signal;
}
	
inline void wlpdstm::TxMixinv::ClearSignals() {
	*signals = SIGNAL_EMPTY;
}

inline bool wlpdstm::TxMixinv::IsSignalSet(TxSignals signal) {
	return (*signals) & signal;
}

#endif /* SIGNALING */
