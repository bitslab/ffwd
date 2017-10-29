/**
 * This code currently works only for 32 bit jumps.
 *
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 */

#ifndef WLPDSTM_ICC_JMP_H_
#define WLPDSTM_ICC_JMP_H_

#include <itm.h>
#include <stdint.h>

typedef struct begin_transaction_jmpbuf {
	uint32_t ebx;
	uint32_t esp;
	uint32_t ebp;
	uint32_t esi;
	uint32_t edi;
	uint32_t ret_addr;
} my_jmpbuf;

#ifdef __cplusplus
extern "C" {
#endif
//_ITM_CALL_CONVENTION int _ITM_beginTransaction(begin_transaction_jmpbuf *td, uint32 __properties, const _ITM_srcLocation *__src);
_ITM_CALL_CONVENTION void jmp_to_begin_transaction(begin_transaction_jmpbuf *td);
#ifdef __cplusplus
}
#endif
		
#endif /* WLPDSTM_ICC_JMP_H_ */
