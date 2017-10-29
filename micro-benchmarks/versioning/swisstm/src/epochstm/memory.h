/**
 *  @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#ifndef WLPDSTM_EPOCHSTM_MEMORY_H_
#define WLPDSTM_EPOCHSTM_MEMORY_H_

#ifdef MM_LEAK
#include "memory/memory_leak.h"
#elif defined MM_TXEND
#include "memory/memory_txend.h"
#elif defined MM_EPOCH
#include "memory/memory_epoch.h"
#endif

#endif /* WLPDSTM_EPOCHSTM_MEMORY_H_ */

