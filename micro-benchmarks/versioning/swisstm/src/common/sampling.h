/**
 *  @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#ifndef WLPDSTM_SAMPLING_H_
#define WLPDSTM_SAMPLING_H_

#define SAMPLING_THRESHOLD 101
#define DEFAULT_RANDOM_FREQ 100

#include "random.h"

namespace wlpdstm {

	class Sampling {
		public:
			Sampling() : sample(false), counter(0) {
				// empty
			}

			bool should_sample() {
				return sample;
			}

			void tx_start() {
				if(++counter == SAMPLING_THRESHOLD) {
					counter = 0;
					sample = true;
				} else {
					sample = false;
				}
			}

		private:
			bool sample;
			unsigned counter;
	};

	class TxSampling {
		public:
			TxSampling() : counter(0) { }

			bool ShouldSample(unsigned threshold) {
				return (counter % threshold) == 0;
			}

			void TxStart() {
				counter++;
			}

		private:
			unsigned counter;
	};

	class RandomTxSampling {
		public:
			void ThreadInit(Random *r, unsigned f = DEFAULT_RANDOM_FREQ) {
				random = r;
				avg_freq = f * 2;
				reset_counter();
			}

			void TxStart() {
				if(sampling) {
                    reset_counter();
				}

                if(++counter > curr_freq) {
                    sampling = true;
                }
			}

			bool IsSampling() const {
				return sampling;
			}

        protected:
            void reset_counter() {
                sampling = false;
                counter = 0;
                curr_freq = random->Get() % avg_freq;
            }

		private:
			Random *random;
			unsigned avg_freq;
			bool sampling;
			unsigned counter;
			unsigned curr_freq;
	};

	class NoTxSampling {
		public:
			void ThreadInit(Random *r, unsigned f = DEFAULT_RANDOM_FREQ) {
				// empty
			}
			
			void TxStart() {
				// empty
			}
			
			bool IsSampling() const {
				return false;
			}
	};

#ifdef WLPDSTM_TX_PROFILING_RANDOM
	typedef RandomTxSampling TxProfiling;
#else
	typedef NoTxSampling TxProfiling;
#endif /* WLPDSTM_TX_PROFILING */
}

#endif /* WLPDSTM_SAMPLING_H_ */
