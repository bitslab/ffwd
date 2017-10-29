/**
 * Definition of TLS transaction descriptor.
 *
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#ifndef WLPDSTM_CURRENT_TRANSACTION_H_
#define WLPDSTM_CURRENT_TRANSACTION_H_

#ifdef SWISSTM
#include "../stm/transaction.h"
namespace wlpdstm {
	typedef TransactionImpl Transaction;
}
#elif defined DYNAMIC
#include "../dynamic/transaction.h"
namespace wlpdstm {
	typedef TransactionImpl Transaction;
}
#else
#include "profiled_transaction.h"
#endif /* SWISSTM */
#include "../common/tls.h"

namespace wlpdstm {
	
	// A shortcut used for accessing transaction in progress.
	class CurrentTransaction : public Tls<Transaction, true, true> {
	public:
		static void TxStart(int lexical_tx_id = NO_LEXICAL_TX) {
			Tls<Transaction, true, true>::Get()->TxStart(lexical_tx_id);
		}
		
		static LONG_JMP_BUF *GetLongJmpBuf() {
			return &Tls<Transaction, true, true>::Get()->start_buf;
		}
		
		static void TxCommit() {
			Tls<Transaction, true, true>::Get()->TxCommit();
		}
		
		static void TxRestart() {
			Tls<Transaction, true, true>::Get()->TxRestart();
		}
		
		static void TxAbort() {
			Tls<Transaction, true, true>::Get()->TxAbort();
		}
		
		static void WriteWord(Word *address, Word value) {
			Tls<Transaction, true, true>::Get()->WriteWord(address, value);
		}
		
#ifdef SUPPORT_LOCAL_WRITES
		static void WriteWordLocal(Word *address, Word value) {
			Tls<Transaction, true, true>::Get()->WriteWordLocal(address, value);
		}
#endif /* SUPPORT_LOCAL_WRITES */
		
		static Word ReadWord(Word *address) {
			return Tls<Transaction, true, true>::Get()->ReadWord(address);
		}
		
		static void *TxMalloc(size_t size) {
			return Tls<Transaction, true, true>::Get()->TxMalloc(size);
		}
		
		static void TxFree(void *address, size_t size) {
			return Tls<Transaction, true, true>::Get()->TxFree(address, size);
		}

		static void ThreadShutdown() {
			Tls<Transaction, true, true>::Get()->ThreadShutdown();
		}
		
	};
}

#endif /* WLPDSTM_CURRENT_TRANSACTION_H_ */
