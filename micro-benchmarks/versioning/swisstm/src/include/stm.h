#include "../src/api/wlpdstm.h"

#define BEGIN_TRANSACTION             \
	if(sigsetjmp(*wlpdstm_get_long_jmp_buf(), 0) != LONG_JMP_ABORT_FLAG) {   \
        wlpdstm_start_tx()

#define BEGIN_TRANSACTION_ID(TX_ID)   \
    if(sigsetjmp(*wlpdstm_get_long_jmp_buf(), 0) != LONG_JMP_ABORT_FLAG) {   \
		wlpdstm_start_tx_id(TX_ID)

#define END_TRANSACTION					\
		wlpdstm_commit_tx();			\
	}

#define BEGIN_TRANSACTION_DESC			\
	if(sigsetjmp(*wlpdstm_get_long_jmp_buf_desc(tx), 0) != LONG_JMP_ABORT_FLAG) {	\
		wlpdstm_start_tx_desc(tx)

#define BEGIN_TRANSACTION_DESC_ID(TX_ID)	\
	if(sigsetjmp(*wlpdstm_get_long_jmp_buf_desc(tx), 0) != LONG_JMP_ABORT_FLAG) {	\
		wlpdstm_start_tx_id_desc(tx, TX_ID)

#define END_TRANSACTION_DESC				\
		wlpdstm_commit_tx_desc(tx);			\
	}
