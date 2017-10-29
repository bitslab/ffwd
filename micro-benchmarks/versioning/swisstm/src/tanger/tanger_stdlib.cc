/**
 *  @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#include <stdlib.h>

#include "tanger-stm-std-stdlib.h"

#include "../stm/read_write.h"
#include "../stm/transaction.h"

extern "C" {
	extern void tanger_stm_std_qsort(void *base, size_t nel, size_t width,
									int (*compar)(const void *, const void *)) __attribute__((weak));
	extern void tanger_stm_std_qsort(void *base, size_t nel, size_t width,
									int (*compar)(const void *, const void *)) {
		wlpdstm::qsort_tx(wlpdstm::CurrentTransaction::Get(), base, nel, width, compar);
	}
}
