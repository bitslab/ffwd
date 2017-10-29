/**
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#ifndef WLPDSTM_DYNAMIC_EAGER_IMPL_INLINE_H_
#define WLPDSTM_DYNAMIC_EAGER_IMPL_INLINE_H_

inline wlpdstm::TxEager::TryCommitResult wlpdstm::TxEager::TxTryCommit(TransactionDynamic *desc) {
	Word ts = desc->valid_ts;
	
	// now get a commit timestamp
	ts = desc->IncrementCommitTs();
	
	// if global time overflows restart
	if(ts >= MAXIMUM_TS) {
		//executing = false;
		desc->tx_status = (Word)TX_ABORTED;
#ifdef PRIVATIZATION_QUIESCENCE
		*desc->quiescence_ts = MINIMUM_TS;
#endif /* PRIVATIZATION_QUIESCENCE */
		// this is a special case where no jump is required
		Rollback(desc);
		
		if(desc->StartSynchronization()) {
			desc->RestartCommitTS();
			desc->EndSynchronization();
			desc->stats.IncrementStatistics(StatisticsType::CLOCK_OVERFLOWS);
		}
		
		return JUMP_RESTART;
	}
	
	// if there is no validation in GV4, then the read set of one transaction could
	// overlap with the write set of another and this would pass unnoticed
#ifdef COMMIT_TS_INC
	if(ts != desc->valid_ts + 1 && !desc->ValidateWithReadLocks()) {
#elif defined COMMIT_TS_GV4
	if(!desc->ValidateWithReadLocks()) {
#endif /* commit_ts */
		desc->stats.IncrementStatistics(StatisticsType::ABORT_COMMIT_VALIDATE);
		return RESTART_COMMITTING;
	}
	
	VersionLock commitVersion = get_version_lock(ts);
	
	// now update all written values
	for(WriteLog::iterator iter = desc->write_log.begin();iter.hasNext();iter.next()) {
		WriteLogEntry &entry = *iter;
		
		// now update actual values
		WriteWordLogEntry *word_log_entry = entry.head;
		
		while(word_log_entry != NULL) {
			*word_log_entry->address = word_log_entry->MaskWord();
			word_log_entry = word_log_entry->next;
		}
		
		// release locks
		atomic_store_release(entry.read_lock, commitVersion);
		atomic_store_release(entry.write_lock, WRITE_LOCK_CLEAR);
	}			
	
	atomic_store_release(&desc->tx_status, TX_COMMITTED);
	
#ifdef PRIVATIZATION_QUIESCENCE
	atomic_store_release(desc->quiescence_ts, MINIMUM_TS);
	desc->PrivatizationQuiescenceWait(ts);
#endif /* PRIVATIZATION_QUIESCENCE */
	
	desc->write_log.clear();
	desc->read_log.clear();
	
	// commit mem
	desc->mm.TxCommit<TransactionDynamic>(ts);

	desc->stats.IncrementStatistics(StatisticsType::COMMIT);
	desc->succ_aborts = 0;

	return COMMIT;
}

inline wlpdstm::TxEager::TryCommitResult wlpdstm::TxEager::TxTryCommitReadOnly(TransactionDynamic *desc) {
	Word ts = desc->valid_ts;
	atomic_store_release(&desc->tx_status, TX_COMMITTED);
	
#ifdef PRIVATIZATION_QUIESCENCE
	atomic_store_release(desc->quiescence_ts, MINIMUM_TS);
	desc->PrivatizationQuiescenceWait(ts);
#endif /* PRIVATIZATION_QUIESCENCE */

	desc->read_log.clear();
	
	// commit mem
	desc->mm.TxCommit<TransactionDynamic>(ts);

	desc->stats.IncrementStatistics(StatisticsType::COMMIT_READ_ONLY);
	desc->stats.IncrementStatistics(StatisticsType::COMMIT);
	desc->succ_aborts = 0;
	
	return COMMIT;	
}

inline void wlpdstm::TxEager::TxCommitAfterTry(TransactionDynamic *desc, TryCommitResult result) {
	if(result == JUMP_RESTART) {
		desc->RestartJump();
	} else if(result == RESTART_RUNNING || result == RESTART_COMMITTING) {
		Restart(desc);
	}
#ifdef PERFORMANCE_COUNTING
	if(desc->perf_cnt_sampling.should_sample()) {
		// if tx is restarted, this code is not reached
		desc->perf_cnt.TxEnd();
		desc->stats.IncrementStatistics(StatisticsType::CYCLES, desc->perf_cnt.GetElapsedCycles());
		desc->stats.IncrementStatistics(StatisticsType::RETIRED_INSTRUCTIONS, desc->perf_cnt.GetRetiredInstructions());
		desc->stats.IncrementStatistics(StatisticsType::CACHE_MISSES, desc->perf_cnt.GetCacheMisses());
	}
#endif /* PERFORMANCE_COUNTING */
	
	desc->stats.TxCommit();	
}

inline void wlpdstm::TxEager::Rollback(TransactionDynamic *desc) {
	if(desc->rolled_back) {
		return;
	}
	
	desc->rolled_back = true;	
	
	for(WriteLog::iterator iter = desc->write_log.begin();iter.hasNext();iter.next()) {
		WriteLogEntry &entry = *iter;
		// first write the old version and then mark the lock clear
		*entry.read_lock = entry.old_version;
		atomic_store_release(entry.write_lock, WRITE_LOCK_CLEAR);
	}
	
	desc->read_log.clear();
	desc->write_log.clear();
	
	desc->YieldCPU();
	desc->mm.TxAbort();
}

inline void wlpdstm::TxEager::Restart(TransactionDynamic *desc) {
	desc->profiling.TxRestartStart();
#ifdef PRIVATIZATION_QUIESCENCE
	*desc->quiescence_ts = MINIMUM_TS;
#endif /* PRIVATIZATION_QUIESCENCE */
	
	Rollback(desc);
	atomic_store_release(&desc->tx_status, (Word)TX_RESTARTED);
	
#ifdef WAIT_ON_SUCC_ABORTS
	if(++desc->succ_aborts > SUCC_ABORTS_MAX) {
		desc->succ_aborts = SUCC_ABORTS_MAX;
	}
	
	if(desc->succ_aborts >= SUCC_ABORTS_THRESHOLD) {
		desc->WaitOnAbort();
	}
#endif /* WAIT_ON_SUCC_ABORTS */	
	
#ifdef PERFORMANCE_COUNTING
	if(desc->perf_cnt_sampling.should_sample()) {
		desc->perf_cnt.TxEnd();
		desc->stats.IncrementStatistics(StatisticsType::CYCLES, desc->perf_cnt.GetElapsedCycles());
		desc->stats.IncrementStatistics(StatisticsType::RETIRED_INSTRUCTIONS, desc->perf_cnt.GetRetiredInstructions());
		desc->stats.IncrementStatistics(StatisticsType::CACHE_MISSES, desc->perf_cnt.GetCacheMisses());
	}
#endif /* PERFORMANCE_COUNTING */
	desc->stats.IncrementStatistics(StatisticsType::ABORT);
	desc->profiling.TxRestartEnd();
	desc->stats.TxRestart();
	desc->RestartJump();
}

inline wlpdstm::WriteLogEntry *wlpdstm::TxEager::LockMemoryStripeInline(TransactionDynamic *desc,
																		WriteLock *write_lock, Word *address) {
#ifdef DETAILED_STATS
	desc->stats.IncrementStatistics(StatisticsType::WRITES);
#endif /* DETAILED_STATS */
	
	// read lock value
	WriteLock lock_value = (WriteLock)atomic_load_no_barrier(write_lock);
	bool locked = is_write_locked(lock_value);
	
	if(locked) {
		WriteLogEntry *log_entry = (WriteLogEntry *)lock_value;
		
		if(desc->LockedByMe(log_entry)) {
			return log_entry;
		}
	}
#ifdef DETAILED_STATS
	desc->stats.IncrementStatistics(StatisticsType::NEW_WRITES);
#endif /* DETAILED_STATS */
	
	while(true) {		
		if(locked) {
			if(desc->ShouldAbortWrite(write_lock)) {
				desc->stats.IncrementStatistics(StatisticsType::ABORT_WRITE_LOCKED);
				Restart(desc);
			} else {
				lock_value = (WriteLock)atomic_load_acquire(write_lock);
				locked = is_write_locked(lock_value);
				desc->YieldCPU();
			}
		}
		
		// prepare write log entry
		WriteLogEntry *log_entry = desc->write_log.get_next();
		log_entry->write_lock = write_lock;
		log_entry->ClearWordLogEntries(); // need this here TODO - maybe move this to commit/abort time
		log_entry->owner = desc; // this is for CM - TODO: try to move it out of write path
		
		// now try to lock it
		if(atomic_cas_release(write_lock, WRITE_LOCK_CLEAR, log_entry)) {
			// need to check read set validity if this address was read before
			// we skip this read_before() check TODO: maybe do that
			VersionLock *read_lock = TransactionDynamic::map_write_lock_to_read_lock(write_lock);
			VersionLock version = (VersionLock)atomic_load_acquire(read_lock);
			log_entry->read_lock = read_lock;
			log_entry->old_version = version;

			if(desc->ShouldExtend(version)) {
				if(!Extend(desc)) {
					desc->stats.IncrementStatistics(StatisticsType::ABORT_WRITE_VALIDATE);
					Restart(desc);
				}
			}

			// this needs to be after validation, so possibly previous
			// read is validated
			atomic_store_release(read_lock, READ_LOCK_SET);
			
			// success
			desc->CmOnAccess();
			
			return log_entry;
		}
		
		// someone locked it in the meantime
		// return last element back to the log
		desc->write_log.delete_last();
		
		// read version again
		lock_value = (WriteLock)atomic_load_acquire(write_lock);
		locked = is_write_locked(lock_value);
		desc->YieldCPU();
	}
	
	// this can never happen
	return NULL;
}

inline Word wlpdstm::TxEager::ReadWordInline(TransactionDynamic *desc, Word *address) {
	WriteLock *write_lock = TransactionDynamic::map_address_to_write_lock(address);
	WriteLogEntry *log_entry = (WriteLogEntry *)atomic_load_no_barrier(write_lock);
	
	// if locked by me return quickly
	if(desc->LockedByMe(log_entry)) {
		WriteWordLogEntry *word_log_entry = log_entry->FindWordLogEntry(address);
		
		if(word_log_entry != NULL) {
			// if it was written return from log
			return word_log_entry->MaskWord();
		} else {
			// if it was not written return from memory
			return (Word)atomic_load_no_barrier(address);
		}		
	}

	return ReadWordInnerLoop(desc, address, write_lock);
}

inline Word wlpdstm::TxEager::ReadWordInnerLoop(TransactionDynamic *desc, Word *address, WriteLock *write_lock) {
	VersionLock *read_lock = TransactionDynamic::map_write_lock_to_read_lock(write_lock);
	VersionLock version = (VersionLock)atomic_load_acquire(read_lock);
	Word value;
	
	while(true) {
		// TODO: check whether there is a bit of a problem here
		//       what if mixed transaction conflicts with eager transaction?
		//       what if lazy transaction confclits with eager transaction?
		//       it seems neither of these can really result in deadlock
		if(is_read_locked(version)) {
			if(desc->ShouldAbortWrite(write_lock)) {
				desc->stats.IncrementStatistics(StatisticsType::ABORT_WRITE_LOCKED);
				Restart(desc);
			} else {
				version = (VersionLock)atomic_load_acquire(read_lock);
				desc->YieldCPU();
				continue;
			}
		}
		
		value = (Word)atomic_load_acquire(address);
		VersionLock version_2 = (VersionLock)atomic_load_acquire(read_lock);
		
		if(version != version_2) {
			version = version_2;
			desc->YieldCPU();
			continue;
		}
		
		ReadLogEntry *entry = desc->read_log.get_next();
		entry->read_lock = read_lock;
		entry->version = version;		
		
		if(desc->ShouldExtend(version)) {
			if(!Extend(desc)) {
				// need to restart here
				desc->stats.IncrementStatistics(StatisticsType::ABORT_READ_VALIDATE);
				Restart(desc);
			}
		}
		
		break;
	}
	
	return value;
}

inline bool wlpdstm::TxEager::Extend(TransactionDynamic *desc) {
	unsigned ts = TransactionDynamic::commit_ts.readCurrentTsAcquire();
	
	if(desc->ValidateWithReadLocks()) {
		desc->valid_ts = ts;
#ifdef PRIVATIZATION_QUIESCENCE
		*desc->quiescence_ts = ts;
#endif /* PRIVATIZATION_QUIESCENCE */
		
#ifdef TS_EXTEND_STATS
		desc->stats.IncrementStatistics(StatisticsType::EXTEND_SUCCESS);
#endif /* TS_EXTEND_STATS */
		return true;
	}
	
#ifdef TS_EXTEND_STATS
	desc->stats.IncrementStatistics(StatisticsType::EXTEND_FAILURE);
#endif /* TS_EXTEND_STATS */
	
	return false;	
}

inline void wlpdstm::TxEager::FirstWriteSetFunPtr(TransactionDynamic *desc) {
	desc->tx_commit_fun = TxCommit;
	desc->read_word_fun = ReadWord;
	desc->lock_memory_stripe_fun = LockMemoryStripe;
}

#ifdef WLPDSTM_TX_PROFILING_ADAPTIVE_DYNAMIC
inline void wlpdstm::TxLazy::FirstWriteSetFunPtrProfiled(TransactionDynamic *desc) {
	desc->tx_commit_fun = TxCommit;
	desc->read_word_fun = ReadWordProfiled;
	desc->lock_memory_stripe_fun = LockMemoryStripe;
}
#endif /* WLPDSTM_TX_PROFILING_ADAPTIVE_DYNAMIC */

#endif /* WLPDSTM_DYNAMIC_EAGER_IMPL_INLINE_H_ */
