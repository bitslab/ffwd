/** 
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 */

#include "wlpdstm.h"

#include "../stm_api/read_write.h"
#include "../stm_api/wlpdstm_malloc.h"

///////////////////////////
// basic interface start //
///////////////////////////	

extern "C" {

void wlpdstm_global_init() {
	wlpdstm::CurrentTransaction::GlobalInit();
}

void wlpdstm_thread_init() {
	wlpdstm::CurrentTransaction::ThreadInit();
}

void wlpdstm_start_tx() {
	wlpdstm::CurrentTransaction::TxStart();
}

void wlpdstm_start_tx_id(int lexical_tx_id) {
	wlpdstm::CurrentTransaction::TxStart(lexical_tx_id);
}

void wlpdstm_choose_tm(Word tm) {
#ifdef DYNAMIC_SWITCH
	wlpdstm::CurrentTransaction::Get()->ChooseTM(tm);
#endif /* DYNAMIC_SWITCH */
}

LONG_JMP_BUF *wlpdstm_get_long_jmp_buf() {
	return wlpdstm::CurrentTransaction::GetLongJmpBuf();
}

void wlpdstm_commit_tx() {
	wlpdstm::CurrentTransaction::TxCommit();
}

void wlpdstm_abort_tx() {
	wlpdstm::CurrentTransaction::TxAbort();
}

void wlpdstm_restart_tx() {
	wlpdstm::CurrentTransaction::TxRestart();
}

void wlpdstm_write_word(Word *address, Word value) {
	wlpdstm::CurrentTransaction::WriteWord(address, value);
}

#ifdef SUPPORT_LOCAL_WRITES
void wlpdstm_write_word_local(Word *address, Word value) {
	wlpdstm::CurrentTransaction::WriteWordLocal(address, value);
}
#endif /* SUPPORT_LOCAL_WRITES */

Word wlpdstm_read_word(Word *address) {
	return wlpdstm::CurrentTransaction::ReadWord(address);
}

void wlpdstm_tx_free(void *ptr, size_t size) {
	wlpdstm_malloc_tx_free(ptr, size);
}

void *wlpdstm_tx_malloc(size_t size) {
	return wlpdstm_malloc_tx_malloc(size);
}

void wlpdstm_start_thread_profiling() {
	wlpdstm::CurrentTransaction::Get()->StartThreadProfiling();
}

void wlpdstm_end_thread_profiling() {
	wlpdstm::CurrentTransaction::Get()->EndThreadProfiling();
}

tx_desc *wlpdstm_get_tx_desc() {
	return (::tx_desc *)wlpdstm::CurrentTransaction::Get();
}

void wlpdstm_start_tx_desc(tx_desc *tx) {
	((wlpdstm::Transaction *)tx)->TxStart();
}

LONG_JMP_BUF *wlpdstm_get_long_jmp_buf_desc(tx_desc *tx) {
	return &((wlpdstm::Transaction *)tx)->start_buf;
}

void wlpdstm_start_tx_id_desc(tx_desc *tx, int lexical_tx_id) {
	((wlpdstm::Transaction *)tx)->TxStart(lexical_tx_id);
}

void wlpdstm_commit_tx_desc(tx_desc *tx) {
	((wlpdstm::Transaction *)tx)->TxCommit();
}

void wlpdstm_choose_tm_desc(tx_desc *tx, Word tm) {
#ifdef DYNAMIC_SWITC
	((wlpdstm::Transaction *)tx)->ChooseTM(tm);
#endif /* DYNAMIC_SWITC */
}

void wlpdstm_restart_tx_desc(tx_desc *tx) {
	((wlpdstm::Transaction *)tx)->TxRestart();
}

void wlpdstm_abort_tx_desc(tx_desc *tx) {
	((wlpdstm::Transaction *)tx)->TxAbort();
}

Word wlpdstm_read_word_desc(tx_desc *tx, Word *address) {
	return ((wlpdstm::Transaction *)tx)->ReadWord(address);
}

void wlpdstm_write_word_desc(tx_desc *tx, Word *address, Word value) {
	((wlpdstm::Transaction *)tx)->WriteWord(address, value);
}

#ifdef SUPPORT_LOCAL_WRITES
void wlpdstm_write_word_local_desc(tx_desc *tx, Word *address, Word value) {
	((wlpdstm::Transaction *)tx)->WriteWordLocal(address, value);
}
#endif /* SUPPORT_LOCAL_WRITES */

void wlpdstm_tx_free_desc(tx_desc *tx, void *ptr, size_t size) {
	((wlpdstm::Transaction *)tx)->TxFree(ptr, size);
}

void *wlpdstm_tx_malloc_desc(tx_desc *tx, size_t size) {
	return ((wlpdstm::Transaction *)tx)->TxMalloc(size);
}

void wlpdstm_start_thread_profiling_desc(tx_desc *tx) {
	((wlpdstm::Transaction *)tx)->StartThreadProfiling();
}

void wlpdstm_end_thread_profiling_desc(tx_desc *tx) {
	((wlpdstm::Transaction *)tx)->EndThreadProfiling();
}

void wlpdstm_thread_shutdown() {
	wlpdstm::CurrentTransaction::ThreadShutdown();
}
	
void wlpdstm_global_shutdown() {
	wlpdstm::Transaction::GlobalShutdown();
}

void *wlpdstm_s_malloc(size_t size) {
	return wlpdstm::MemoryManager::Malloc(size);
}

void wlpdstm_s_free(void *ptr) {
	wlpdstm::MemoryManager::Free(ptr);
}

/////////////////////////
// basic interface end //
/////////////////////////	


//////////////////////////////
// extended interface start //
//////////////////////////////

uint32_t wlpdstm_read_32(uint32_t *address) {
	return read32aligned(wlpdstm::CurrentTransaction::Get(), address);
}

uint32_t wlpdstm_read_32_desc(tx_desc *tx, uint32_t *address) {
	return read32aligned((wlpdstm::Transaction *)tx, address);
}

float wlpdstm_read_float(float *address) {
	return read_float_aligned(wlpdstm::CurrentTransaction::Get(), address);
}

float wlpdstm_read_float_desc(tx_desc *tx, float *address) {
	return read_float_aligned((wlpdstm::Transaction *)tx, address);
}

uint64_t wlpdstm_read_64(uint64_t *address) {
	return read64aligned(wlpdstm::CurrentTransaction::Get(), address);
}

uint64_t wlpdstm_read_64_desc(tx_desc *tx, uint64_t *address) {
	return read64aligned((wlpdstm::Transaction *)tx, address);
}

double wlpdstm_read_double(double *address) {
	return read_double_aligned(wlpdstm::CurrentTransaction::Get(), address);
}

double wlpdstm_read_double_desc(tx_desc *tx, double *address) {
	return read_double_aligned((wlpdstm::Transaction *)tx, address);
}

void wlpdstm_write_32(uint32_t *address, uint32_t value) {
	write32aligned(wlpdstm::CurrentTransaction::Get(), address, value);
}

void wlpdstm_write_32_desc(tx_desc *tx, uint32_t *address, uint32_t value) {
	write32aligned((wlpdstm::Transaction *)tx, address, value);
}

void wlpdstm_write_float(float *address, float value) {
	write_float_aligned(wlpdstm::CurrentTransaction::Get(), address, value);
}

void wlpdstm_write_float_desc(tx_desc *tx, float *address, float value) {
	write_float_aligned((wlpdstm::Transaction *)tx, address, value);
}

void wlpdstm_write_64(uint64_t *address, uint64_t value) {
	write64aligned(wlpdstm::CurrentTransaction::Get(), address, value);
}

void wlpdstm_write_64_desc(tx_desc *tx, uint64_t *address, uint64_t value) {
	write64aligned((wlpdstm::Transaction *)tx, address, value);
}

void wlpdstm_write_double(double *address, double value) {
	write_double_aligned(wlpdstm::CurrentTransaction::Get(), address, value);
}

void wlpdstm_write_double_desc(tx_desc *tx, double *address, double value) {
	write_double_aligned((wlpdstm::Transaction *)tx, address, value);
}

}

//////////////////////////////
// extended interface end //
//////////////////////////////
