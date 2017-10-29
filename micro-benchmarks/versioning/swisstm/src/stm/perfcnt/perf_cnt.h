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

#ifdef PERFORMANCE_COUNTING

#ifndef WLPDSTM_PERF_CNT_H_
#define WLPDSTM_PERF_CNT_H_

#ifdef WLPDSTM_SOLARIS

#include "perf_cnt_solaris.h"

#elif defined WLPDSTM_LINUXOS

// this assumes papi is installed and working
#include "perf_cnt_papi.h"

#else

#undef PERFORMANCE_COUNTING

#endif /* WLPDSTM_SOLARIS */

#endif /* WLPDSTM_PERF_CNT_H_ */

#endif /* PERFORMANCE_COUNTING */
