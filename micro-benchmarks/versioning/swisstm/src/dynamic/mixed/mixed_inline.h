/**
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#ifndef WLPDSTM_DYNAMIC_MIXED_INLINE_H_
#define WLPDSTM_DYNAMIC_MIXED_INLINE_H_

#include "../api_linkage.h"

API_LINKAGE void wlpdstm::TxMixed::TxCommitStatic(TransactionDynamic *desc) {
	TryCommitResult result = TxTryCommitStatic(desc);
	TxCommitAfterTry(desc, result);
}

inline wlpdstm::TxMixed::TryCommitResult wlpdstm::TxMixed::TxTryCommitStatic(TransactionDynamic *desc) {
	Word ts = desc->valid_ts;
	bool read_only = desc->write_log.empty();
	
	if(!read_only) {		
		// first lock all read locks
		for(WriteLog::iterator iter = desc->write_log.begin();iter.hasNext();iter.next()) {
			WriteLogEntry &entry = *iter;
			*(entry.read_lock) = READ_LOCK_SET;
		}
		
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
			RollbackCommitting(desc);
			
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
#ifdef GREEN_CM
		desc->ContinueWaitingForMe();
#endif /* GREEN_CM */
	}

	desc->read_log.clear();
	
	// commit mem
	desc->mm.TxCommit<TransactionDynamic>(ts);
	
	desc->stats.IncrementStatistics(StatisticsType::COMMIT);
#ifndef GREEN_CM
	desc->succ_aborts = 0;
#endif /* GREEN_CM */

	return COMMIT;		
}

API_LINKAGE void wlpdstm::TxMixed::RollbackRunningStatic(TransactionDynamic *desc) {
	RollbackRunningInline(desc);
}

API_LINKAGE Word wlpdstm::TxMixed::ReadWordStatic(TransactionDynamic *desc, Word *addr) {
	return ReadWordInline(desc, addr);
}

API_LINKAGE wlpdstm::WriteLogEntry *wlpdstm::TxMixed::LockMemoryStripeStatic(TransactionDynamic *desc, WriteLock *write_lock, Word *address) {
	return LockMemoryStripeInline(desc, write_lock, address);
}


#endif /* WLPDSTM_DYNAMIC_MIXED_INLINE_H_ */
