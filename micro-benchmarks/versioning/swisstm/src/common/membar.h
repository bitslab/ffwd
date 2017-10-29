/**
 * It seems it might be better to have various types of memory barrires explicitly defined
 * instead of coupling them with memory accesses like done in atomic_ops library.
 *
 * The naming scheme for different barriers is:
 *
 *     (membar_<op1>_<op2>)+
 *
 * This means that all op1 types have to execute before the next op2. op1 and op2 can
 * be either load or store.
 *
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 */

#ifndef WLPDSTM_MEMBAR_H_
#define WLPDSTM_MEMBAR_H_

#ifdef WLPDSTM_X86

inline void membar_full() {
	asm volatile("mfence":::"memory");
}

inline void membar_store_load_membar_store_store() {
	membar_full();
}

inline void membar_load_store_membar_store_store() {
	membar_full();
}

inline void membar_store_load() {
	membar_full();
}

inline void membar_store_store() {
	membar_full();
}

#elif defined WLPDSTM_SPARC

inline void membar_store_load_membar_store_store() {
	asm volatile ("membar #StoreLoad;"
				  "membar #StoreStore"
				  :::"memory");
}

inline void membar_load_store_membar_store_store() {
	asm volatile ("membar #LoadStore;"
				  "membar #StoreStore"
				  :::"memory");	
}

inline void membar_store_load() {
	asm volatile ("membar #StoreLoad":::"memory");
}

inline void membar_store_store() {
	asm volatile ("membar #StoreStore":::"memory");
}

#endif /* arch */

#endif /* WLPDSTM_MEMBAR_H_ */

