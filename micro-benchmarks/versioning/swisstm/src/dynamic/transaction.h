/**
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 */

// TODO: check whether quiescence ts update can be moved to rollback functions

#ifndef WLPDSTM_TRANSACTION_DYNAMIC_H_
#define WLPDSTM_TRANSACTION_DYNAMIC_H_

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "../common/cache_aligned_alloc.h"
#include "../common/word.h"
#include "../common/tid.h"
#include "../common/tls.h"
#include "../common/cache_aligned_alloc.h"
#include "../common/jmp.h"
#include "../common/timing.h"
#include "../common/random.h"
#include "../common/padded.h"
#include "../common/tm_impl_const.h"

#include "log.h"
#include "version_lock.h"
#include "constants.h"
#include "timestamp.h"
#include "memory.h"
#include "stats.h"
#include "profiling.h"

#ifdef USE_PREEMPTIVE_WAITING
#include "../common/preemptive_utils.h"
#endif // USE_PREEMPTIVE_WAITING

#ifdef STACK_PROTECT
#include "../common/arch.h"
#endif /* STACK_PROTECT */

#ifdef PERFORMANCE_COUNTING
#include "perf_cnt.h"
#include "../common/sampling.h"
#endif /* PERFORMANCE_COUNTING */

// different STM implementations
#include "mixed/mixed.h"
#include "eager/eager.h"
#include "lazy/lazy.h"
#include "lazy/hashtable.h"

#ifdef GREEN_CM
#include "../common/lock.h"
#include "../common/energy.h"
#endif /* GREEN_CM */

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

	enum TryCommitResult {
		COMMIT = 0,
		JUMP_RESTART,
		RESTART_RUNNING,
		RESTART_COMMITTING
	};

#ifndef GREEN_CM
	// possible CM phases
	enum CmPhase {
		CM_PHASE_INITIAL,
		CM_PHASE_GREEDY
	};
#endif /* GREEN_CM */

	// possible statuses of aborted transactions
	enum TxStatus {
		TX_IDLE,
		TX_EXECUTING,
		TX_ABORTED,
		TX_RESTARTED,
		TX_COMMITTED
	};

#ifndef GREEN_CM
#ifdef WAIT_ON_SUCC_ABORTS
	static const unsigned SUCC_ABORTS_THRESHOLD = 1;
	static const unsigned SUCC_ABORTS_MAX = 10;
	static const unsigned WAIT_CYCLES_MULTIPLICATOR = 8000;
#endif /* WAIT_ON_SUCC_ABORTS */
#endif /* GREEN_CM */

#ifdef GREEN_CM
	enum GreenCMDecision {
		GREEN_CM_NO_DECISION = 0,
		GREEN_CM_CONTINUE = 1,
		GREEN_CM_RESTART = 2
	};

	struct GreenCMQueueItem {
		volatile GreenCMQueueItem *head;
		cas_lock lock;

		uint64_t consumed_energy;
		volatile Word decision;
		GreenCMQueueItem *next;
	};
#endif /* GREEN_CM */

#ifdef WLPDSTM_TX_PROFILING_ADAPTIVE_DYNAMIC
	static const unsigned TM_IMPLEMENTATION_VARIANT_COUNT = TM_IMPLEMENTATION_COUNT * 2;
#else
	static const unsigned TM_IMPLEMENTATION_VARIANT_COUNT = TM_IMPLEMENTATION_COUNT;
#endif /* WLPDSTM_TX_PROFILING_ADAPTIVE_DYNAMIC */

	class TransactionDynamic : public CacheAlignedAlloc {
		friend class TxMixed;
		friend class TxEager;
		friend class TxLazy;

			//////////////////////////////////
			// function pointer types start //
			//////////////////////////////////

			typedef void (*SetFunPtr)(TransactionDynamic *desc);
		
			typedef void (*TxCommitFunPtr)(TransactionDynamic *desc);
		
			typedef void (*RollbackRunningFunPtr)(TransactionDynamic *desc);
		
			typedef Word (*ReadWordFunPtr)(TransactionDynamic *desc, Word *addr);
		
			typedef WriteLogEntry *(*LockMemoryStripeFunPtr)(TransactionDynamic *desc, WriteLock *write_lock, Word *address);

			////////////////////////////////
			// function pointer types end //
			////////////////////////////////

		public:
			////////////////////////////
			// public interface start //
			////////////////////////////

			static void GlobalInit();
			void ThreadInit();

			static void GlobalShutdown();
			void ThreadShutdown();

			void TxStart(int lex_tx_id = NO_LEXICAL_TX);

			void ChooseTM(TmImplementation tm);

			void TxCommit();
			
			void TxAbort();
			
			void TxRestart();		

			// TODO: add mask here to optimize for lazy stm variant
			//       this mask can safely be ignored by all except lazy
			Word ReadWord(Word *addr);
			
			void WriteWord(Word *address, Word val, Word mask = LOG_ENTRY_UNMASKED);
			
			void *TxMalloc(size_t size);
			
			void TxFree(void *ptr, size_t size);
			
			void LockMemoryBlock(void *address, size_t size);

			bool IsExecuting() const;
			
			TxStatus GetTxStatus() const;

			int GetTransactionId() const;

			int GetThreadId();

			// profiling functions
			void StartThreadProfiling();
			void EndThreadProfiling();

			//////////////////////////
			// public interface end //
			//////////////////////////

			/////////////////////
			// functions start //
			/////////////////////

		protected:
			void TxStartInline(int lex_tx_id = NO_LEXICAL_TX);

			static void InitializeReadLocks();
			static void InitializeWriteLocks();
		
#ifdef PRIVATIZATION_QUIESCENCE		
			static void InitializeQuiescenceTimestamps();
#endif /* PRIVATIZATION_QUIESCENCE */

			void RestartJump();
			void AbortJump();

			static unsigned map_address_to_index(Word *address, unsigned le);
			static unsigned map_address_to_index(Word *address);
			static VersionLock *map_address_to_read_lock(Word *address);
			static WriteLock *map_address_to_write_lock(Word *address);
			static WriteLock *map_read_lock_to_write_lock(VersionLock *lock_address);
			static VersionLock *map_write_lock_to_read_lock(WriteLock *lock_address);

			void WriteWordInner(Word *address, Word val, Word mask = LOG_ENTRY_UNMASKED);

			bool StartSynchronization();		
			void EndSynchronization();
			bool Synchronize();
			void RestartCommitTS();

			static Word IncrementCommitTs();

			static void PrintStatistics();

			void Register();
			
			void YieldCPU();

			// invoke functions
			void InvokeTxCommit(TransactionDynamic *desc);
			void InvokeRollbackRunning(TransactionDynamic *desc);
			Word InvokeReadWord(TransactionDynamic *desc, Word *addr);
			WriteLogEntry *InvokeLockMemoryStripe(TransactionDynamic *desc, WriteLock *write_lock, Word *address);		

#ifdef MM_EPOCH
			void InitLastObservedTs();
			
			void UpdateLastObservedTs(Word ts);
		
		public:
			static Word UpdateMinimumObservedTs();
			
			static Word GetMinimumObservedTs();
#endif /* MM_EPOCH */
		
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

			///////////////////////////
			// contention management //
			///////////////////////////
			bool ShouldAbortWrite(WriteLock *write_lock);
			void CmStartTx();

#ifdef GREEN_CM
			GreenCMDecision ProcessWaitingForMe(uint64_t e);
			void ContinueWaitingForMe();
#else
			// TODO: isolate CM in a separate class
			bool CMStrongerThan(TransactionDynamic *other);

			void CmOnAccess();
			
#ifdef WAIT_ON_SUCC_ABORTS
			void WaitOnAbort();
#endif /* WAIT_ON_SUCC_ABORTS */
#endif /* GREEN_CM */

			void ThreadInitFunctions();

			// only check for version in read locks
			bool Validate();

			// if version not in read lock, check whether it was
			// locked by this transaction too
			bool ValidateWithReadLocks();

			// if version not in read lock, check whether it was
			// locked by this transaction too and make sure that the version
			// is equal the one read in the current transaction
			bool ValidateWithReadLockVersions();
		
			bool ShouldExtend(VersionLock version);
		
			bool LockedByMe(WriteLogEntry *log_entry);

#ifdef DYNAMIC_DYNAMIC
			static void InitializeFunPtrTable();
			void ResetFunPtrOnStart();
			unsigned GetImplementationVariant(unsigned tm_implementation);
			static unsigned GetProfilingVariant(unsigned tm_implementation);
#endif /* DYNAMIC_DYNAMIC */

			///////////////////
			// functions end //
			///////////////////
		
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

#ifdef GREEN_CM
			CACHE_LINE_ALIGNED union {
				volatile GreenCMQueueItem green_cm_item;
				char padding_green_cm_item[CACHE_LINE_SIZE_BYTES];
			};
#endif /* GREEN_CM */

			// tx local data next
			// its alignment is not important, as it is not shared
			
			Word valid_ts;

			TxCommitFunPtr tx_commit_fun;
			RollbackRunningFunPtr rollback_running_fun;
			ReadWordFunPtr read_word_fun;
			LockMemoryStripeFunPtr lock_memory_stripe_fun;
			
#ifdef PRIVATIZATION_QUIESCENCE
			Word *quiescence_ts;
#endif /* PRIVATIZATION_QUIESCENCE */
			
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

			// local
			// only needed for lazy scheme
			WriteLogHashtable write_log_hashtable;

			// contention management
#ifdef GREEN_CM
			Energy thread_energy;
			Energy tx_energy;
#else
			// local
			unsigned locations_accessed;
			
			// local
			CmPhase cm_phase;
			
			// local
			unsigned succ_aborts;
#endif /* GREEN_CM */
			
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

			// local
			unsigned current_tm;
			unsigned new_tm;
			
#ifdef PERFORMANCE_COUNTING
		protected:
			Sampling perf_cnt_sampling;
			PerfCnt perf_cnt;		
#endif /* PERFORMANCE_COUNTING */

			//////////////////////
			// static variables //
			//////////////////////
			
		protected:
			static VersionLock version_lock_table[FULL_VERSION_LOCK_TABLE_SIZE];
			
			static GlobalTimestamp commit_ts;

#ifndef GREEN_CM
			static GlobalTimestamp cm_ts;
#endif /* GREEN_CM */
			
			static PaddedWord minimum_observed_ts;
			
			static PaddedSpinTryLock minimum_observed_ts_lock;
			
			static TransactionDynamic *transactions[MAX_THREADS];
			
			static Word thread_count;
			
			static PaddedBool synchronization_in_progress;
			
#ifdef PRIVATIZATION_QUIESCENCE
			static volatile Word quiescence_timestamp_array[MAX_THREADS];
#endif /* PRIVATIZATION_QUIESCENCE */

#ifdef DYNAMIC_DYNAMIC
			static SetFunPtr set_fun_ptr_table[TM_IMPLEMENTATION_VARIANT_COUNT];
#endif /* DYNAMIC_DYNAMIC */

	}  __attribute__ ((aligned(CACHE_LINE_SIZE_BYTES)));

	typedef TransactionDynamic TransactionImpl;
}

#define LOCK_EXTENT DEFAULT_LOCK_EXTENT_SIZE

////////////////////////////
// public interface start //
////////////////////////////

inline void wlpdstm::TransactionDynamic::TxStart(int lex_tx_id) {
	TxStartInline(lex_tx_id);

#ifdef DYNAMIC_DYNAMIC
	unsigned tm_variant = GetImplementationVariant(new_tm);

	if(tm_variant != current_tm) {
		set_fun_ptr_table[tm_variant](this);
		current_tm = tm_variant;
	}
#endif /* DYNAMIC_DYNAMIC */
}

inline void wlpdstm::TransactionDynamic::ChooseTM(TmImplementation tm) {
#ifdef DYNAMIC_SWITCH
	new_tm = tm;
#endif /* DYNAMIC_SWITCH */
}

inline void wlpdstm::TransactionDynamic::TxStartInline(int lex_tx_id) {
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
	*quiescence_ts = desc->valid_ts;
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
	profiling.TxStartEnd();	
}

inline void wlpdstm::TransactionDynamic::TxCommit() {
	profiling.TxCommitStart();
	InvokeTxCommit(this);
	profiling.TxCommitEnd();
}

inline void wlpdstm::TransactionDynamic::TxAbort() {
	profiling.TxAbortStart();
#ifdef PRIVATIZATION_QUIESCENCE
	*quiescence_ts = MINIMUM_TS;
#endif /* PRIVATIZATION_QUIESCENCE */

	InvokeRollbackRunning(this);
	atomic_store_release(&tx_status, (Word)TX_RESTARTED);
	
#ifdef PERFORMANCE_COUNTING
	if(perf_cnt_sampling.should_sample()) {
		perf_cnt.TxEnd();
		stats.IncrementStatistics(StatisticsType::CYCLES, perf_cnt.GetElapsedCycles());
		stats.IncrementStatistics(StatisticsType::RETIRED_INSTRUCTIONS, perf_cnt.GetRetiredInstructions());
		stats.IncrementStatistics(StatisticsType::CACHE_MISSES, perf_cnt.GetCacheMisses());
	}
#endif /* PERFORMANCE_COUNTING */
	stats.IncrementStatistics(StatisticsType::USER_ABORT);
	profiling.TxAbortEnd();
	stats.TxAbort();
	AbortJump();		
}

inline void wlpdstm::TransactionDynamic::TxRestart() {
	profiling.TxRestartStart();
#ifdef PRIVATIZATION_QUIESCENCE
	*quiescence_ts = MINIMUM_TS;
#endif /* PRIVATIZATION_QUIESCENCE */

	InvokeRollbackRunning(this);
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

inline Word wlpdstm::TransactionDynamic::ReadWord(Word *addr) {
#ifndef WLPDSTM_TX_PROFILING_ADAPTIVE_DYNAMIC
	profiling.ReadWordStart(addr);
#endif /* WLPDSTM_TX_PROFILING_ADAPTIVE_DYNAMIC */
	Word ret;
#ifdef STACK_PROTECT_ON_READ
	if(OnStack((uintptr_t)address)) {
		ret = *address;
	} else {
		ret = InvokeReadWord(this, addr);
	}
#else
	ret = InvokeReadWord(this, addr);
#endif /* STACK_PROTECT_ON_READ */
#ifndef WLPDSTM_TX_PROFILING_ADAPTIVE_DYNAMIC
	profiling.ReadWordEnd(ret);
#endif /* WLPDSTM_TX_PROFILING_ADAPTIVE_DYNAMIC */
	return ret;
}

inline void wlpdstm::TransactionDynamic::WriteWord(Word *address, Word value, Word mask) {
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

inline void *wlpdstm::TransactionDynamic::TxMalloc(size_t size) {
	profiling.TxMallocStart(size);
	void *ret = mm.TxMalloc(size);
	profiling.TxMallocEnd(ret);
	return ret;
}

inline void wlpdstm::TransactionDynamic::TxFree(void *ptr, size_t size) {
	profiling.TxFreeStart(ptr, size);
#ifdef DETAILED_STATS
	stats.IncrementStatistics(StatisticsType::MEMORY_DEALLOC_COUNT);
	stats.IncrementStatistics(StatisticsType::MEMORY_DEALLOC_SIZE, size);
#endif /* DETAILED_STATS */
	LockMemoryBlock(ptr, size);
	mm.TxFree(ptr);	
	profiling.TxFreeEnd();
}

inline void wlpdstm::TransactionDynamic::LockMemoryBlock(void *address, size_t size) {
	uintptr_t start = (uintptr_t)address;
	uintptr_t end = start + size;
	VersionLock *old = NULL;
	VersionLock *curr;
	
	for(uintptr_t address = start;address < end;address++) {
		curr = map_address_to_write_lock((Word *)address);
		
		if(curr != old) {
			InvokeLockMemoryStripe(this, curr, (Word *)address);
			old = curr;
		}
	}
}

//////////////////////////
// public interface end //
//////////////////////////

//////////////////////////
// initialization start //
//////////////////////////

inline void wlpdstm::TransactionDynamic::GlobalInit() {
	InitializeReadLocks();
	
	InitializeWriteLocks();
	
#ifdef PRIVATIZATION_QUIESCENCE
	InitializeQuiescenceTimestamps();
#endif /* PRIVATIZATION_QUIESCENCE */
	
	// initialize shared data
	minimum_observed_ts.val = MINIMUM_TS;
	
	// initialize memory manager
	MemoryManager::GlobalInit();
	
	synchronization_in_progress.val = false;

#ifdef DYNAMIC_DYNAMIC
	InitializeFunPtrTable();
#endif /* DYNAMIC_DYNAMIC */
	
#ifdef PERFORMANCE_COUNTING
	PerfCnt::GlobalInit();
#endif /* PERFORMANCE_COUNTING */
	
	Profiling::GlobalInit();
}

inline void wlpdstm::TransactionDynamic::InitializeReadLocks() {
	VersionLock initial_version = get_version_lock(MINIMUM_TS + 1);
	
	for(int i = 0;i < FULL_VERSION_LOCK_TABLE_SIZE;i += 2) {
		version_lock_table[i] = initial_version;
	}	
}

inline void wlpdstm::TransactionDynamic::InitializeWriteLocks() {
	for(int i = 1;i < FULL_VERSION_LOCK_TABLE_SIZE;i += 2) {
		version_lock_table[i] = WRITE_LOCK_CLEAR;
	}
}

#ifdef PRIVATIZATION_QUIESCENCE
inline void wlpdstm::TransactionDynamic::InitializeQuiescenceTimestamps() {
	for(unsigned i = 0;i < MAX_THREADS;i++) {
		quiescence_timestamp_array[i] = MINIMUM_TS;
	}	
}
#endif /* PRIVATIZATION_QUIESCENCE */

inline void wlpdstm::TransactionDynamic::ThreadInit() {
	stats.IncrementStatistics(StatisticsType::THREAD_COUNT);

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

#ifndef GREEN_CM
	succ_aborts = 0;
#endif /* GREEN_CM */
	
	assert((uintptr_t)this == (uintptr_t)(&start_buf));
	
#ifdef PERFORMANCE_COUNTING
	perf_cnt.ThreadInit();
#endif /* PERFORMANCE_COUNTING */
	
	profiling.ThreadInit(&stats);

	write_log_hashtable.ThreadInit();
	ThreadInitFunctions();

#ifdef GREEN_CM
	green_cm_item.head = NULL;
	green_cm_item.lock.init();
	thread_energy.start_measurement();
#endif /* GREEN_CM */
}

inline void wlpdstm::TransactionDynamic::GlobalShutdown() {
	PrintStatistics();
}

inline void wlpdstm::TransactionDynamic::ThreadShutdown() {
	profiling.ThreadShutdown();
#ifdef GREEN_CM
	stats.IncrementStatistics(StatisticsType::CONSUMED_ENERGY, thread_energy.get_consumed_energy());
#endif /* GREEN_CM */
}

inline wlpdstm::TxStatus wlpdstm::TransactionDynamic::GetTxStatus() const {
	return (TxStatus)tx_status;
}

inline bool wlpdstm::TransactionDynamic::IsExecuting() const {
	return tx_status == TX_EXECUTING;
}

inline int wlpdstm::TransactionDynamic::GetTransactionId() const {
	return stats.lexical_tx_id;
}

inline int wlpdstm::TransactionDynamic::GetThreadId() {
	return tid.Get();
}

////////////////////////
// initialization end //
////////////////////////

////////////////////////////
// mapping to locks start //
////////////////////////////

inline unsigned wlpdstm::TransactionDynamic::map_address_to_index(Word *address) {
	return map_address_to_index(address, LOCK_EXTENT) << 1;
}

inline unsigned wlpdstm::TransactionDynamic::map_address_to_index(Word *address, unsigned le) {
	return (((uintptr_t)address >> (le)) & (VERSION_LOCK_TABLE_SIZE - 1));
}

inline wlpdstm::VersionLock *wlpdstm::TransactionDynamic::map_address_to_read_lock(Word *address) {
	return version_lock_table + map_address_to_index(address);
}

inline wlpdstm::WriteLock *wlpdstm::TransactionDynamic::map_address_to_write_lock(Word *address) {
	return map_address_to_read_lock(address) + 1;
}

inline wlpdstm::WriteLock *wlpdstm::TransactionDynamic::map_read_lock_to_write_lock(VersionLock *lock_address) {
	return lock_address + 1;
}

inline wlpdstm::VersionLock *wlpdstm::TransactionDynamic::map_write_lock_to_read_lock(WriteLock *lock_address) {
	return lock_address - 1;
}

//////////////////////////
// mapping to locks end //
//////////////////////////

inline void wlpdstm::TransactionDynamic::WriteWordInner(Word *address, Word value, Word mask) {
	// map address to the lock
	VersionLock *write_lock = map_address_to_write_lock(address);
	
	// try to lock the address - it will abort if address cannot be locked
	WriteLogEntry *log_entry = InvokeLockMemoryStripe(this, write_lock, address);
	
	// insert (address, value) pair into the log
	log_entry->InsertWordLogEntry(write_log.write_word_log_mem_pool, address, value, mask);	
}

inline Word wlpdstm::TransactionDynamic::IncrementCommitTs() {
#ifdef COMMIT_TS_INC
	return commit_ts.getNextTsRelease() + 1;
#elif defined COMMIT_TS_GV4
	return commit_ts.GenerateTsGV4();
#endif /* commit_ts */
}

inline void wlpdstm::TransactionDynamic::RestartJump() {
#ifdef WLPDSTM_ICC
	jmp_to_begin_transaction(&start_buf);
#else
	siglongjmp(start_buf, LONG_JMP_RESTART_FLAG);
#endif /* WLPDSTM_ICC */
}

inline void wlpdstm::TransactionDynamic::AbortJump() {
#ifdef WLPDSTM_ICC
	jmp_to_begin_transaction(&start_buf);
#else	
	siglongjmp(start_buf, LONG_JMP_ABORT_FLAG);
#endif /* WLPDSTM_ICC */
}

#ifndef GREEN_CM
inline bool wlpdstm::TransactionDynamic::ShouldAbortWrite(WriteLock *write_lock) {
	if(aborted_externally) {
		return true;
	}
	
	if(greedy_ts == MINIMUM_TS) {
		return true;
	}
	
	WriteLock lock_value = (WriteLock)atomic_load_no_barrier(write_lock);
	
	if(is_write_locked(lock_value)) {
		WriteLogEntry *log_entry = (WriteLogEntry *)lock_value;
		TransactionDynamic *owner = log_entry->owner;
		
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

inline bool wlpdstm::TransactionDynamic::CMStrongerThan(TransactionDynamic *other) {
	if(greedy_ts == MINIMUM_TS) {
		return false;
	}
	
	unsigned other_ts = other->greedy_ts;
	return other_ts == MINIMUM_TS || greedy_ts < other_ts;
}

inline void wlpdstm::TransactionDynamic::CmStartTx() {
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

inline void wlpdstm::TransactionDynamic::CmOnAccess() {
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
inline void wlpdstm::TransactionDynamic::WaitOnAbort() {
	uint64_t cycles_to_wait = random.Get() % (succ_aborts * WAIT_CYCLES_MULTIPLICATOR);
	wait_cycles(cycles_to_wait);
#ifdef DETAILED_STATS
	stats.IncrementStatistics(StatisticsType::WAIT_ON_ABORT);
#endif /* DETAILED_STATS */
}
#endif /* WAIT_ON_SUCC_ABORTS */
#endif /* GREEN_CM */

#ifdef MM_EPOCH
inline void wlpdstm::TransactionDynamic::InitLastObservedTs() {
	last_observed_ts = MINIMUM_TS;
	start_count = 0;	
}

inline void wlpdstm::TransactionDynamic::UpdateLastObservedTs(Word ts) {
	if(++start_count >= UPDATE_LOCAL_LAST_OBSERVED_TS_FREQUENCY) {
		last_observed_ts = ts - 1;
		start_count = 0;
	}
}

inline Word wlpdstm::TransactionDynamic::UpdateMinimumObservedTs() {
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

inline Word wlpdstm::TransactionDynamic::GetMinimumObservedTs() {
	return minimum_observed_ts.val;
}
#endif /* MM_EPOCH */

inline void wlpdstm::TransactionDynamic::Register() {
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

inline void wlpdstm::TransactionDynamic::YieldCPU() {
#ifdef USE_PREEMPTIVE_WAITING
	pre_yield();
#endif	
}

#ifdef STACK_PROTECT

#if defined STACK_PROTECT_TANGER_BOUND || defined STACK_PROTECT_ICC_BOUND
inline void wlpdstm::TransactionDynamic::SetStackHigh(uintptr_t addr) {
	stack_high = addr;
}
#elif defined STACK_PROTECT_WLPDSTM_BOUND
// This is safe to do because start_tx should have its own stack.
inline void wlpdstm::TransactionDynamic::SetStackHigh() {
	stack_high = read_bp();
}
#endif /* STACK_PROTECT_TANGER_BOUND */

inline bool wlpdstm::TransactionDynamic::OnStack(uintptr_t addr) {
	return OnStack(addr, read_sp());
}

inline bool wlpdstm::TransactionDynamic::OnStack(uintptr_t addr, uintptr_t current_sp) {
	if(addr < current_sp) {
		return false;
	}
	
	if(addr > stack_high) {
		return false;
	}
	
	return true;
}
#endif /* STACK_PROTECT */

inline void wlpdstm::TransactionDynamic::PrintStatistics() {
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

inline bool wlpdstm::TransactionDynamic::StartSynchronization() {
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

inline void wlpdstm::TransactionDynamic::EndSynchronization() {
	atomic_store_release(&synchronization_in_progress.val, false);
}

inline bool wlpdstm::TransactionDynamic::Synchronize() {
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

inline void wlpdstm::TransactionDynamic::RestartCommitTS() {
	commit_ts.restart();
	InitializeReadLocks();
}

#ifdef PRIVATIZATION_QUIESCENCE
inline void wlpdstm::TransactionDynamic::PrivatizationQuiescenceWait(Word ts) {
	unsigned tc = thread_count;
	
	for(unsigned i = 0;i < tc;i++) {		
		while(quiescence_timestamp_array[i] != MINIMUM_TS && quiescence_timestamp_array[i] < ts) {
			// do nothing
		}
	}
}
#endif /* PRIVATIZATION_QUIESCENCE */		

inline bool wlpdstm::TransactionDynamic::ShouldExtend(VersionLock version) {
	if(get_value(version) > valid_ts) {
		return true;
	}
	
	return false;
}

inline bool wlpdstm::TransactionDynamic::Validate() {
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
inline bool wlpdstm::TransactionDynamic::ValidateWithReadLocks() {
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

// validate invoked at commit time which checks the version stored in the
// write log entry of the locked stripe (if it was locked by this transaction)
inline bool wlpdstm::TransactionDynamic::ValidateWithReadLockVersions() {
	ReadLog::iterator iter;
	
	for(iter = read_log.begin();iter.hasNext();iter.next()) {
		ReadLogEntry &entry = *iter;
		VersionLock currentVersion = (VersionLock)atomic_load_no_barrier(entry.read_lock);

		if(currentVersion != entry.version) {
			if(is_read_locked(currentVersion)) {
				WriteLock *write_lock = map_read_lock_to_write_lock(entry.read_lock);
				WriteLogEntry *log_entry = (WriteLogEntry *)atomic_load_no_barrier(write_lock);
				
				if(LockedByMe(log_entry) && log_entry->old_version == entry.version) {
					continue;
				}
			}

			return false;
		}
	}

	return true;
}

inline bool wlpdstm::TransactionDynamic::LockedByMe(WriteLogEntry *log_entry) {
	// this is much faster than going through the log or checking address ranges
	return log_entry != NULL && log_entry->owner == this;
}

#ifdef DYNAMIC_MIXED
#define TX_IMPLEMENTATION TxMixed
#elif defined DYNAMIC_EAGER
#define TX_IMPLEMENTATION TxEager
#elif defined DYNAMIC_LAZY
#define TX_IMPLEMENTATION TxLazy
#endif /* STM_ALGORITHM */

inline void wlpdstm::TransactionDynamic::ThreadInitFunctions() {
#ifdef DYNAMIC_DYNAMIC
#ifdef DYNAMIC_MIXED
	new_tm = TM_MIXED;
#elif defined DYNAMIC_EAGER
	new_tm = TM_EAGER;
#elif defined DYNAMIC_LAZY
	new_tm = TM_LAZY;
#elif defined DYNAMIC_SWITCH
	new_tm = TM_MIXED;
#endif /* STM_ALGORITHM */	
	current_tm = TM_IMPLEMENTATION_VARIANT_COUNT;
#elif defined DYNAMIC_STATIC
	tx_commit_fun = TX_IMPLEMENTATION::TxCommitStatic;
	rollback_running_fun = TX_IMPLEMENTATION::RollbackRunningStatic;
	read_word_fun = TX_IMPLEMENTATION::ReadWordStatic;
	lock_memory_stripe_fun = TX_IMPLEMENTATION::LockMemoryStripeStatic;
#endif /* DYNAMIC_DYNAMIC */
}

// invoke functions
inline void wlpdstm::TransactionDynamic::InvokeTxCommit(TransactionDynamic *desc) {
#ifdef DYNAMIC_DYNAMIC
	tx_commit_fun(desc);
#elif defined DYNAMIC_STATIC
	tx_commit_fun(desc);
#elif defined DYNAMIC_INLINE
	TX_IMPLEMENTATION::TxCommitStatic(desc);
#endif	
}

inline void wlpdstm::TransactionDynamic::InvokeRollbackRunning(TransactionDynamic *desc) {
#ifdef DYNAMIC_DYNAMIC
	rollback_running_fun(desc);
#elif defined DYNAMIC_STATIC
	rollback_running_fun(desc);
#elif defined DYNAMIC_INLINE
	TX_IMPLEMENTATION::RollbackRunningStatic(desc);
#endif	
}

inline Word wlpdstm::TransactionDynamic::InvokeReadWord(TransactionDynamic *desc, Word *addr) {
#ifdef DYNAMIC_DYNAMIC
	return read_word_fun(desc, addr);
#elif defined DYNAMIC_STATIC
	return read_word_fun(desc, addr);
#elif defined DYNAMIC_INLINE
	return TX_IMPLEMENTATION::ReadWordStatic(desc, addr);
#endif
	// for the compiler
	assert(0);
    return 0;
}

inline wlpdstm::WriteLogEntry *wlpdstm::TransactionDynamic::InvokeLockMemoryStripe(TransactionDynamic *desc, WriteLock *write_lock, Word *address) {
#ifdef DYNAMIC_DYNAMIC
	return lock_memory_stripe_fun(desc, write_lock, address);
#elif defined DYNAMIC_STATIC
	return lock_memory_stripe_fun(desc, write_lock, address);
#elif defined DYNAMIC_INLINE
	return TX_IMPLEMENTATION::LockMemoryStripeStatic(desc, write_lock, address);
#endif
	// for the compiler
	assert(0);
    return 0;
}

#ifdef DYNAMIC_DYNAMIC
inline void wlpdstm::TransactionDynamic::InitializeFunPtrTable() {
	set_fun_ptr_table[TM_MIXED] = TxMixed::TxStartSetFunPtr;
	set_fun_ptr_table[TM_LAZY] = TxLazy::TxStartSetFunPtr;
	set_fun_ptr_table[TM_EAGER] = TxEager::TxStartSetFunPtr;	
#ifdef WLPDSTM_TX_PROFILING_ADAPTIVE_DYNAMIC
	set_fun_ptr_table[GetProfilingVariant(TM_MIXED)] = TxMixed::TxStartSetFunPtrProfiled;
	set_fun_ptr_table[GetProfilingVariant(TM_LAZY)] = TxLazy::TxStartSetFunPtr;
	set_fun_ptr_table[GetProfilingVariant(TM_EAGER)] = TxEager::TxStartSetFunPtr;		
#endif /* WLPDSTM_TX_PROFILING_ADAPTIVE_DYNAMIC */
}

inline void wlpdstm::TransactionDynamic::ResetFunPtrOnStart() {
	current_tm = TM_IMPLEMENTATION_VARIANT_COUNT;
}

inline unsigned wlpdstm::TransactionDynamic::GetImplementationVariant(unsigned tm_implementation) {
#ifdef WLPDSTM_TX_PROFILING_ADAPTIVE_DYNAMIC
	if(profiling.ShouldProfile()) {
		return GetProfilingVariant(tm_implementation);
	} else {
		return tm_implementation;
	}
#else
	return tm_implementation;
#endif /* WLPDSTM_TX_PROFILING_ADAPTIVE_DYNAMIC */
}

inline unsigned wlpdstm::TransactionDynamic::GetProfilingVariant(unsigned tm_implementation) {
	return tm_implementation + TM_IMPLEMENTATION_COUNT;
}
#endif /* DYNAMIC_DYNAMIC */

inline void wlpdstm::TransactionDynamic::StartThreadProfiling() {
	profiling.ThreadStart();
}

inline void wlpdstm::TransactionDynamic::EndThreadProfiling() {
	profiling.ThreadEnd();
}

#ifdef GREEN_CM
inline void wlpdstm::TransactionDynamic::CmStartTx() {
	tx_energy.start_measurement();
}

inline bool wlpdstm::TransactionDynamic::ShouldAbortWrite(WriteLock *write_lock) {
	WriteLock lock_value = (WriteLock)atomic_load_no_barrier(write_lock);

	if(!is_write_locked(lock_value)) {
		return false;
	}
	
	uint64_t e = tx_energy.get_consumed_energy();

	// check if someone is waiting for this tx
	// before announcing the conflict to the victim
	if(ProcessWaitingForMe(e) == GREEN_CM_RESTART) {
		return true;
	}

	// prepare green_cm item
	green_cm_item.consumed_energy = e;
	atomic_store_release(&green_cm_item.decision, GREEN_CM_NO_DECISION);
	
	WriteLogEntry *log_entry = (WriteLogEntry *)lock_value;
	TransactionDynamic *owner = log_entry->owner;

	// some processing of the queue is taking place
	// don't decide and reread the lock
	if(!owner->green_cm_item.lock.try_lock()) {
		return false;
	}

	// here the lock is ready
	green_cm_item.next = (GreenCMQueueItem *)owner->green_cm_item.head;
	green_cm_item.head = &green_cm_item;
	owner->green_cm_item.lock.release();

	while(true) {
		GreenCMDecision decision = (GreenCMDecision)atomic_load_no_barrier(&green_cm_item.decision);

		if(decision == GREEN_CM_CONTINUE) {
			return false;
		} else if(decision == GREEN_CM_RESTART) {
			return true;
		}

		if(ProcessWaitingForMe(e) == GREEN_CM_RESTART) {
			return true;
		}		
	}

	// this should never happen
	assert(0);
}

// return true if not aborted
inline wlpdstm::GreenCMDecision wlpdstm::TransactionDynamic::ProcessWaitingForMe(uint64_t e) {
	green_cm_item.lock.lock();

	GreenCMQueueItem *item = (GreenCMQueueItem *)green_cm_item.head;

	while(item != NULL) {
		if(item->consumed_energy < e) {
			item->decision = (Word)GREEN_CM_RESTART;
		} else {
			green_cm_item.head = item;
			green_cm_item.lock.release();
			return GREEN_CM_RESTART;
		}

		item = item->next;
	}

	green_cm_item.head = NULL;
	green_cm_item.lock.release();

	return GREEN_CM_CONTINUE;
}

inline void wlpdstm::TransactionDynamic::ContinueWaitingForMe() {
	green_cm_item.lock.lock();
	
	GreenCMQueueItem *item = (GreenCMQueueItem *)green_cm_item.head;
	
	while(item != NULL) {
		item->decision = (Word)GREEN_CM_CONTINUE;
		item = item->next;
	}
	
	green_cm_item.head = NULL;
	green_cm_item.lock.release();
}

#endif /* GREEN_CM */

#ifdef DYNAMIC_INLINE
#ifdef DYNAMIC_MIXED
#include "mixed/mixed_inline_fun.h"
#include "mixed/mixed_inline.h"
#elif defined DYNAMIC_EAGER
#include "eager/eager_impl_inline.h"
#include "eager/eager_static.h"
#elif defined DYNAMIC_LAZY
#include "lazy/lazy_impl_inline.h"
#include "lazy/lazy_static.h"
#endif /* STM_ALGORITHM */
#endif /* DYNAMIC_INLINE */

#endif // WLPDSTM_TRANSACTION_DYNAMIC_H_
