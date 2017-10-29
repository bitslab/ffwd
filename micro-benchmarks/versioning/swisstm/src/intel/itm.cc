/**
 * This is a file that implements Intel's STM compiler ABI.
 *
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 */

#include <stdlib.h>
#include <stdio.h>

#include "../stm/transaction.h"
#include "../stm/common/atomic.h"
#include "../stm/read_write.h"

#include <itm.h>

#ifdef __cplusplus
extern "C"
{
#endif
	
	/////////////////////////////////////
	// definitions for itmuser.h start //
	/////////////////////////////////////

	// TODO: check whether this needs to call initialize thread
	// specs are not 100% clear on that 
	_ITM_transaction *_ITM_CALL_CONVENTION _ITM_getTransaction(void) {
		_ITM_transaction *ret = (_ITM_transaction *)wlpdstm::CurrentTransaction::Get();

		if(ret == NULL) {
			_ITM_initializeProcess();
			ret = (_ITM_transaction *)wlpdstm::CurrentTransaction::Get();
		}

		return ret;
	}

	_ITM_howExecuting _ITM_CALL_CONVENTION _ITM_inTransaction(_ITM_transaction *td) {
		wlpdstm::TxMixinv *tx = (wlpdstm::TxMixinv *)td;
		return tx->IsExecuting() ? inRetryableTransaction : outsideTransaction;
	}
	
	int _ITM_CALL_CONVENTION _ITM_getThreadnum(void) {
		wlpdstm::TxMixinv *tx = (wlpdstm::TxMixinv *)_ITM_getTransaction();
		return tx->GetThreadId();
	}

	// NOT SUPPORTED
	void _ITM_CALL_CONVENTION _ITM_addUserCommitAction(_ITM_transaction * __td, 
													   _ITM_userCommitFunction __commit,
													   _ITM_transactionId resumingTransactionId,
													   void * __arg) {
		_ITM_userError("Invoking non-implemented _ITM_addUserCommitAction\n", 1);
	}

	// NOT SUPPORTED
	void _ITM_CALL_CONVENTION _ITM_addUserUndoAction(_ITM_transaction * __td, 
													 const _ITM_userUndoFunction __undo, void * __arg) {
		_ITM_userError("Invoking non-implemented _ITM_addUserUndoAction\n", 1);
	}
	
	_ITM_transactionId _ITM_CALL_CONVENTION _ITM_getTransactionId(_ITM_transaction *td) {
		wlpdstm::TxMixinv *tx = (wlpdstm::TxMixinv *)td;
		return tx->GetTransactionId();
	}

	// NOT SUPPORTED
	void _ITM_CALL_CONVENTION _ITM_dropReferences(_ITM_transaction * __td, const void * __start, size_t __size) {
		_ITM_userError("Invoking non-implemented _ITM_dropReferences\n", 1);
	}
	
	void _ITM_CALL_CONVENTION _ITM_userError(const char *errString, int exitCode) {
		fprintf(stderr, errString);
		exit(exitCode);
	}

	/////////////////////////////
	// memory management start //
	/////////////////////////////

// NOTE: I had to put all into the same file, otherwise the linker complains about
// wrong dwarf version/format when linking the benchmark 

#include "icc_memory.h"

#if defined WLPDSTM_LINUXOS
#include <malloc.h>
	
	inline size_t malloc_mem_size(void *ptr) {
		return malloc_usable_size(ptr);
	}
#endif /* WLPDSTM_LINUXOS */
	
	void *wlpdstm_icc_malloc(size_t size) {
		wlpdstm::TxMixinv *tx = (wlpdstm::TxMixinv *)_ITM_getTransaction();
		return tx->TxMalloc(size);
	}
	
	void wlpdstm_icc_free(void *ptr) {
		wlpdstm::TxMixinv *tx = (wlpdstm::TxMixinv *)_ITM_getTransaction();
		tx->TxFree(ptr, malloc_mem_size(ptr));
	}
	
	///////////////////////////
	// memory management end //
	///////////////////////////
	
	
	///////////////////////////
	// definitions for itm.h //
	///////////////////////////

	// TODO: these are unimplemented for now
	_ITM_EXPORT void * _ITM_CALL_CONVENTION _ITM_malloc (size_t size) {
		_ITM_userError("Invoking non-implemented _ITM_malloc", 1);
		return NULL;
	}
	
	_ITM_EXPORT void _ITM_CALL_CONVENTION _ITM_free (void * ptr) {
		_ITM_userError("Invoking non-implemented _ITM_free", 1);
	}
	
	/*! Version checking */
	_ITM_EXPORT const char * _ITM_CALL_CONVENTION _ITM_libraryVersion (void) {
		return "SwissTM 0.001";
	}
	
	/*! Call with the _ITM_VERSION_NO macro defined above, so that the
	 * library can handle compatibility issues.
	 */
	_ITM_EXPORT int _ITM_CALL_CONVENTION _ITM_versionCompatible (int) { /* Zero if not compatible */
		// always compatible for now
		return 1;
	}
	
	/*! Initialization, finalization
	 * Result of initialization function is zero for success, non-zero for failure, so you want
	 * if (!_ITM_initializeProcess())
	 * {
	 *     ... error exit ...
	 * }
	 */
	_ITM_EXPORT int _ITM_CALL_CONVENTION _ITM_initializeProcess(void) { /* Idempotent */
		static wlpdstm::PaddedSpinTryLock lock;
		static volatile bool initialized = 0;

		if(initialized) {
			_ITM_initializeThread();
			return 0;
		}

		while(!lock.try_lock()) {
			// do nothing
		}

		if(!initialized) {
			printf("Initializing the process\n");
			initialized = 1;
			wlpdstm::CurrentTransaction::GlobalInit();
		}

		lock.release();

		// needed by ABI spec
		_ITM_initializeThread();

		return 0;
	}
	
	_ITM_EXPORT void _ITM_CALL_CONVENTION _ITM_finalizeProcess(void) {
		wlpdstm::Transaction::GlobalShutdown();
	}

	_ITM_EXPORT int _ITM_CALL_CONVENTION _ITM_initializeThread(void) {
		printf("Initializing thread\n");
		wlpdstm::CurrentTransaction::ThreadInit();
		return 0;
	}

	_ITM_EXPORT void _ITM_CALL_CONVENTION _ITM_finalizeThread(void) {
		wlpdstm::CurrentThread::ThreadShutdown();
	}

	/*! Error reporting */
	_ITM_EXPORT void _ITM_CALL_CONVENTION _ITM_error (const _ITM_srcLocation *loc, int errorCode) {
		fprintf(stderr, "Error occurred at %s\n", loc->psource);
		exit(errorCode);
	}
	
	/*! Begin a transaction.
	 * This function can return more than once (cf setjump). The result always tells the compiled
	 * code what needs to be executed next.
	 * \param __properties A bit mask composed of values from _ITM_codeProperties or-ed together.
	 * \param __src The source location.
	 * \return A bit mask composed of values from _ITM_actions or-ed together.
	 */
	uint32 _ITM_CALL_CONVENTION _inner_beginTransaction(_ITM_transaction *td,
														uint32 __properties,
														void *dummy_ret_addr,
														const _ITM_srcLocation *__src) {
		wlpdstm::Transaction *tx = (wlpdstm::Transaction *)td;
#ifdef STACK_PROTECT_ICC_BOUND
		tx->SetStackHigh(tx->start_buf.esp);
#endif /* STACK_PROTECT_ICC_BOUND */
		
		tx->TxStart();
		return a_runInstrumentedCode | a_saveLiveVariables;
	}

	uint32 _ITM_CALL_CONVENTION _inner_jmpReturn(_ITM_transaction *td) {
		wlpdstm::Transaction *tx = (wlpdstm::Transaction *)td;
		wlpdstm::Transaction::TxStatus status = tx->GetTxStatus();
		uint32 ret;

		if(status == wlpdstm::Transaction::TX_IDLE) {
			_ITM_userError("Invoking _inner_jmpReturn for idle transaction\n", 1);
		} else if(status == wlpdstm::Transaction::TX_RESTARTED) {
			ret = a_runInstrumentedCode | a_restoreLiveVariables;
			tx->TxStart();
		} else if(status == wlpdstm::Transaction::TX_ABORTED) {
			ret = a_abortTransaction | a_restoreLiveVariables;
		} else if(status == wlpdstm::Transaction::TX_COMMITTED) {
			_ITM_userError("Invoking _inner_jmpReturn for committed transaction\n", 1);
		} else if(status == wlpdstm::Transaction::TX_EXECUTING) {
			_ITM_userError("Invoking _inner_jmpReturn while transaction is executing\n", 1);
		} else {
			_ITM_userError("Unknown status in _inner_jmpReturn\n", 1);
		}
		
		return ret;		
	}

	/*! Commit the innermost transaction. If the innermost transaction is also the outermost,
	 * validate and release all reader/writer locks.
	 * If this fuction returns the transaction is committed. On a commit
	 * failure the commit will not return, but will instead longjump and return from the associated
	 * beginTransaction call.
	 * \param __src The source location of the transaction.
	 * 
	 */
	_ITM_EXPORT void _ITM_CALL_CONVENTION _ITM_commitTransaction(_ITM_transaction *td,
																 const _ITM_srcLocation *__src) {
		wlpdstm::Transaction *tx = (wlpdstm::Transaction *)td;
		tx->TxCommit();
	}
	
	/*! Try to commit the innermost transaction. If the innermost transaction is also the outermost,
	 * validate. If validation fails, return 0. Otherwise,  release all reader/writer locks and return 1.
	 * \param __src The source location of the transaction.
	 * 
	 */
	_ITM_EXPORT uint32 _ITM_CALL_CONVENTION _ITM_tryCommitTransaction(_ITM_transaction *td,
																	  const _ITM_srcLocation *__src) {
		_ITM_userError("Invoking non-implemented _ITM_tryCommitTransaction\n", 1);
		return 0;
	}

	/*! Commit the to the nested transaction specifed by the first arugment.
	 * \param tid The transaction id for the nested transaction that we are commit to.
	 * \param __src The source location of the catch block.
	 * 
	 */
	_ITM_EXPORT void _ITM_CALL_CONVENTION _ITM_commitTransactionToId(_ITM_transaction *td,
																	 const _ITM_transactionId tid,
																	 const _ITM_srcLocation *__src) {
		_ITM_userError("Invoking non-implemented _ITM_commitTransactionToId\n", 1);
	}
	
	/*! Abort a transaction.
	 * This function will never return, instead it will longjump and return from the associated
	 * beginTransaction call (or from the beginTransaction call assocatiated with a catch block or destructor
	 * entered on an exception).
	 * \param __td The transaction pointer.
	 * \param __reason The reason for the abort.
	 * \param __src The source location of the __tm_abort (if abort is called by the compiler).
	 */
	_ITM_EXPORT void _ITM_CALL_CONVENTION _ITM_abortTransaction(_ITM_transaction *td,
																_ITM_abortReason __reason,
																const _ITM_srcLocation *__src) {
		wlpdstm::Transaction *tx = (wlpdstm::Transaction *)td;

		if(__reason == userAbort) {
			tx->TxAbort();
		} else if(__reason == userRetry) {
			tx->TxRestart();
		} else {
			printf("%d", __reason);
			_ITM_userError("Unexpected reason in _ITM_abortTransaction\n", 1);
		}

		// this should never happen
		_ITM_userError("Never happens in _ITM_abortTransaction\n", 1);
	}
	
	/*! Rollback a transaction to the innermost nesting level.
	 * This function is similar to abortTransaction, but returns.
	 * It does not longjump and return from a beginTransaction.
	 * \param __td The transaction pointer.
	 * \param __src The source location of the __tm_abort (if abort is called by the compiler).
	 */
	/*! Abort a transaction which may or may not be nested. Arguments and semantics as for abortOuterTransaction. */
	/* Will return if called with uplevelAbort, otherwise it longjumps */
	_ITM_EXPORT void _ITM_CALL_CONVENTION _ITM_rollbackTransaction(_ITM_transaction *td,
																   const _ITM_srcLocation *__src) {
		_ITM_userError("Invoking non-implemented _ITM_rollbackTransactionToId\n", 1);
	}

	/*! Register the thrown object to avoid undoing it, in case the transaction aborts and throws an exception
	 * from inside a catch handler.
	 * \param __td The transaction pointer.
	 * \param __obj The base address of the thrown object.
	 * \param __size The size of the object in bytes.
	 */
	_ITM_EXPORT void _ITM_CALL_CONVENTION _ITM_registerThrownObject(_ITM_transaction *td,
																	const void *__obj, size_t __size) {
		_ITM_userError("Invoking non-implemented _ITM_registerThrownObject\n", 1);
	}
	
	/*! Enter an irrevocable mode.
	 * If this function returns then execution is now in the new mode, however it's possible that
	 * to enter the new mode the transaction must be aborted and retried, in which case this function
	 * will not return.
	 *
	 * \param __td The transaction descriptor.
	 * \param __mode The new execution mode.
	 * \param __src The source location.
	 */
	_ITM_EXPORT void _ITM_CALL_CONVENTION _ITM_changeTransactionMode(_ITM_transaction *td,
																	 _ITM_transactionState __mode,
																	 const _ITM_srcLocation * __loc) {
		printf("Switching to mode %d at %s\n", __mode, __loc->psource);
		_ITM_userError("Invoking non-implemented _ITM_changeTransactionMode\n", 1);
	}
	

	////////////////////////////
	// read/write definitions //
	////////////////////////////

	// 8 bits
	_ITM_CALL_CONVENTION uint8 _ITM_RU1 (_ITM_transaction *td, const uint8 *addr) {
		return wlpdstm::read8((wlpdstm::Transaction *)td, (uint8_t *)addr);
	}

	_ITM_CALL_CONVENTION uint8 _ITM_RaRU1(_ITM_transaction *td, const uint8 *addr) {
		return wlpdstm::read8((wlpdstm::Transaction *)td, (uint8_t *)addr);
	}

	_ITM_CALL_CONVENTION uint8 _ITM_RaWU1(_ITM_transaction *td, const uint8 *addr) {
		return wlpdstm::read8((wlpdstm::Transaction *)td, (uint8_t *)addr);
	}

	_ITM_CALL_CONVENTION uint8 _ITM_RfWU1(_ITM_transaction *td, const uint8 *addr) {
		return wlpdstm::read8((wlpdstm::Transaction *)td, (uint8_t *)addr);
	}

	_ITM_CALL_CONVENTION void _ITM_WU1(_ITM_transaction *td, uint8 *addr,  uint8 val) {
		wlpdstm::write8((wlpdstm::Transaction *)td, (uint8_t *)addr, (uint8_t)val);
	}

	_ITM_CALL_CONVENTION void _ITM_WaRU1(_ITM_transaction *td, uint8 *addr, uint8 val) {
		wlpdstm::write8((wlpdstm::Transaction *)td, (uint8_t *)addr, (uint8_t)val);
	}

	_ITM_CALL_CONVENTION void _ITM_WaWU1(_ITM_transaction *td, uint8 *addr, uint8 val) {
		wlpdstm::write8((wlpdstm::Transaction *)td, (uint8_t *)addr, (uint8_t)val);
	}

	// 16 bits
	_ITM_CALL_CONVENTION uint16 _ITM_RU2(_ITM_transaction *td, const uint16 *addr) {
		_ITM_userError("Invoking non-implemented read 16", 1);
		return 0;
	}

	_ITM_CALL_CONVENTION uint16 _ITM_RaRU2(_ITM_transaction *td, const uint16 *addr) {
		_ITM_userError("Invoking non-implemented read 16", 1);
		return 0;
	}

	_ITM_CALL_CONVENTION uint16 _ITM_RaWU2(_ITM_transaction *td, const uint16 *addr) {
		_ITM_userError("Invoking non-implemented read 16", 1);
		return 0;
	}

	_ITM_CALL_CONVENTION uint16 _ITM_RfWU2(_ITM_transaction *td, const uint16 *addr) {
		_ITM_userError("Invoking non-implemented read 16", 1);
		return 0;
	}

	_ITM_CALL_CONVENTION void _ITM_WU2(_ITM_transaction *td, uint16 *addr, uint16 val) {
		_ITM_userError("Invoking non-implemented write 16", 1);
	}

	_ITM_CALL_CONVENTION void _ITM_WaRU2(_ITM_transaction *td, uint16 *addr,  uint16 val) {
		_ITM_userError("Invoking non-implemented write 16", 1);
	}

	_ITM_CALL_CONVENTION void _ITM_WaWU2(_ITM_transaction *td, uint16 *addr, uint16 val) {
		_ITM_userError("Invoking non-implemented write 16", 1);
	}

	// 32 bits
	_ITM_CALL_CONVENTION uint32 _ITM_RU4(_ITM_transaction *td, const uint32 *addr) {
		return wlpdstm::read32((wlpdstm::Transaction *)td, (uint32_t *)addr);
	}

	_ITM_CALL_CONVENTION uint32 _ITM_RaRU4(_ITM_transaction *td, const uint32 *addr) {
		return wlpdstm::read32((wlpdstm::Transaction *)td,  (uint32_t *)addr);
	}

	_ITM_CALL_CONVENTION uint32 _ITM_RaWU4(_ITM_transaction *td, const uint32 *addr) {
		return wlpdstm::read32((wlpdstm::Transaction *)td,  (uint32_t *)addr);
	}

	_ITM_CALL_CONVENTION uint32 _ITM_RfWU4(_ITM_transaction *td, const uint32 *addr) {
		return wlpdstm::read32((wlpdstm::Transaction *)td,  (uint32_t *)addr);
	}

	_ITM_CALL_CONVENTION void _ITM_WU4(_ITM_transaction *td, uint32 *addr, uint32 val) {
		wlpdstm::write32((wlpdstm::Transaction *)td, (uint32_t *)addr, (uint32_t)val);
	}

	_ITM_CALL_CONVENTION void _ITM_WaRU4(_ITM_transaction *td,  uint32 *addr, uint32 val) {
		wlpdstm::write32((wlpdstm::Transaction *)td, (uint32_t *)addr, (uint32_t)val);
	}

	_ITM_CALL_CONVENTION void _ITM_WaWU4(_ITM_transaction *td,  uint32 *addr, uint32 val) {
		wlpdstm::write32((wlpdstm::Transaction *)td, (uint32_t *)addr, (uint32_t)val);
	}

	// 64 bits
	_ITM_CALL_CONVENTION uint64 _ITM_RU8(_ITM_transaction *td, const uint64 *addr) {
		_ITM_userError("Invoking non-implemented read 64", 1);
		return 0;
	}

	_ITM_CALL_CONVENTION uint64 _ITM_RaRU8(_ITM_transaction *td, const uint64 *addr) {
		_ITM_userError("Invoking non-implemented read 64", 1);
		return 0;
	}

	_ITM_CALL_CONVENTION uint64 _ITM_RaWU8(_ITM_transaction *td, const uint64 *addr) {
		_ITM_userError("Invoking non-implemented read 64", 1);
		return 0;
	}

	_ITM_CALL_CONVENTION uint64  _ITM_RfWU8(_ITM_transaction *td, const uint64 *addr) {
		_ITM_userError("Invoking non-implemented read 64", 1);
		return 0;
	}

	_ITM_CALL_CONVENTION void _ITM_WU8(_ITM_transaction *td, uint64 *addr, uint64 val) {
		_ITM_userError("Invoking non-implemented write 64", 1);
	}

	_ITM_CALL_CONVENTION void _ITM_WaRU8(_ITM_transaction *td, uint64 *addr, uint64 val) {
		_ITM_userError("Invoking non-implemented write 64", 1);
	}

	_ITM_CALL_CONVENTION void _ITM_WaWU8(_ITM_transaction *td, uint64 *addr, uint64 val) {
		_ITM_userError("Invoking non-implemented write 64", 1);
	}

	// float
	_ITM_CALL_CONVENTION float _ITM_RF(_ITM_transaction *td, const float *addr) {
		return read_float((wlpdstm::Transaction *)td, (float *)addr);
	}

	_ITM_CALL_CONVENTION float _ITM_RaRF(_ITM_transaction *td, const float *addr) {
		return read_float((wlpdstm::Transaction *)td, (float *)addr);
	}

	_ITM_CALL_CONVENTION float _ITM_RaWF(_ITM_transaction *td, const float *addr) {
		return read_float((wlpdstm::Transaction *)td, (float *)addr);
	}

	_ITM_CALL_CONVENTION float _ITM_RfWF(_ITM_transaction *td, const float  *addr) {
		return read_float((wlpdstm::Transaction *)td, (float *)addr);
	}

	_ITM_CALL_CONVENTION void _ITM_WF(_ITM_transaction *td, float *addr, float val) {
		write_float((wlpdstm::Transaction *)td, addr, val);
	}

	_ITM_CALL_CONVENTION void _ITM_WaRF(_ITM_transaction *td, float *addr, float val) {
		write_float((wlpdstm::Transaction *)td, addr, val);
	}

	_ITM_CALL_CONVENTION void _ITM_WaWF(_ITM_transaction *td, float *addr, float val) {
		write_float((wlpdstm::Transaction *)td, addr, val);
	}

	// double
	_ITM_CALL_CONVENTION double _ITM_RD(_ITM_transaction *td, const double *addr) {
		return read_double((wlpdstm::Transaction *)td, (double *)addr);
	}

	_ITM_CALL_CONVENTION double _ITM_RaRD(_ITM_transaction *td, const double *addr) {
		return read_double((wlpdstm::Transaction *)td, (double *)addr);
	}

	_ITM_CALL_CONVENTION double _ITM_RaWD(_ITM_transaction *td, const double *addr) {
		return read_double((wlpdstm::Transaction *)td, (double *)addr);
	}

	_ITM_CALL_CONVENTION double _ITM_RfWD(_ITM_transaction *td, const double *addr) {
		return read_double((wlpdstm::Transaction *)td, (double *)addr);
	}

	_ITM_CALL_CONVENTION void _ITM_WD(_ITM_transaction *td,  double *addr, double val) {
		write_double((wlpdstm::Transaction *)td, addr, val);
	}

	_ITM_CALL_CONVENTION void _ITM_WaRD(_ITM_transaction *td,  double *addr, double val) {
		write_double((wlpdstm::Transaction *)td, addr, val);
	}

	_ITM_CALL_CONVENTION void _ITM_WaWD(_ITM_transaction *td,  double *addr, double val) {
		write_double((wlpdstm::Transaction *)td, addr, val);
	}


	// m128
	_ITM_CALL_CONVENTION __m128 _ITM_RM128(_ITM_transaction *td, const __m128 *addr) {
		return read_m128((wlpdstm::Transaction *)td, (__m128 *)addr);
	}

	_ITM_CALL_CONVENTION __m128 _ITM_RaRM128(_ITM_transaction *td, const __m128 *addr) {
		return read_m128((wlpdstm::Transaction *)td, (__m128 *)addr);
	}

	_ITM_CALL_CONVENTION __m128 _ITM_RaWM128(_ITM_transaction *td, const __m128 *addr) {
		return read_m128((wlpdstm::Transaction *)td, (__m128 *)addr);
	}

	_ITM_CALL_CONVENTION __m128 _ITM_RfWM128(_ITM_transaction *td, const __m128 *addr) {
		return read_m128((wlpdstm::Transaction *)td, (__m128 *)addr);
	}

	_ITM_CALL_CONVENTION void _ITM_WM128(_ITM_transaction *td, __m128 *addr, __m128 val) {
		write_m128((wlpdstm::Transaction *)td, addr, val);
	}

	_ITM_CALL_CONVENTION void _ITM_WaRM128(_ITM_transaction *td, __m128 *addr, __m128 val) {
		write_m128((wlpdstm::Transaction *)td, addr, val);
	}

	_ITM_CALL_CONVENTION void _ITM_WaWM128(_ITM_transaction *td, __m128 *addr, __m128 val) {
		write_m128((wlpdstm::Transaction *)td, addr, val);
	}

	
/* TODO These are not really necessary for the benchmarks we have, so skip implementation for now.
	// long double
	_ITM_CALL_CONVENTION long double _ITM_RE(_ITM_transaction *td, const long double *addr);  
	_ITM_CALL_CONVENTION long double _ITM_RaRE(_ITM_transaction *td, const long double *addr);  
	_ITM_CALL_CONVENTION long double _ITM_RaWE(_ITM_transaction *td, const long double *addr);  
	_ITM_CALL_CONVENTION long double _ITM_RfWE(_ITM_transaction *td, const long double *addr);  
	_ITM_CALL_CONVENTION void _ITM_WE(_ITM_transaction *td, long double *addr, long double val);  
	_ITM_CALL_CONVENTION void _ITM_WaRE(_ITM_transaction *td, long double *addr, long double val);  
	_ITM_CALL_CONVENTION void _ITM_WaWE(_ITM_transaction *td, long double *addr, long double val);  

	// m64
	_ITM_CALL_CONVENTION __m64 _ITM_RM64 (_ITM_transaction *td,  const  __m64 *addr);  
	_ITM_CALL_CONVENTION __m64 _ITM_RaRM64 (_ITM_transaction *td,  const  __m64 *addr);  
	_ITM_CALL_CONVENTION __m64 _ITM_RaWM64 (_ITM_transaction *td,  const  __m64 *addr);  
	_ITM_CALL_CONVENTION __m64  _ITM_RfWM64 (_ITM_transaction *td,  const  __m64 *addr);  
	_ITM_CALL_CONVENTION void _ITM_WM64 (_ITM_transaction *td,  __m64 *addr,  __m64 val);  
	_ITM_CALL_CONVENTION void _ITM_WaRM64 (_ITM_transaction *td,  __m64 *addr,  __m64 val);  
	_ITM_CALL_CONVENTION void _ITM_WaWM64 (_ITM_transaction *td,  __m64 *addr,  __m64 val);  

	// m256
	_ITM_CALL_CONVENTION __m256 _ITM_RM256 (_ITM_transaction *td,  const  __m256 *addr);  
	_ITM_CALL_CONVENTION __m256 _ITM_RaRM256 (_ITM_transaction *td,  const  __m256 *addr);  
	_ITM_CALL_CONVENTION __m256 _ITM_RaWM256 (_ITM_transaction *td,  const  __m256 *addr);  
	_ITM_CALL_CONVENTION __m256  _ITM_RfWM256 (_ITM_transaction *td,  const  __m256 *addr);  
	_ITM_CALL_CONVENTION void _ITM_WM256 (_ITM_transaction *td,  __m256 *addr,  __m256 val);  
	_ITM_CALL_CONVENTION void _ITM_WaRM256 (_ITM_transaction *td,  __m256 *addr,  __m256 val);  
	_ITM_CALL_CONVENTION void _ITM_WaWM256 (_ITM_transaction *td,  __m256 *addr,  __m256 val);  

	// float complex
	_ITM_CALL_CONVENTION float _Complex _ITM_RCF (_ITM_transaction *td,  const  float _Complex *addr);  
	_ITM_CALL_CONVENTION float _Complex _ITM_RaRCF (_ITM_transaction *td,  const  float _Complex *addr);  
	_ITM_CALL_CONVENTION float _Complex _ITM_RaWCF (_ITM_transaction *td,  const  float _Complex *addr);  
	_ITM_CALL_CONVENTION float _Complex  _ITM_RfWCF (_ITM_transaction *td,  const  float _Complex *addr);  
	_ITM_CALL_CONVENTION void _ITM_WCF (_ITM_transaction *td,  float _Complex *addr,  float _Complex val);  
	_ITM_CALL_CONVENTION void _ITM_WaRCF (_ITM_transaction *td,  float _Complex *addr,  float _Complex val);  
	_ITM_CALL_CONVENTION void _ITM_WaWCF (_ITM_transaction *td,  float _Complex *addr,  float _Complex val);  

	// double complex
	_ITM_CALL_CONVENTION double _Complex _ITM_RCD (_ITM_transaction *td,  const  double _Complex *addr);  
	_ITM_CALL_CONVENTION double _Complex _ITM_RaRCD (_ITM_transaction *td,  const  double _Complex *addr);  
	_ITM_CALL_CONVENTION double _Complex _ITM_RaWCD (_ITM_transaction *td,  const  double _Complex *addr);  
	_ITM_CALL_CONVENTION double _Complex  _ITM_RfWCD (_ITM_transaction *td,  const  double _Complex *addr);  
	_ITM_CALL_CONVENTION void _ITM_WCD (_ITM_transaction *td,  double _Complex *addr,  double _Complex val);  
	_ITM_CALL_CONVENTION void _ITM_WaRCD (_ITM_transaction *td,  double _Complex *addr,  double _Complex val);  
	_ITM_CALL_CONVENTION void _ITM_WaWCD (_ITM_transaction *td,  double _Complex *addr,  double _Complex val);  

	// long double complex
	_ITM_CALL_CONVENTION long double _Complex _ITM_RCE (_ITM_transaction *td, const long double _Complex *addr);  
	_ITM_CALL_CONVENTION long double _Complex _ITM_RaRCE (_ITM_transaction *td, const long double _Complex *addr);  
	_ITM_CALL_CONVENTION long double _Complex _ITM_RaWCE (_ITM_transaction *td, const long double _Complex *addr);  
	_ITM_CALL_CONVENTION long double _Complex  _ITM_RfWCE (_ITM_transaction *td, const long double _Complex *addr);  
	
	_ITM_CALL_CONVENTION void _ITM_WCE (_ITM_transaction *td, long double _Complex *addr, long double _Complex val);
	_ITM_CALL_CONVENTION void _ITM_WaRCE (_ITM_transaction *td, long double _Complex *addr, long double _Complex val);  
	_ITM_CALL_CONVENTION void _ITM_WaWCE (_ITM_transaction *td, long double _Complex *addr, long double _Complex val);  	
*/

	// memcpy
	_ITM_CALL_CONVENTION void _ITM_memcpyRnWt(_ITM_transaction *td, void *dest, const void *src, size_t size) {
		wlpdstm::memcpy_tx((wlpdstm::Transaction *)td, dest, src, size);
	}

	_ITM_CALL_CONVENTION void _ITM_memcpyRnWtaR(_ITM_transaction *td, void *dest, const void *src, size_t size) {
		wlpdstm::memcpy_tx((wlpdstm::Transaction *)td, dest, src, size);
	}

	_ITM_CALL_CONVENTION void _ITM_memcpyRnWtaW(_ITM_transaction *td, void *dest, const void *src, size_t size) {
		wlpdstm::memcpy_tx((wlpdstm::Transaction *)td, dest, src, size);
	}

	_ITM_CALL_CONVENTION void _ITM_memcpyRtWn(_ITM_transaction *td, void *dest, const void *src, size_t size) {
		wlpdstm::memcpy_tx((wlpdstm::Transaction *)td, dest, src, size);
	}

	_ITM_CALL_CONVENTION void _ITM_memcpyRtWt(_ITM_transaction *td, void *dest, const void *src, size_t size) {
		wlpdstm::memcpy_tx((wlpdstm::Transaction *)td, dest, src, size);
	}

	_ITM_CALL_CONVENTION void _ITM_memcpyRtWtaR(_ITM_transaction *td, void *dest, const void *src, size_t size) {
		wlpdstm::memcpy_tx((wlpdstm::Transaction *)td, dest, src, size);
	}

	_ITM_CALL_CONVENTION void _ITM_memcpyRtWtaW(_ITM_transaction *td, void *dest, const void *src, size_t size) {
		wlpdstm::memcpy_tx((wlpdstm::Transaction *)td, dest, src, size);
	}

	_ITM_CALL_CONVENTION void _ITM_memcpyRtaRWn(_ITM_transaction *td, void *dest, const void *src, size_t size) {
		wlpdstm::memcpy_tx((wlpdstm::Transaction *)td, dest, src, size);
	}

	_ITM_CALL_CONVENTION void _ITM_memcpyRtaRWt(_ITM_transaction *td, void *dest, const void *src, size_t size) {
		wlpdstm::memcpy_tx((wlpdstm::Transaction *)td, dest, src, size);
	}

	_ITM_CALL_CONVENTION void _ITM_memcpyRtaRWtaR(_ITM_transaction *td, void *dest, const void *src, size_t size) {
		wlpdstm::memcpy_tx((wlpdstm::Transaction *)td, dest, src, size);
	}

	_ITM_CALL_CONVENTION void _ITM_memcpyRtaRWtaW(_ITM_transaction *td, void *dest, const void *src, size_t size) {
		wlpdstm::memcpy_tx((wlpdstm::Transaction *)td, dest, src, size);
	}

	_ITM_CALL_CONVENTION void _ITM_memcpyRtaWWn(_ITM_transaction *td, void *dest, const void *src, size_t size) {
		wlpdstm::memcpy_tx((wlpdstm::Transaction *)td, dest, src, size);
	}

	_ITM_CALL_CONVENTION void _ITM_memcpyRtaWWt(_ITM_transaction *td, void *dest, const void *src, size_t size) {
		wlpdstm::memcpy_tx((wlpdstm::Transaction *)td, dest, src, size);
	}

	_ITM_CALL_CONVENTION void _ITM_memcpyRtaWWtaR(_ITM_transaction *td, void *dest, const void *src, size_t size) {
		wlpdstm::memcpy_tx((wlpdstm::Transaction *)td, dest, src, size);
	}

	_ITM_CALL_CONVENTION void _ITM_memcpyRtaWWtaW(_ITM_transaction *td, void *dest, const void *src, size_t size) {
		wlpdstm::memcpy_tx((wlpdstm::Transaction *)td, dest, src, size);
	}

	// memmove
	_ITM_CALL_CONVENTION void _ITM_memmoveRnWt(_ITM_transaction *td, void *dest, const void *src, size_t size) {
		_ITM_userError("Invoking non-implemented memmmove", 1);
	}

	_ITM_CALL_CONVENTION void _ITM_memmoveRnWtaR(_ITM_transaction *td, void *dest, const void *src, size_t size) {
		_ITM_userError("Invoking non-implemented memmmove", 1);
	}

	_ITM_CALL_CONVENTION void _ITM_memmoveRnWtaW(_ITM_transaction *td, void *dest, const void *src, size_t size) {
		_ITM_userError("Invoking non-implemented memmmove", 1);
	}

	_ITM_CALL_CONVENTION void _ITM_memmoveRtWn(_ITM_transaction *td, void *dest, const void *src, size_t size) {
		_ITM_userError("Invoking non-implemented memmmove", 1);
	}

	_ITM_CALL_CONVENTION void _ITM_memmoveRtWt(_ITM_transaction *td, void *dest, const void *src, size_t size) {
		_ITM_userError("Invoking non-implemented memmmove", 1);
	}

	_ITM_CALL_CONVENTION void _ITM_memmoveRtWtaR(_ITM_transaction *td, void *dest, const void *src, size_t size) {
		_ITM_userError("Invoking non-implemented memmmove", 1);
	}

	_ITM_CALL_CONVENTION void _ITM_memmoveRtWtaW(_ITM_transaction *td, void *dest, const void *src, size_t size) {
		_ITM_userError("Invoking non-implemented memmmove", 1);
	}

	_ITM_CALL_CONVENTION void _ITM_memmoveRtaRWn(_ITM_transaction *td, void *dest, const void *src, size_t size) {
		_ITM_userError("Invoking non-implemented memmmove", 1);
	}

	_ITM_CALL_CONVENTION void _ITM_memmoveRtaRWt(_ITM_transaction *td, void *dest, const void *src, size_t size) {
		_ITM_userError("Invoking non-implemented memmmove", 1);
	}

	_ITM_CALL_CONVENTION void _ITM_memmoveRtaRWtaR(_ITM_transaction *td, void *dest, const void *src, size_t size) {
		_ITM_userError("Invoking non-implemented memmmove", 1);
	}

	_ITM_CALL_CONVENTION void _ITM_memmoveRtaRWtaW(_ITM_transaction *td, void *dest, const void *src, size_t size) {
		_ITM_userError("Invoking non-implemented memmmove", 1);
	}

	_ITM_CALL_CONVENTION void _ITM_memmoveRtaWWn(_ITM_transaction *td, void *dest, const void *src, size_t size) {
		_ITM_userError("Invoking non-implemented memmmove", 1);
	}

	_ITM_CALL_CONVENTION void _ITM_memmoveRtaWWt(_ITM_transaction *td, void *dest, const void *src, size_t size) {
		_ITM_userError("Invoking non-implemented memmmove", 1);
	}

	_ITM_CALL_CONVENTION void _ITM_memmoveRtaWWtaR(_ITM_transaction *td, void *dest, const void *src, size_t size) {
		_ITM_userError("Invoking non-implemented memmmove", 1);
	}

	_ITM_CALL_CONVENTION void _ITM_memmoveRtaWWtaW(_ITM_transaction *td, void *dest, const void *src, size_t size) {
		_ITM_userError("Invoking non-implemented memmmove", 1);
	}

	// memset
	_ITM_CALL_CONVENTION void _ITM_memsetW(_ITM_transaction *td, void *target, int src, size_t count) {
		_ITM_userError("Invoking non-implemented memset", 1);
	}

	_ITM_CALL_CONVENTION void _ITM_memsetWaR(_ITM_transaction *td, void *target, int src, size_t count) {
		_ITM_userError("Invoking non-implemented memset", 1);
	}

	_ITM_CALL_CONVENTION void _ITM_memsetWaW(_ITM_transaction *td, void *target, int src, size_t count) {
		_ITM_userError("Invoking non-implemented memset", 1);
	}

	// logging
	_ITM_CALL_CONVENTION void _ITM_LU1(_ITM_transaction *td, const uint8 *addr) {
		write8((wlpdstm::Transaction *)td,  (uint8_t *)addr, *((uint8_t *)addr));
	}

	_ITM_CALL_CONVENTION void _ITM_LU2(_ITM_transaction *td, const uint16 *addr) {
		write16((wlpdstm::Transaction *)td,  (uint16_t *)addr, *((uint16_t *)addr));
	}

	_ITM_CALL_CONVENTION void _ITM_LU4(_ITM_transaction *td, const uint32 *addr) {
		write32((wlpdstm::Transaction *)td,  (uint32_t *)addr, *((uint32_t *)addr));
	}

	_ITM_CALL_CONVENTION void _ITM_LU8(_ITM_transaction *td, const uint64 *addr) {
		write64((wlpdstm::Transaction *)td,  (uint64_t *)addr, *((uint64_t *)addr));
	}

	_ITM_CALL_CONVENTION void _ITM_LF(_ITM_transaction *td, const float *addr) {
		_ITM_userError("Invoking non-implemented logging float", 1);
	}

	_ITM_CALL_CONVENTION void _ITM_LD(_ITM_transaction *td, const double *addr) {
		_ITM_userError("Invoking non-implemented logging double", 1);
	}

	_ITM_CALL_CONVENTION void _ITM_LE(_ITM_transaction *td, const long double *addr) {
		_ITM_userError("Invoking non-implemented logging extended double", 1);
	}

	_ITM_CALL_CONVENTION void _ITM_LM64(_ITM_transaction *td, const __m64 *addr) {
		_ITM_userError("Invoking non-implemented logging m64", 1);
	}

	_ITM_CALL_CONVENTION void _ITM_LM128(_ITM_transaction *td, const __m128 *addr) {
		_ITM_userError("Invoking non-implemented logging m128", 1);
	}

// TODO Not important right now
//	_ITM_CALL_CONVENTION void _ITM_LM256(_ITM_transaction *td, const __m256 *addr) {
//		_ITM_userError("Invoking non-implemented logging", 1);
//	}

	_ITM_CALL_CONVENTION void _ITM_LCF(_ITM_transaction *td, const float _Complex *addr) {
		_ITM_userError("Invoking non-implemented logging", 1);
	}

	_ITM_CALL_CONVENTION void _ITM_LCD(_ITM_transaction *td, const double _Complex *addr) {
		_ITM_userError("Invoking non-implemented logging", 1);
	}

	_ITM_CALL_CONVENTION void _ITM_LCE(_ITM_transaction *td, const long double _Complex *addr) {
		_ITM_userError("Invoking non-implemented logging", 1);
	}

	_ITM_CALL_CONVENTION void _ITM_LB(_ITM_transaction *td, const void *addr,  size_t size) {
		_ITM_userError("Invoking non-implemented logging", 1);
	}

#ifdef __cplusplus
}
#endif

