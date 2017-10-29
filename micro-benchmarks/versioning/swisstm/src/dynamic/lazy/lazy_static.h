/**
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#ifndef WLPDSTM_DYNAMIC_LAZY_STATIC_H_
#define WLPDSTM_DYNAMIC_LAZY_STATIC_H_

#include "../api_linkage.h"

API_LINKAGE void wlpdstm::TxLazy::TxCommitStatic(TransactionDynamic *desc) {
	TryCommitResult result = TxTryCommitStatic(desc);
	TxCommitAfterTry(desc, result);
}

API_LINKAGE void wlpdstm::TxLazy::RollbackRunningStatic(TransactionDynamic *desc) {
	RollbackRunningInline(desc);
}

API_LINKAGE Word wlpdstm::TxLazy::ReadWordStatic(TransactionDynamic *desc, Word *addr) {
	return ReadWordInline(desc, addr);
}

API_LINKAGE wlpdstm::WriteLogEntry *wlpdstm::TxLazy::LockMemoryStripeStatic(TransactionDynamic *desc, WriteLock *write_lock, Word *address) {
	return LockMemoryStripeInline(desc, write_lock);
}

#endif /* WLPDSTM_DYNAMIC_EAGER_STATIC_H_ */
