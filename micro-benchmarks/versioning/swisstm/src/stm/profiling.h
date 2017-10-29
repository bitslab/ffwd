/**
 * Choose profiling implementation.
 *
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#ifndef WLPDSTM_PROFILING_H_
#define WLPDSTM_PROFILING_H_

#include "stats.h"

#ifdef WLPDSTM_TX_PROFILING_NO

#include "../common/profiling/base.h"

namespace wlpdstm {
	typedef BaseProfiling<ThreadStatistics> Profiling;
}

#elif defined WLPDSTM_TX_PROFILING_SIMPLE

#include "../common/profiling/simple.h"

namespace wlpdstm {
	typedef SimpleProfiling<ThreadStatistics, StatisticsType> Profiling;
}

#endif /* profiling class */

#endif /* WLPDSTM_PROFILING_H_ */
