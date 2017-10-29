/**
 * This file contains interface for profiling with no implementation. Useful for building
 * STM with no profiling overhead.
 *
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#ifndef WLPDSTM_PROFILING_BASE_H_
#define WLPDSTM_PROFILING_BASE_H_

namespace wlpdstm {

	template<class STATS>
	class BaseProfiling {
		public:
			static void GlobalInit() {
				// empty
			}

			void ThreadInit(STATS *stats) {
				// empty
			}

			void ThreadShutdown() {
				// empty
			}

			/////////////////////
			// callbacks start //
			/////////////////////

			void ThreadStart() {
				// empty
			}

			void ThreadEnd() {
				// empty
			}
		
			void TxStartStart(int lex_tx_id) {
				// empty
			}

			void TxStartEnd() {
				// empty
			}
		
			void TxCommitStart() {
				// empty
			}

			void TxCommitEnd() {
				// empty
			}
		
			void ReadWordStart(Word *addr) {
				// empty
			}

			void ReadWordEnd(Word res) {
				// empty
			}
		
			void WriteWordStart(Word *addr, Word val, Word mask = LOG_ENTRY_UNMASKED) {
				// empty
			}

			void WriteWordEnd() {
				// empty
			}
		
			void TxMallocStart(size_t size) {
				// empty
			}

			void TxMallocEnd(void *ptr) {
				// empty
			}
		
			void TxFreeStart(void *ptr, size_t size) {
				// empty
			}

			void TxFreeEnd() {
				// empty
			}

			void TxRestartStart() {
				// empty
			}

			void TxRestartEnd() {
				// empty
			}

			void TxAbortStart() {
				// empty
			}

			void TxAbortEnd() {
				// empty
			}

			///////////////////
			// callbacks end //
			///////////////////
	};
};

#endif /* WLPDSTM_PROFILING_BASE_H_ */
