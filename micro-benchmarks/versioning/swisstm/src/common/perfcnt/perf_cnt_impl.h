/**
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#ifdef WLPDSTM_SOLARIS

#include "perf_cnt_solaris.h"

#elif defined WLPDSTM_LINUXOS

// this assumes papi is installed and working
#include "perf_cnt_papi.h"

#endif /* WLPDSTM_SOLARIS */
