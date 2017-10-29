#ifndef WLPDSTM_MEMORY_H_
#define WLPDSTM_MEMORY_H_

#ifdef MM_PRIVATIZATION
#include "memory/memory_impl_priv.h"
#elif defined MM_EPOCH
#include "memory/memory_impl.h"
#endif /* mm */

#endif /* WLPDSTM_MEMORY_H_ */
