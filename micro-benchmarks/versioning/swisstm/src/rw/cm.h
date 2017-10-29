/**
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#ifndef WLPDSTM_CM_H_
#define WLPDSTM_CM_H_

namespace wlpdstm {

	class Timid {
		public:
			static void GlobalInit() {
				// do nothing
			}

			void ThreadInit(unsigned tid) {
				// do nothing
			}
		
			void TxStart() {
				// do nothing
			}

			void TxCommit() {
				// do nothing
			}

			void TxAbort() {
				// do nothing
			}

			void TxRestart() {
				// do nothing
			}

			bool ShouldRestartWriteWrite(Timid *other) {
				return true;
			}

			bool ShouldRestartReadWrite(Timid *other) {
				return true;
			}

			bool ShouldRestartWriteRead(Timid *other) {
				return true;
			}

			bool ShouldRestart() const {
				return false;
			}
	};

	class Greedy {
	public:
		static void GlobalInit() {
			// do nothing
		}
		
		void ThreadInit(unsigned tid) {
			// do nothing
		}
		
		void TxStart() {
			if(!aborted) {
				current_ts = counter++;
			}

			aborted = false;
		}
		
		void TxCommit() {
			// do nothing
		}

		void TxAbort() {
			// do nothing
		}
		
		void TxRestart() {
			// do nothing
		}
		
		bool ShouldRestartWriteWrite(Greedy *other) {
			return ShoudRestartConflict(other);
		}
		
		bool ShouldRestartReadWrite(Greedy *other) {
			//return ShoudRestartConflict(other);
			// give priority to writers
			return true;
		}

		// TODO: return false here all the time
		bool ShouldRestartWriteRead(Greedy *other) {
			//return ShoudRestartConflict(other);
			other->aborted = true;
			return aborted;
		}
		
		bool ShouldRestart() const {
			return aborted;
		}

	private:
		bool ShoudRestartConflict(Greedy *other) {
			if(aborted) {
				return true;
			}

			// synchronize counters
			// races here shouldn't really matter
			if(other->counter > counter) {
				counter = other->counter;
			} else if(other->counter < counter) {
				other->counter = counter;
			}

			// decide who to abort
			if(other->current_ts < current_ts) {
				aborted = true;
			} else if(other->current_ts > current_ts) {
				other->aborted = true;
			} else {
				if(other->tid < tid) {
					aborted = true;
				} else {
					other->aborted = true;
				}
			}

			return aborted;
		}
		
	private:
		union {
			struct {
				// was I aborted
				volatile bool aborted;

				unsigned tid;

				// local counter
				volatile uint64_t counter;

				volatile uint64_t current_ts;
			};
			
			char padding[CACHE_LINE_SIZE_BYTES];
		};
	};
	
}


namespace wlpdstm {

#ifdef CM_TIMID
	typedef Timid ContentionManager;
#elif defined CM_GREEDY
	typedef Greedy ContentionManager;
#endif /* cm class */
}

#endif /* WLPDSTM_CM_H_ */
