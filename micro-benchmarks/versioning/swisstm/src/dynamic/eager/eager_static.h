/**
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#ifndef WLPDSTM_DYNAMIC_EAGER_STATIC_H_
#define WLPDSTM_DYNAMIC_EAGER_STATIC_H_

#include "../api_linkage.h"

API_LINKAGE void wlpdstm::TxEager::TxCommitStatic(TransactionDynamic *desc) {
	TryCommitResult result = TxTryCommitStatic(desc);
	TxCommitAfterTry(desc, result);
}

API_LINKAGE void wlpdstm::TxEager::RollbackRunningStatic(TransactionDynamic *desc) {
	Rollback(desc);
}

API_LINKAGE Word wlpdstm::TxEager::ReadWordStatic(TransactionDynamic *desc, Word *addr) {
	return ReadWordInline(desc, addr);
}

API_LINKAGE wlpdstm::WriteLogEntry *wlpdstm::TxEager::LockMemoryStripeStatic(TransactionDynamic *desc, WriteLock *write_lock, Word *address) {
	return LockMemoryStripeInline(desc, write_lock, address);
}


inline wlpdstm::TxEager::TryCommitResult wlpdstm::TxEager::TxTryCommitStatic(TransactionDynamic *desc) {
	Word ts = desc->valid_ts;
	bool read_only = desc->write_log.empty();
	
	if(!read_only) {
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
	} else {
		desc->stats.IncrementStatistics(StatisticsType::COMMIT_READ_ONLY);
	}

	atomic_store_release(&desc->tx_status, TX_COMMITTED);
	
#ifdef PRIVATIZATION_QUIESCENCE
	atomic_store_release(desc->quiescence_ts, MINIMUM_TS);
	desc->PrivatizationQuiescenceWait(ts);
#endif /* PRIVATIZATION_QUIESCENCE */

	if(!read_only) {
		desc->write_log.clear();
	}

	desc->read_log.clear();
	
	// commit mem
	desc->mm.TxCommit<TransactionDynamic>(ts);
	
	desc->stats.IncrementStatistics(StatisticsType::COMMIT);
	desc->succ_aborts = 0;
	
	return COMMIT;				
}

#endif /* WLPDSTM_DYNAMIC_EAGER_STATIC_H_ */
