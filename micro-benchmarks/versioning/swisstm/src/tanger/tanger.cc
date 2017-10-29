/**
 * Implementation of tanger interface for SwissTM.
 *
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 */

extern "C" {
#include <stdlib.h>
}

#include "../stm/transaction.h"
#include "../stm/read_write.h"
//#include "../stm/common/tls.h"
#include "../stm/memory.h"
#include "tanger_stack_area.h"

extern "C" {
#include "tanger-stm-internal.h"
}

////////////////
// util start //
////////////////	

//#define TLS_STACK_AREA wlpdstm::Tls<StackArea, false, true>::Get()

#ifdef WLPDSTM_MACOS
#include <malloc/malloc.h>

inline size_t malloc_mem_size(void *ptr) {
	return malloc_size(ptr);
}
#elif defined WLPDSTM_LINUXOS
#include <malloc.h>

inline size_t malloc_mem_size(void *ptr) {
	return malloc_usable_size(ptr);
}
#endif /* OS */

//////////////
// util end //
//////////////

///////////////////////////////////////////
// tanger interface implementation start //
///////////////////////////////////////////

extern "C" {
uint8_t tanger_stm_load1(tanger_stm_tx_t* tx, uint8_t *addr) {
	return wlpdstm::read8((wlpdstm::Transaction *)tx, addr);
}

uint8_t tanger_stm_load8(tanger_stm_tx_t* tx, uint8_t *addr) {
	return wlpdstm::read8((wlpdstm::Transaction *)tx, addr);
}

uint16_t tanger_stm_load16(tanger_stm_tx_t* tx, uint16_t *addr) {
	return wlpdstm::read16((wlpdstm::Transaction *)tx, addr);
}

uint32_t tanger_stm_load32(tanger_stm_tx_t* tx, uint32_t *addr) {
	return wlpdstm::read32((wlpdstm::Transaction *)tx, addr);
}

uint64_t tanger_stm_load64(tanger_stm_tx_t* tx, uint64_t *addr) {
	return wlpdstm::read64((wlpdstm::Transaction *)tx, addr);
}

uint16_t tanger_stm_load16aligned(tanger_stm_tx_t* tx, uint16_t *addr) {
	return wlpdstm::read16aligned((wlpdstm::Transaction *)tx, addr);
}

uint32_t tanger_stm_load32aligned(tanger_stm_tx_t* tx, uint32_t *addr) {
	return wlpdstm::read32aligned((wlpdstm::Transaction *)tx, addr);
}

uint64_t tanger_stm_load64aligned(tanger_stm_tx_t* tx, uint64_t *addr) {
	return wlpdstm::read64aligned((wlpdstm::Transaction *)tx, addr);
}

void tanger_stm_loadregion(tanger_stm_tx_t* tx, uint8_t *src, uintptr_t bytes, uint8_t *dest) {
	wlpdstm::read_region((wlpdstm::Transaction *)tx, src, bytes, dest);
}

void* tanger_stm_loadregionpre(tanger_stm_tx_t* tx, uint8_t *addr, uintptr_t bytes) {
	fprintf(stderr, "tanger_stm_loadregionpre not supported yet...\n");
	exit(1);
}

void tanger_stm_loadregionpost(tanger_stm_tx_t* tx, uint8_t *addr, uintptr_t bytes) {
	fprintf(stderr, "tanger_stm_loadregionpost not supported yet...\n");
	exit(1);
}


void tanger_stm_store1(tanger_stm_tx_t* tx, uint8_t *addr, uint8_t value) {
	wlpdstm::write8((wlpdstm::Transaction *)tx, addr, value);
}

void tanger_stm_store8(tanger_stm_tx_t* tx, uint8_t *addr, uint8_t value) {
	wlpdstm::write8((wlpdstm::Transaction *)tx, addr, value);
}

void tanger_stm_store16(tanger_stm_tx_t* tx, uint16_t *addr, uint16_t value) {
	wlpdstm::write16((wlpdstm::Transaction *)tx, addr, value);
}

void tanger_stm_store32(tanger_stm_tx_t* tx, uint32_t *addr, uint32_t value) {
	wlpdstm::write32((wlpdstm::Transaction *)tx, addr, value);
}

void tanger_stm_store64(tanger_stm_tx_t* tx, uint64_t *addr, uint64_t value) {
	wlpdstm::write64((wlpdstm::Transaction *)tx, addr, value);
}

void tanger_stm_store16aligned(tanger_stm_tx_t* tx, uint16_t *addr, uint16_t value) {
	wlpdstm::write16aligned((wlpdstm::Transaction *)tx, addr, value);
}

void tanger_stm_store32aligned(tanger_stm_tx_t* tx, uint32_t *addr, uint32_t value) {
	wlpdstm::write32aligned((wlpdstm::Transaction *)tx, addr, value);
}

void tanger_stm_store64aligned(tanger_stm_tx_t* tx, uint64_t *addr, uint64_t value) {
	wlpdstm::write64aligned((wlpdstm::Transaction *)tx, addr, value);
}

void tanger_stm_storeregion(tanger_stm_tx_t* tx, uint8_t *src, uintptr_t bytes, uint8_t *dest) {
	wlpdstm::write_region((wlpdstm::Transaction *)tx, src, bytes, dest);
}

void* tanger_stm_storeregionpre(tanger_stm_tx_t* tx, uint8_t *addr, uintptr_t bytes) {
	fprintf(stderr, "tanger_stm_storeregionpre not supported yet...\n");
	exit(1);
}

void* tanger_stm_updateregionpre(tanger_stm_tx_t* tx, uint8_t *addr, uintptr_t bytes) {
	fprintf(stderr, "tanger_stm_storeregionpost not supported yet...\n");
	exit(1);
}

void tanger_stm_begin(tanger_stm_tx_t* tx) {
	((wlpdstm::Transaction *)tx)->TxStart();
}

void tanger_stm_commit(tanger_stm_tx_t* tx) {
	((wlpdstm::Transaction *)tx)->TxCommit();
	TLS_STACK_AREA->low = 0;
}

tanger_stm_tx_t* tanger_stm_get_tx() {
	return (tanger_stm_tx_t *)wlpdstm::CurrentTransaction::Get();
}

void* tanger_stm_get_jmpbuf(tanger_stm_tx_t* tx) {
	return &((wlpdstm::Transaction *)tx)->start_buf;
}

/**
 * This is based on tinystm-0.9.9 code.
 */
void tanger_stm_save_restore_stack(void* low_addr, void* high_addr) __attribute__ ((noinline));
void tanger_stm_save_restore_stack(void* low_addr, void* high_addr) {
	uintptr_t size;
	wlpdstm::StackArea *area = TLS_STACK_AREA;
	
	if(area->low != 0) {
		// restore stack area
		size = area->high - area->low;
		memcpy((void *)area->low, area->data, size);
	} else {
		// save stack area
		area->high = (uintptr_t)high_addr;
		area->low = (uintptr_t)low_addr;
		size = area->high - area->low;

		if (area->size < size) {
			// allocate twice the necessary size, and at least 1KB
			area->size = (size < 1024 / 2 ? 1024 : size * 2);
			wlpdstm::MemoryManager::Free(area->data);
			area->data = wlpdstm::MemoryManager::Malloc(size);
		}

		memcpy(area->data, (void *)area->low, size);

#ifdef STACK_PROTECT_TANGER_BOUND
		// TODO: This is a problem with Tanger if parts of a
		// function stack that
		// contains the transaction get accessed by other threads.
		// In this case, effects of other transactions might get
		// roll-backed when the transaction aborts (or restarts).
		//
		// One possible solution would be for Tanger to use a new
		// stack frame for transactional blocks and move all
		// stack variables accessed exclusively in this block to it.
		//
		// This code would work correctly if area->high is top of
		// tx_local stack (which tanger does not do).
		wlpdstm::CurrentTransaction::Get()->SetStackHigh(area->high);
#endif /* STACK_PROTECT_TANGER_BOUND */
	}	
}

void tanger_stm_init() {
	wlpdstm::CurrentTransaction::GlobalInit();
	wlpdstm::Tls<wlpdstm::StackArea, false, true>::GlobalInit();
}

void tanger_stm_shutdown() {
	wlpdstm::Transaction::GlobalShutdown();
}

void tanger_stm_thread_init() {
	wlpdstm::CurrentTransaction::ThreadInit();
	wlpdstm::ThreadStackArea::ThreadInit();
//	wlpdstm::thread_init_stack_area();
//	wlpdstm::Tls<wlpdstm::StackArea, false, true>::ThreadInit();
}

void tanger_stm_thread_shutdown() {
	wlpdstm::CurrentTransaction::ThreadShutdown();
}

void *tanger_stm_malloc(size_t size, tanger_stm_tx_t* tx) {
	return ((wlpdstm::Transaction *)tx)->TxMalloc(size);
}

void tanger_stm_free(void *ptr, tanger_stm_tx_t* tx) {
	return ((wlpdstm::Transaction *)tx)->TxFree(ptr, malloc_mem_size(ptr));
}

void *tanger_stm_calloc(size_t nmemb, size_t size, tanger_stm_tx_t* tx) {
	void *ret = ((wlpdstm::Transaction *)tx)->TxMalloc(nmemb * size);
	memset(ret, 0, nmemb * size);
	return ret;
}

void *tanger_stm_realloc(void *ptr, size_t size, tanger_stm_tx_t* tx) {
	// if it is free
	if (size == 0) {
		((wlpdstm::Transaction *)tx)->TxFree(ptr, malloc_mem_size(ptr));
		return NULL;
	}
	
	void *ret = ((wlpdstm::Transaction *)tx)->TxMalloc(size);

	// if it is malloc
	if (ptr == NULL) {
		return ret;
	}

	// copy old
	wlpdstm::read_region((wlpdstm::Transaction *)tx, (uint8_t *)ptr, (uintptr_t)size, (uint8_t *)ret);

	// free old
	((wlpdstm::Transaction *)tx)->TxFree(ptr, malloc_mem_size(ptr));
	
	return ret;
}

void fix_tanger_restart() {
	printf("Restarting...\n");
	wlpdstm::CurrentTransaction::TxRestart();
}
}

///////////////////////////////////////////
// tanger interface implementation start //
///////////////////////////////////////////
