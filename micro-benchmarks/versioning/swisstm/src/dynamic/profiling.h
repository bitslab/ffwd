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

#elif defined WLPDSTM_TX_PROFILING_ADAPTIVE
#include "../common/profiling/adaptive.h"

namespace wlpdstm {
	typedef AdaptiveProfiling<ThreadStatistics, StatisticsType> Profiling;
}

#elif defined WLPDSTM_TX_PROFILING_ADAPTIVE_DYNAMIC
// NOTE: this makes sense only with dynamic function calls
#include "profiling/adaptive_dynamic.h"

namespace wlpdstm {
	typedef AdaptiveDynamicProfiling<ThreadStatistics, StatisticsType> Profiling;
}

#elif defined WLPDSTM_TX_PROFILING_TX_DURATION
#include "../common/profiling/tx_duration.h"

namespace wlpdstm {
	typedef TxDurationProfiling<ThreadStatistics, StatisticsType> Profiling;
}

#elif defined WLPDSTM_NON_TX_PROFILING_TX_DURATION
#include "../common/profiling/non_tx_duration.h"

namespace wlpdstm {
	typedef NonTxDurationProfiling<ThreadStatistics, StatisticsType> Profiling;
}

#elif defined WLPDSTM_NON_TX_PROFILING_TX_DURATION_SIMPLE
#include "../common/profiling/non_tx_duration_simple.h"

namespace wlpdstm {
	typedef NonTxDurationProfilingSimple<ThreadStatistics, StatisticsType> Profiling;
}

#elif defined WLPDSTM_NON_TX_PROFILING_TX_DURATION_PCNT_SIMPLE
#include "../common/profiling/non_tx_duration_pcnt_simple.h"

namespace wlpdstm {
	typedef NonTxDurationPcntProfilingSimple<ThreadStatistics, StatisticsType> Profiling;
}

#elif defined WLPDSTM_NON_TX_PROFILING_TX_DURATION_PCNT_SAMPLING
#include "../common/profiling/non_tx_duration_pcnt_sampling.h"

namespace wlpdstm {
	typedef NonTxDurationPcntProfilingSampling<ThreadStatistics, StatisticsType> Profiling;
}

#endif /* profiling class */

#endif /* WLPDSTM_PROFILING_H_ */
