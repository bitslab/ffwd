/**
 *  @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#ifndef WLPDSTM_ICC_MEMORY_H_
#define WLPDSTM_ICC_MEMORY_H_

#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif

__attribute__((tm_wrapping(malloc))) void *wlpdstm_icc_malloc(size_t size);
__attribute__((tm_wrapping(free))) void wlpdstm_icc_free(void *ptr);

			  
#ifdef __cplusplus
}
#endif
			  
#endif /* WLPDSTM_ICC_MEMORY_H_ */
