/**
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 */

#ifndef WLPDSTM_WLPDSTM_H_
#define WLPDSTM_WLPDSTM_H_

#include <stdlib.h>
#include <stdint.h>

#include "../common/word.h"
#include "../common/jmp.h"
#include "../common/tm_impl_const.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	///////////////////////////
	// basic interface start //
	///////////////////////////	
	
	typedef void *tx_desc;

	// initialization
	void wlpdstm_global_init();
	void wlpdstm_thread_init();

	// cleanup
	void wlpdstm_thread_shutdown();
	void wlpdstm_global_shutdown();	

	// start/end tx
	void wlpdstm_start_tx() __attribute__ ((noinline));
	void wlpdstm_start_tx_id(int lexical_tx_id) __attribute__ ((noinline));

	void wlpdstm_choose_tm(Word tm);

	LONG_JMP_BUF *wlpdstm_get_long_jmp_buf();

	void wlpdstm_commit_tx();

	void wlpdstm_abort_tx();

	void wlpdstm_restart_tx();

	// read/write word
	Word wlpdstm_read_word(Word *address);

	void wlpdstm_write_word(Word *address, Word value);

#ifdef SUPPORT_LOCAL_WRITES
	void wlpdstm_write_word_local(Word *address, Word value);
#endif /* SUPPORT_LOCAL_WRITES */

	// memory management
	void *wlpdstm_tx_malloc(size_t size);
	void wlpdstm_tx_free(void *ptr, size_t size);

	// invoke before the start of the first transaction to measure
	// the duration of all the code before it
	void wlpdstm_start_thread_profiling();
	// invoke after the end of the last transaction to measure the
	// duration of all the code after it
	void wlpdstm_end_thread_profiling();

	///////////////////////////////////////////////////////
	// a separate set of methods if current tx is cached //
	///////////////////////////////////////////////////////

	tx_desc *wlpdstm_get_tx_desc();

	// start/end tx
	void wlpdstm_start_tx_desc(tx_desc *tx);
	void wlpdstm_start_tx_id_desc(tx_desc *tx, int lexical_tx_id);

	void wlpdstm_choose_tm_desc(tx_desc *tx, Word tm);

	LONG_JMP_BUF *wlpdstm_get_long_jmp_buf_desc(tx_desc *tx);

	void wlpdstm_commit_tx_desc(tx_desc *tx);

	void wlpdstm_abort_tx_desc(tx_desc *tx);

	void wlpdstm_restart_tx_desc(tx_desc *tx);

	// read/write word
	Word wlpdstm_read_word_desc(tx_desc *tx, Word *address);

	void wlpdstm_write_word_desc(tx_desc *tx, Word *address, Word value);

#ifdef SUPPORT_LOCAL_WRITES
	void wlpdstm_write_word_local_desc(tx_desc *tx, Word *address, Word value);
#endif /* SUPPORT_LOCAL_WRITES */

	// memory management
	void *wlpdstm_tx_malloc_desc(tx_desc *tx, size_t size);
	void wlpdstm_tx_free_desc(tx_desc *tx, void *ptr, size_t size);

	// use for non-tx code to be able to simply switch
	// the whole memory management scheme
	void *wlpdstm_s_malloc(size_t size);
	void wlpdstm_s_free(void *ptr);

	void wlpdstm_start_thread_profiling_desc(tx_desc *tx);
	void wlpdstm_end_thread_profiling_desc(tx_desc *tx);

	/////////////////////////
	// basic interface end //
	/////////////////////////	

	//////////////////////////////
	// extended interface start //
	//////////////////////////////

	uint32_t wlpdstm_read_32(uint32_t *address);
	uint32_t wlpdstm_read_32_desc(tx_desc *tx, uint32_t *address);
	float wlpdstm_read_float(float *address);
	float wlpdstm_read_float_desc(tx_desc *tx, float *address);
	uint64_t wlpdstm_read_64(uint64_t *address);
	uint64_t wlpdstm_read_64_desc(tx_desc *tx, uint64_t *address);
	double wlpdstm_read_double(double *address);
	double wlpdstm_read_double_desc(tx_desc *tx, double *address);
	
	void wlpdstm_write_32(uint32_t *address, uint32_t value);
	void wlpdstm_write_32_desc(tx_desc *tx, uint32_t *address, uint32_t value);
	void wlpdstm_write_float(float *address, float value);
	void wlpdstm_write_float_desc(tx_desc *tx, float *address, float value);
	void wlpdstm_write_64(uint64_t *address, uint64_t value);
	void wlpdstm_write_64_desc(tx_desc *tx, uint64_t *address, uint64_t value);
	void wlpdstm_write_double(double *address, double value);
	void wlpdstm_write_double_desc(tx_desc *tx, double *address, double value);
	
	////////////////////////////
	// extended interface end //
	////////////////////////////	
#ifdef __cplusplus
}
#endif /* __cplusplus */
		

#endif // WLPDSTM_WLPDSTM_H_

