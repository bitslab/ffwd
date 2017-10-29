/**
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#include "lazy.h"
#include "../transaction.h"

#ifdef DYNAMIC_DYNAMIC

#include "lazy_impl_inline.h"

void wlpdstm::TxLazy::TxCommit(TransactionDynamic *desc) {
	TryCommitResult result = TxTryCommit(desc);
	TxCommitAfterTry(desc, result);
}

void wlpdstm::TxLazy::TxCommitReadOnly(TransactionDynamic *desc) {
	TryCommitResult result = TxTryCommitReadOnly(desc);
	TxCommitAfterTry(desc, result);
}

void wlpdstm::TxLazy::RollbackRunning(TransactionDynamic *desc) {
	RollbackRunningInline(desc);
}

Word wlpdstm::TxLazy::ReadWord(TransactionDynamic *desc, Word *address) {
	return ReadWordInline(desc, address);
}

Word wlpdstm::TxLazy::ReadWordReadOnly(TransactionDynamic *desc, Word *address) {
	VersionLock *read_lock = TransactionDynamic::map_address_to_read_lock(address);
	return ReadWordInnerLoop(desc, address, read_lock);
}

#ifdef WLPDSTM_TX_PROFILING_ADAPTIVE_DYNAMIC
Word wlpdstm::TxLazy::ReadWordProfiled(TransactionDynamic *desc, Word *address) {
	desc->profiling.ReadWordStart(address);
	Word ret = ReadWordInline(desc, address);
	desc->profiling.ReadWordEnd(ret);
	return ret;
}

Word wlpdstm::TxLazy::ReadWordReadOnlyProfiled(TransactionDynamic *desc, Word *address) {
	desc->profiling.ReadWordStart(address);
	VersionLock *read_lock = TransactionDynamic::map_address_to_read_lock(address);
	Word ret =  ReadWordInnerLoop(desc, address, read_lock);
	desc->profiling.ReadWordEnd(ret);
	return ret;
}
#endif /* WLPDSTM_TX_PROFILING_ADAPTIVE_DYNAMIC */

wlpdstm::WriteLogEntry *wlpdstm::TxLazy::LockMemoryStripe(TransactionDynamic *desc, WriteLock *write_lock, Word *address) {
	return LockMemoryStripeInline(desc, write_lock);
}

wlpdstm::WriteLogEntry *wlpdstm::TxLazy::LockMemoryStripeReadOnly(TransactionDynamic *desc, WriteLock *write_lock, Word *address) {
	WriteLogEntry *ret = LockMemoryStripeInline(desc, write_lock);
	FirstWriteSetFunPtr(desc);
	desc->ResetFunPtrOnStart();
	return ret;
}

#ifdef WLPDSTM_TX_PROFILING_ADAPTIVE_DYNAMIC
wlpdstm::WriteLogEntry *wlpdstm::TxLazy::LockMemoryStripeReadOnlyProfiled(TransactionDynamic *desc, WriteLock *write_lock, Word *address) {
	FirstWriteSetFunPtr(desc);
	desc->ResetFunPtrOnStart();
	return LockMemoryStripeInline(desc, write_lock);
}
#endif /* WLPDSTM_TX_PROFILING_ADAPTIVE_DYNAMIC */

void wlpdstm::TxLazy::TxStartSetFunPtr(TransactionDynamic *desc) {
	desc->tx_commit_fun = TxCommitReadOnly;
	desc->read_word_fun = ReadWordReadOnly;
	desc->lock_memory_stripe_fun = LockMemoryStripeReadOnly;
	desc->rollback_running_fun = RollbackRunning;
}	

#ifdef WLPDSTM_TX_PROFILING_ADAPTIVE_DYNAMIC
void wlpdstm::TxLazy::TxStartSetFunPtrProfiled(TransactionDynamic *desc) {
	desc->tx_commit_fun = TxCommitReadOnly;
	desc->read_word_fun = ReadWordReadOnlyProfiled;
	desc->lock_memory_stripe_fun = LockMemoryStripeReadOnlyProfiled;
	desc->rollback_running_fun = RollbackRunning;
}
#endif /* WLPDSTM_TX_PROFILING_ADAPTIVE_DYNAMIC */

#elif defined DYNAMIC_STATIC
#include "lazy_impl_inline.h"
#include "lazy_static.h"
#endif /* DYNAMIC_STATIC */

