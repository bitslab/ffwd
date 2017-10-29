/**
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#include "mixed.h"
#include "../transaction.h"

#ifdef DYNAMIC_DYNAMIC

#include "mixed_inline_fun.h"

void wlpdstm::TxMixed::TxCommit(TransactionDynamic *desc) {
	TryCommitResult result = TxTryCommit(desc);
	TxCommitAfterTry(desc, result);
}

void wlpdstm::TxMixed::TxCommitReadOnly(TransactionDynamic *desc) {
	TryCommitResult result = TxTryCommitReadOnly(desc);
	TxCommitAfterTry(desc, result);
}

void wlpdstm::TxMixed::RollbackRunning(TransactionDynamic *desc) {
	RollbackRunningInline(desc);
}

Word wlpdstm::TxMixed::ReadWord(TransactionDynamic *desc, Word *address) {
	return ReadWordInline(desc, address);
}

Word wlpdstm::TxMixed::ReadWordReadOnly(TransactionDynamic *desc, Word *address) {
	VersionLock *read_lock = TransactionDynamic::map_address_to_read_lock(address);
	return ReadWordInnerLoop(desc, address, read_lock);
}

#ifdef WLPDSTM_TX_PROFILING_ADAPTIVE_DYNAMIC
Word wlpdstm::TxMixed::ReadWordProfiled(TransactionDynamic *desc, Word *address) {
	desc->profiling.ReadWordStart(address);
	Word ret = ReadWordInline(desc, address);
	desc->profiling.ReadWordEnd(ret);
	return ret;
}

Word wlpdstm::TxMixed::ReadWordReadOnlyProfiled(TransactionDynamic *desc, Word *address) {
	desc->profiling.ReadWordStart(address);
	VersionLock *read_lock = TransactionDynamic::map_address_to_read_lock(address);
	Word ret =  ReadWordInnerLoop(desc, address, read_lock);
	desc->profiling.ReadWordEnd(ret);
	return ret;
}
#endif /* WLPDSTM_TX_PROFILING_ADAPTIVE_DYNAMIC */

wlpdstm::WriteLogEntry *wlpdstm::TxMixed::LockMemoryStripe(TransactionDynamic *desc, WriteLock *write_lock, Word *address) {
	return LockMemoryStripeInline(desc, write_lock, address);
}

wlpdstm::WriteLogEntry *wlpdstm::TxMixed::LockMemoryStripeReadOnly(TransactionDynamic *desc, WriteLock *write_lock, Word *address) {
	FirstWriteSetFunPtr(desc);
	desc->ResetFunPtrOnStart();
	return LockMemoryStripeInline(desc, write_lock, address);
}

#ifdef WLPDSTM_TX_PROFILING_ADAPTIVE_DYNAMIC
wlpdstm::WriteLogEntry *wlpdstm::TxMixed::LockMemoryStripeReadOnlyProfiled(TransactionDynamic *desc, WriteLock *write_lock, Word *address) {
	FirstWriteSetFunPtr(desc);
	desc->ResetFunPtrOnStart();
	return LockMemoryStripeInline(desc, write_lock, address);
}
#endif /* WLPDSTM_TX_PROFILING_ADAPTIVE_DYNAMIC */

void wlpdstm::TxMixed::TxStartSetFunPtr(TransactionDynamic *desc) {
	desc->tx_commit_fun = TxCommitReadOnly;
	desc->read_word_fun = ReadWordReadOnly;
	desc->lock_memory_stripe_fun = LockMemoryStripeReadOnly;
	desc->rollback_running_fun = RollbackRunning;
}

#ifdef WLPDSTM_TX_PROFILING_ADAPTIVE_DYNAMIC
void wlpdstm::TxMixed::TxStartSetFunPtrProfiled(TransactionDynamic *desc) {
	desc->tx_commit_fun = TxCommitReadOnly;
	desc->read_word_fun = ReadWordReadOnlyProfiled;
	desc->lock_memory_stripe_fun = LockMemoryStripeReadOnlyProfiled;
	desc->rollback_running_fun = RollbackRunning;
}
#endif /* WLPDSTM_TX_PROFILING_ADAPTIVE_DYNAMIC */


#elif defined DYNAMIC_STATIC
#include "mixed_inline_fun.h"
#include "mixed_inline.h"
#endif /* DYNAMIC_STATIC */
