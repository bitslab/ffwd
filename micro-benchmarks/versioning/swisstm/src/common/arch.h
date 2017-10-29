/**
 *  Functions that depend on CPU architecture. Reading stack pointer for example.
 *
 *  @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#ifndef WLPDSTM_ARCH_H_
#define WLPDSTM_ARCH_H_

#include <stdint.h>

namespace wlpdstm {

	uintptr_t read_sp();

	uintptr_t read_bp();
	
}

inline uintptr_t wlpdstm::read_sp() {
#ifdef WLPDSTM_X86
	uintptr_t ret;
	__asm__ volatile ("mov %%esp, %0" : "=A" (ret) : : "%eax");
	return ret;	
#else 
#error Target not supported yet.
#endif  /* arch */
}

inline uintptr_t wlpdstm::read_bp() {
#ifdef WLPDSTM_X86
	uintptr_t ret;
	__asm__ volatile ("mov %%ebp, %0" : "=A" (ret) : : "%eax");
	return ret;	
#else 
#error Target not supported yet.
#endif  /* arch */
}

#endif /* WLPDSTM_ARCH_H_ */
