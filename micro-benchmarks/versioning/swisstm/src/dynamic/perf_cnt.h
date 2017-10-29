/**
 * The implementing classes should implement the following interface:
 *
 * - GlobalInit()
 * - ThreadInit()
 * - TxStart()
 * - TxEnd()
 * valid after TxEnd() is called:
 * - uint64_t GetElapsedCycles()
 * - uint64_t GetRetiredInstructions()
 *
 *  @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 */

#ifndef WLPDSTM_PERF_CNT_H_
#define WLPDSTM_PERF_CNT_H_

#include "../common/perfcnt/perf_cnt_impl.h"

namespace wlpdstm {

    class PerfCnt {
        public:
            void ThreadInit() {
                pcnt.Init();
            }

            void TxStart() {
                pcnt.Start();
            }

            void TxEnd() {
                pcnt.End();
            }

            static void GlobalInit() {
                PerfCntImpl::GlobalInit();
            }

            uint64_t GetElapsedCycles() {
                return pcnt.GetElapsedCycles();
            }

            uint64_t GetRetiredInstructions() {
                return pcnt.GetRetiredInstructions();
            }

            uint64_t GetCacheMisses() {
                return pcnt.GetCacheMisses();
            }

        protected:
            PerfCntImpl pcnt;
    };

}

#endif /* WLPDSTM_PERF_CNT_H_ */
