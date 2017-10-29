#ifndef ATOMIC_H_
#define ATOMIC_H_

#include <atomic_ops.h>
#include <stdint.h>

#include "constants.h"
#include "membar.h"

#define aword AO_t

namespace wlpdstm {

	uintptr_t compare_and_swap_release(uintptr_t *addr, uintptr_t oldval, uintptr_t newval);
}

#define atomic_cas_no_barrier(addr, old_val, new_val) (AO_compare_and_swap((volatile AO_t *)(addr), (AO_t)(old_val), (AO_t)(new_val)))
#define atomic_cas_full(addr, old_val, new_val) (AO_compare_and_swap_full((volatile AO_t *)(addr), (AO_t)(old_val), (AO_t)(new_val)))
#define atomic_cas_acquire(addr, old_val, new_val) (AO_compare_and_swap_acquire((volatile AO_t *)(addr), (AO_t)(old_val), (AO_t)(new_val)))
#define atomic_cas_release(addr, old_val, new_val) (AO_compare_and_swap_release((volatile AO_t *)(addr), (AO_t)(old_val), (AO_t)(new_val)))

#define atomic_store_full(addr, val) (AO_store_full((volatile AO_t *)(addr), (AO_t)(val)))
#define atomic_store_release(addr, val) (AO_store_release((volatile AO_t *)(addr), (AO_t)(val)))
#define atomic_store_no_barrier(addr, val) (AO_store((volatile AO_t *)(addr), (AO_t)(val)))

#define atomic_load_acquire(addr) (AO_load_acquire((volatile AO_t *)(addr)))
#define atomic_load_no_barrier(addr) (AO_load((volatile AO_t *)(addr)))

#define fetch_and_inc_no_barrier(addr) (AO_fetch_and_add1((volatile AO_t *)(addr)))
#define fetch_and_inc_full(addr) (AO_fetch_and_add1_full((volatile AO_t *)(addr)))
#define fetch_and_inc_acquire(addr) (AO_fetch_and_add1_acquire((volatile AO_t *)(addr)))
#define fetch_and_inc_release(addr) (AO_fetch_and_add1_release((volatile AO_t *)(addr)))

inline uintptr_t wlpdstm::compare_and_swap_release(uintptr_t *addr, uintptr_t oldval, uintptr_t newval) {
#ifdef WLPDSTM_X86
    uintptr_t ret;
	
    asm volatile("lock;"
                 "cmpxchgl %1, %2;"
                 : "=a"(ret)
                 : "q"(newval), "m"(*addr), "a"(oldval)
                 : "memory");
    return ret;	
#elif defined WLPDSTM_SPARC
#ifdef WLPDSTM_32
	asm volatile("cas [%2], %3, %0;"
#elif defined WLPDSTM_64
	asm volatile("casx [%2], %3, %0;"
#endif /* word size */
				 "membar #LoadStore"
                 : "=&r"(newval)
                 : "0"(newval), "r"(addr), "r"(oldval)
                 : "memory");
    return newval;	
#endif /* arch */
}

namespace wlpdstm {

	template <typename T, T INITIAL_VALUE>
	class AtomicVariable {
		public:
			AtomicVariable() : value(INITIAL_VALUE)
				{  }

			void setValue(T newval) {
				atomic_store_no_barrier(&value, newval);
			}

			T getValue() {
				return (T)atomic_load_no_barrier(&value);
			}

			bool exchangeValue(T oldval, T newval) {
				return atomic_cas_no_barrier(&value, oldval, newval);
			}

		protected:
			T value;
	};

	class CounterOF : protected AtomicVariable<Word, 0> {
		public:
			Word getNext() {
				return fetch_and_inc_no_barrier(&value);
			}

			Word getMax() {
				return getValue();
			}
	};

	class PaddedSpinTryLock {
		private:
			enum SpinLockState {
				Free,
				Acquired
			};

		public:
			PaddedSpinTryLock() {
				padded_state.state = Free;
			}

			bool try_lock() {
				return atomic_cas_acquire(&(padded_state.state),
										  Free, Acquired);
			}

			void release() {
				atomic_store_release(&(padded_state.state), Free);
			}

		private:
			union {
				volatile Word state;
				char padding[CACHE_LINE_SIZE_BYTES];
			} padded_state;
	};
}

#endif
