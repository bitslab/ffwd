/**
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#include "eager.h"
#include "../transaction.h"

#ifdef DYNAMIC_DYNAMIC

#include "eager_impl_inline.h"

void wlpdstm::TxEager::TxCommit(TransactionDynamic *desc) {
	TryCommitResult result = TxTryCommit(desc);
	TxCommitAfterTry(desc, result);
}

void wlpdstm::TxEager::TxCommitReadOnly(TransactionDynamic *desc) {
	TryCommitResult result = TxTryCommitReadOnly(desc);
	TxCommitAfterTry(desc, result);
}

void wlpdstm::TxEager::RollbackRunning(TransactionDynamic *desc) {
	Rollback(desc);
}

Word wlpdstm::TxEager::ReadWord(TransactionDynamic *desc, Word *address) {
	return ReadWordInline(desc, address);
}

Word wlpdstm::TxEager::ReadWordReadOnly(TransactionDynamic *desc, Word *address) {
	WriteLock *write_lock = TransactionDynamic::map_address_to_write_lock(address);
	return ReadWordInnerLoop(desc, address, write_lock);
}

#ifdef WLPDSTM_TX_PROFILING_ADAPTIVE_DYNAMIC
Word wlpdstm::TxEager::ReadWordProfiled(TransactionDynamic *desc, Word *address) {
	desc->profiling.ReadWordStart(address);
	Word ret = ReadWordInline(desc, address);
	desc->profiling.ReadWordEnd(ret);
	return ret;
}

Word wlpdstm::TxEager::ReadWordReadOnlyProfiled(TransactionDynamic *desc, Word *address) {
	desc->profiling.ReadWordStart(address);
	VersionLock *read_lock = TransactionDynamic::map_address_to_read_lock(address);
	Word ret =  ReadWordInnerLoop(desc, address, read_lock);
	desc->profiling.ReadWordEnd(ret);
	return ret;
}
#endif /* WLPDSTM_TX_PROFILING_ADAPTIVE_DYNAMIC */

wlpdstm::WriteLogEntry *wlpdstm::TxEager::LockMemoryStripe(TransactionDynamic *desc, WriteLock *write_lock, Word *address) {
	return LockMemoryStripeInline(desc, write_lock, address);
}

wlpdstm::WriteLogEntry *wlpdstm::TxEager::LockMemoryStripeReadOnly(TransactionDynamic *desc, WriteLock *write_lock, Word *address) {
	WriteLogEntry *ret = LockMemoryStripeInline(desc, write_lock, address);
	FirstWriteSetFunPtr(desc);
	desc->ResetFunPtrOnStart();
	return ret;
}

#ifdef WLPDSTM_TX_PROFILING_ADAPTIVE_DYNAMIC
wlpdstm::WriteLogEntry *wlpdstm::TxEager::LockMemoryStripeReadOnlyProfiled(TransactionDynamic *desc, WriteLock *write_lock, Word *address) {
	FirstWriteSetFunPtr(desc);
	desc->ResetFunPtrOnStart();
	return LockMemoryStripeInline(desc, write_lock, address);
}
#endif /* WLPDSTM_TX_PROFILING_ADAPTIVE_DYNAMIC */

void wlpdstm::TxEager::TxStartSetFunPtr(TransactionDynamic *desc) {
	desc->tx_commit_fun = TxCommitReadOnly;
	desc->read_word_fun = ReadWordReadOnly;
	desc->lock_memory_stripe_fun = LockMemoryStripeReadOnly;
	desc->rollback_running_fun = RollbackRunning;
}	

#ifdef WLPDSTM_TX_PROFILING_ADAPTIVE_DYNAMIC
void wlpdstm::TxEager::TxStartSetFunPtrProfiled(TransactionDynamic *desc) {
	desc->tx_commit_fun = TxCommitReadOnly;
	desc->read_word_fun = ReadWordReadOnlyProfiled;
	desc->lock_memory_stripe_fun = LockMemoryStripeReadOnlyProfiled;
	desc->rollback_running_fun = RollbackRunning;
}
#endif /* WLPDSTM_TX_PROFILING_ADAPTIVE_DYNAMIC */

#elif defined DYNAMIC_STATIC
#include "eager_impl_inline.h"
#include "eager_static.h"
#endif /* DYNAMIC_STATIC */

