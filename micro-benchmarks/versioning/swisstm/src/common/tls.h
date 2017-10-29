/**
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 */

#ifndef WLPDSTM_TLS_H_
#define WLPDSTM_TLS_H_

namespace wlpdstm {

	///////////////////////
	// invoke init start //
	///////////////////////

	template<typename T, bool INIT = true>
	struct GlobalInitInvoker {
		static void GlobalInit() {
			T::GlobalInit();
		}
	};

	template<typename T>
	struct GlobalInitInvoker<T, false> {
		static void GlobalInit() {
			// do nothing
		}
	};

	template<typename T, bool INIT = true>
	struct ThreadInitInvoker {
		static void ThreadInit(T *obj) {
			obj->ThreadInit();
		}
	};

	template<typename T>
	struct ThreadInitInvoker<T, false> {
		static void ThreadInit(T *obj) {
			// do nothing
		}
	};
}
	/////////////////////
	// invoke init end //
	/////////////////////

#ifdef USE_PTHREAD_TLS

#include <pthread.h>

namespace wlpdstm {
	/**
	 * This is a TLS class that will put one instance of templated
	 * class into TLS storage and provide access to it. Assumption here
	 * is that the TLS class exposes default constructor. If this is
	 * not the case this class should be slightly changed.
	 * 
	 */
	template<class T, bool GLOBAL_INIT, bool THREAD_INIT>
	class Tls {
		public:
			static void GlobalInit();
			static void ThreadInit();
			static T *Get();

		private:
			static ::pthread_key_t tlsKey;
			static ::pthread_key_t initKey;
	};
}

template<class T, bool GLOBAL_INIT, bool THREAD_INIT> ::pthread_key_t wlpdstm::Tls<T, GLOBAL_INIT, THREAD_INIT>::tlsKey;
template<class T, bool GLOBAL_INIT, bool THREAD_INIT> ::pthread_key_t wlpdstm::Tls<T, GLOBAL_INIT, THREAD_INIT>::initKey;

template<class T, bool GLOBAL_INIT, bool THREAD_INIT>
inline void wlpdstm::Tls<T, GLOBAL_INIT, THREAD_INIT>::GlobalInit() {
	::pthread_key_create(&tlsKey, NULL);
	GlobalInitInvoker<T, GLOBAL_INIT>::GlobalInit();

	// not locally initialized
	::pthread_key_create(&initKey, NULL);
	::pthread_setspecific(initKey, (const void *)false);
}

template<class T, bool GLOBAL_INIT, bool THREAD_INIT>
inline void wlpdstm::Tls<T, GLOBAL_INIT, THREAD_INIT>::ThreadInit() {
	bool initialized = (bool)::pthread_getspecific(initKey);

	if(!initialized) {
		T *obj = new T();
		::pthread_setspecific(tlsKey, (const void *)obj);
		ThreadInitInvoker<T, THREAD_INIT>::ThreadInit(obj);
		::pthread_setspecific(initKey, (const void *)true);
	}
}

template<class T, bool GLOBAL_INIT, bool THREAD_INIT>
inline T *wlpdstm::Tls<T, GLOBAL_INIT, THREAD_INIT>::Get() {
	return (T *)::pthread_getspecific(tlsKey);
}

#else

namespace wlpdstm {

	template<class T, bool GLOBAL_INIT, bool THREAD_INIT>
	class Tls {
		public:
			static void GlobalInit() {
				GlobalInitInvoker<T, GLOBAL_INIT>::GlobalInit();
			}

			static void ThreadInit() {
				if(!init) {
					val = new T();
					ThreadInitInvoker<T, THREAD_INIT>::ThreadInit(val);
					init = true;
				}
			}

			static T *Get() {
				return val;
			}

		private:
			static __thread T *val;
			static __thread bool init;
	};
}

#if defined(__INTEL_COMPILER)
template<class T, bool GLOBAL_INIT, bool THREAD_INIT> T* wlpdstm::Tls<T, GLOBAL_INIT, THREAD_INIT>::val;
template<class T, bool GLOBAL_INIT, bool THREAD_INIT> bool wlpdstm::Tls<T, GLOBAL_INIT, THREAD_INIT>::init;
#else
template<class T, bool GLOBAL_INIT, bool THREAD_INIT> __thread T* wlpdstm::Tls<T, GLOBAL_INIT, THREAD_INIT>::val;
template<class T, bool GLOBAL_INIT, bool THREAD_INIT> __thread bool wlpdstm::Tls<T, GLOBAL_INIT, THREAD_INIT>::init;
#endif


#endif // USE_PTHREAD_TLS

#endif // WLPDSTM_TLS_H_
